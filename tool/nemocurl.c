#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/epoll.h>
#include <curl/curl.h>

#include <nemotool.h>
#include <nemocurl.h>
#include <nemomisc.h>

static struct curlreq *nemocurl_create_request(void)
{
	struct curlreq *req;

	req = (struct curlreq *)malloc(sizeof(struct curlreq));
	if (req == NULL)
		return NULL;
	memset(req, 0, sizeof(struct curlreq));

	req->handle = curl_easy_init();
	if (req->handle == NULL)
		goto err1;

	return req;

err1:
	free(req);

	return NULL;
}

static void nemocurl_destroy_request(struct curlreq *req)
{
	curl_easy_cleanup(req->handle);

	nemolist_remove(&req->link);

	if (req->url != NULL)
		free(req->url);
	if (req->header != NULL)
		free(req->header);
	if (req->body != NULL)
		free(req->body);

	free(req);
}

static int nemocurl_update_fds(struct nemocurl *curl)
{
	CURLMcode res;
	fd_set readfds;
	fd_set writefds;
	fd_set errfds;
	uint32_t events;
	int maxfd;
	int fd;
	int i;

	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&errfds);

	res = curl_multi_fdset(curl->handle, &readfds, &writefds, &errfds, &maxfd);
	if (res != CURLM_OK)
		return -1;

	for (i = 0; i < curl->nfds; i++) {
		nemotool_unwatch_fd(curl->tool, curl->fds[i]);
	}

	curl->nfds = 0;

	for (fd = 0; fd <= maxfd; fd++) {
		events = 0;

		if (FD_ISSET(fd, &readfds))
			events |= EPOLLIN;
		if (FD_ISSET(fd, &writefds))
			events |= EPOLLOUT;
		if (FD_ISSET(fd, &errfds))
			events |= EPOLLERR;

		if (events == 0)
			continue;

		nemotool_watch_fd(curl->tool, fd, events, &curl->task);

		curl->fds[curl->nfds++] = fd;
	}
}

static int nemocurl_update_timer(struct nemocurl *curl)
{
	CURLMcode res;
	long timeout;

	res = curl_multi_timeout(curl->handle, &timeout);
	if (res != CURLM_OK)
		return -1;

	nemotimer_set_timeout(curl->timer, timeout);
}

static int nemocurl_perform(struct nemocurl *curl)
{
	CURLMcode res;
	CURLMsg *msg;
	int lefts;
	int running;
	int i;

	res = curl_multi_perform(curl->handle, &running);
	if (res == CURLM_CALL_MULTI_PERFORM) {
	} else if (res != CURLM_OK) {
		return -1;
	}

	while ((msg = curl_multi_info_read(curl->handle, &lefts))) {
		if (msg->msg == CURLMSG_DONE) {
			struct curlreq *req, *next;

			nemolist_for_each_safe(req, next, &curl->list, link) {
				if (req->handle == msg->easy_handle) {
					if (curl->callback != NULL)
						curl->callback(curl, req->id, req->url, req->header, req->nheader, req->body, req->nbody);

					nemocurl_destroy_request(req);

					break;
				}
			}
		}
	}

	if (running == 0) {
		for (i = 0; i < curl->nfds; i++) {
			nemotool_unwatch_fd(curl->tool, curl->fds[i]);
		}

		curl->nfds = 0;
	} else {
		nemocurl_update_fds(curl);
		nemocurl_update_timer(curl);
	}

	return 0;
}

static void nemocurl_dispatch_task(struct nemotask *task, uint32_t events)
{
	struct nemocurl *curl = (struct nemocurl *)container_of(task, struct nemocurl, task);

	nemocurl_perform(curl);
}

static void nemocurl_dispatch_timer(struct nemotimer *timer, void *data)
{
	struct nemocurl *curl = (struct nemocurl *)data;

	nemocurl_perform(curl);
}

static int _global_init_done = 0;

struct nemocurl *nemocurl_create(struct nemotool *tool)
{
	struct nemocurl *curl;

	if (_global_init_done++ == 0) {
		curl_global_init(CURL_GLOBAL_ALL);
	}

	curl = (struct nemocurl *)malloc(sizeof(struct nemocurl));
	if (curl == NULL)
		return NULL;
	memset(curl, 0, sizeof(struct nemocurl));

	curl->timer = nemotimer_create(tool);
	if (curl->timer == NULL)
		goto err1;
	nemotimer_set_callback(curl->timer, nemocurl_dispatch_timer);
	nemotimer_set_userdata(curl->timer, curl);

	curl->handle = curl_multi_init();
	if (curl->handle == NULL)
		goto err2;

	curl->tool = tool;

	curl->task.dispatch = nemocurl_dispatch_task;

	nemolist_init(&curl->list);

	return curl;

err2:
	nemotimer_destroy(curl->timer);

err1:
	free(curl);

	return NULL;
}

void nemocurl_destroy(struct nemocurl *curl)
{
	struct curlreq *req, *next;

	nemotimer_destroy(curl->timer);

	curl_multi_cleanup(curl->handle);

	nemolist_for_each_safe(req, next, &curl->list, link) {
		nemocurl_destroy_request(req);
	}

	free(curl);

	if (--_global_init_done == 0) {
		curl_global_cleanup();
	}
}

void nemocurl_set_callback(struct nemocurl *curl, nemocurl_dispatch_t callback)
{
	curl->callback = callback;
}

static size_t nemocurl_dispatch_write_header(char *buffer, size_t size, size_t nmemb, void *data)
{
	struct curlreq *req = (struct curlreq *)data;

	ARRAY_APPEND_BUFFER(req->header, req->sheader, req->nheader, buffer, size * nmemb);

	return size * nmemb;
}

static size_t nemocurl_dispatch_write_body(char *buffer, size_t size, size_t nmemb, void *data)
{
	struct curlreq *req = (struct curlreq *)data;

	ARRAY_APPEND_BUFFER(req->body, req->sbody, req->nbody, buffer, size * nmemb);

	return size * nmemb;
}

int nemocurl_request(struct nemocurl *curl, uint32_t id, const char *url, const char *post, uint32_t timeout, uint32_t flags, ...)
{
	struct curlreq *req;
	CURLcode res;
	va_list vargs;
	const char *name, *value;
	char *header;

	req = nemocurl_create_request();
	if (req == NULL)
		return -1;

	req->id = id;
	req->url = strdup(url);

	va_start(vargs, flags);

	while ((name = va_arg(vargs, const char *)) != NULL &&
			(value = va_arg(vargs, const char *)) != NULL) {
		asprintf(&header, "%s: %s", name, value);

		req->clist = curl_slist_append(req->clist, header);

		free(header);
	}

	va_end(vargs);

	curl_easy_setopt(req->handle, CURLOPT_URL, url);
	curl_easy_setopt(req->handle, CURLOPT_HEADERFUNCTION, nemocurl_dispatch_write_header);
	curl_easy_setopt(req->handle, CURLOPT_HEADERDATA, req);
	curl_easy_setopt(req->handle, CURLOPT_WRITEFUNCTION, nemocurl_dispatch_write_body);
	curl_easy_setopt(req->handle, CURLOPT_WRITEDATA, req);

	curl_easy_setopt(req->handle, CURLOPT_CONNECTTIMEOUT, timeout);

	if (flags & NEMOCURL_FOLLOW_LOCATION_FLAG) {
		curl_easy_setopt(req->handle, CURLOPT_FOLLOWLOCATION, 1);
	}

	if (req->clist != NULL) {
		curl_easy_setopt(req->handle, CURLOPT_HTTPHEADER, req->clist);
	}

	if (post != NULL) {
		curl_easy_setopt(req->handle, CURLOPT_POSTFIELDS, post);
		curl_easy_setopt(req->handle, CURLOPT_POSTFIELDSIZE, (long)strlen(post));
	}

	curl_multi_add_handle(curl->handle, req->handle);

	nemolist_insert(&curl->list, &req->link);

	return 0;
}

int nemocurl_dispatch(struct nemocurl *curl)
{
	CURLcode res;
	int running;

	res = curl_multi_perform(curl->handle, &running);
	if (res != CURLM_OK)
		return -1;

	if (running != 0) {
		nemocurl_update_fds(curl);
		nemocurl_update_timer(curl);
	}

	return 0;
}
