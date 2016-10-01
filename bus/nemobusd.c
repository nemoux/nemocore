#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <time.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <getopt.h>

#include <nemobus.h>
#include <nemohelper.h>
#include <nemolist.h>
#include <nemomisc.h>

struct busclient {
	char *path;

	int soc;

	struct nemolist link;
};

struct nemobusd {
	struct nemolist client_list;
};

struct bustask;

typedef void (*nemobus_dispatch_task)(int efd, struct bustask *task);

struct bustask {
	struct nemobusd *busd;

	int fd;

	nemobus_dispatch_task dispatch;
};

static void nemobus_dispatch_message(struct nemobusd *busd, int soc, const char *path, struct json_object *bobj)
{
	if (strcmp(path, "/nemobusd") == 0) {
		struct json_object *tobj;
		const char *type;

		if (json_object_object_get_ex(bobj, "type", &tobj) != 0) {
			type = json_object_get_string(tobj);

			if (strcmp(type, "advertise") == 0) {
				struct json_object *pobj;

				if (json_object_object_get_ex(bobj, "path", &pobj) != 0) {
					struct busclient *client;

					client = (struct busclient *)malloc(sizeof(struct busclient));
					client->path = strdup(json_object_get_string(pobj));
					client->soc = soc;

					nemolist_insert_tail(&busd->client_list, &client->link);

					json_object_put(pobj);
				}
			}

			json_object_put(tobj);
		}
	} else {
		struct busclient *client;
		const char *contents;
		int ncontents;

		contents = json_object_get_string(bobj);
		ncontents = strlen(contents);

		nemolist_for_each(client, &busd->client_list, link) {
			if (namespace_has_prefix(client->path, path) != 0) {
				send(client->soc, contents, ncontents, MSG_NOSIGNAL | MSG_DONTWAIT);
			}
		}
	}
}

static void nemobus_dispatch_message_task(int efd, struct bustask *task)
{
	struct nemobusd *busd = task->busd;
	struct epoll_event ep;
	struct json_object *jobj;
	struct json_object *pobj;
	struct json_object *bobj;
	struct json_object *cobj;
	const char *type;
	const char *path;
	const char *body;
	char msg[4096];
	int len;
	int i;

	len = read(task->fd, msg, sizeof(msg));
	if (len < 0) {
		if (errno == EAGAIN) {
			ep.events = EPOLLIN | EPOLLERR | EPOLLHUP;
			ep.data.ptr = (void *)task;
			epoll_ctl(efd, EPOLL_CTL_MOD, task->fd, &ep);
		} else {
			epoll_ctl(efd, EPOLL_CTL_DEL, task->fd, &ep);

			close(task->fd);

			free(task);
		}
	} else if (len == 0) {
		epoll_ctl(efd, EPOLL_CTL_DEL, task->fd, &ep);

		close(task->fd);

		free(task);
	} else {
		msg[len] = '\0';

		jobj = json_tokener_parse(msg);

		if (json_object_object_get_ex(jobj, "path", &pobj) == 0)
			goto out0;
		if (json_object_object_get_ex(jobj, "body", &bobj) == 0)
			goto out1;

		if (json_object_is_type(pobj, json_type_string)) {
			nemobus_dispatch_message(busd, task->fd, json_object_get_string(pobj), bobj);
		} else if (json_object_is_type(pobj, json_type_array)) {
			for (i = 0; i < json_object_array_length(pobj); i++) {
				cobj = json_object_array_get_idx(pobj, i);

				nemobus_dispatch_message(busd, task->fd, json_object_get_string(cobj), bobj);
			}
		}

		json_object_put(bobj);
out1:
		json_object_put(pobj);
out0:
		json_object_put(jobj);
	}
}

static void nemobus_dispatch_listen_task(int efd, struct bustask *task)
{
	struct nemobusd *busd = task->busd;
	struct busclient *client;
	struct sockaddr_un addr;
	struct bustask *ctask;
	struct epoll_event ep;
	socklen_t size;
	int csoc;

	csoc = accept(task->fd, (struct sockaddr *)&addr, &size);
	if (csoc <= 0)
		return;

	os_set_nonblocking_mode(csoc);

	ctask = (struct bustask *)malloc(sizeof(struct bustask));
	ctask->fd = csoc;
	ctask->busd = busd;
	ctask->dispatch = nemobus_dispatch_message_task;

	os_epoll_add_fd(efd, csoc, EPOLLIN | EPOLLERR | EPOLLHUP, ctask);
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "socketpath",				required_argument,		NULL,		's' },
		{ 0 }
	};
	struct sockaddr_un addr;
	socklen_t size, namesize;
	struct nemobusd *busd;
	struct bustask *task;
	struct epoll_event ep;
	struct epoll_event eps[16];
	char *socketpath = NULL;
	int efd;
	int neps;
	int lsoc;
	int opt;
	int i;

	while (opt = getopt_long(argc, argv, "s:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 's':
				socketpath = strdup(optarg);
				break;

			default:
				break;
		}
	}

	if (socketpath == NULL)
		socketpath = strdup("/tmp/nemo.bus.0");
	unlink(socketpath);

	lsoc = socket(PF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (lsoc < 0)
		goto err0;

	os_set_nonblocking_mode(lsoc);

	addr.sun_family = AF_LOCAL;
	namesize = snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", socketpath);
	size = offsetof(struct sockaddr_un, sun_path) + namesize;

	if (bind(lsoc, (struct sockaddr *)&addr, size) < 0)
		goto err1;

	if (listen(lsoc, 16) < 0)
		goto err1;

	efd = os_epoll_create_cloexec();
	if (efd < 0)
		goto err1;

	busd = (struct nemobusd *)malloc(sizeof(struct nemobusd));
	nemolist_init(&busd->client_list);

	task = (struct bustask *)malloc(sizeof(struct bustask));
	task->fd = lsoc;
	task->busd = busd;
	task->dispatch = nemobus_dispatch_listen_task;

	os_epoll_add_fd(efd, lsoc, EPOLLIN | EPOLLERR | EPOLLHUP, task);

	while (1) {
		neps = epoll_wait(efd, eps, ARRAY_LENGTH(eps), -1);

		for (i = 0; i < neps; i++) {
			task = eps[i].data.ptr;
			task->dispatch(efd, task);
		}
	}

	close(efd);

err1:
	close(lsoc);

err0:
	free(socketpath);

	return 0;
}
