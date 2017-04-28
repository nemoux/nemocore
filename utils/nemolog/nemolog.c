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

#include <nemomisc.h>

struct logtask;

typedef void (*nemolog_dispatch_task)(int efd, struct logtask *task);

struct logtask {
	int fd;

	nemolog_dispatch_task dispatch;
};

static FILE *nemologfile = NULL;

static void nemolog_dispatch_message_task(int efd, struct logtask *task)
{
	struct epoll_event ep;
	char msg[4096];
	int len;

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

		if (nemologfile == NULL) {
			fprintf(stdout, "%s", msg);
		} else {
			fprintf(nemologfile, "%s", msg);
			fflush(nemologfile);
		}
	}
}

static void nemolog_dispatch_listen_task(int efd, struct logtask *task)
{
	struct sockaddr_un addr;
	struct logtask *ctask;
	struct epoll_event ep;
	socklen_t size;
	int csoc;

	csoc = accept(task->fd, (struct sockaddr *)&addr, &size);
	if (csoc <= 0)
		return;

	os_fd_set_nonblocking_mode(csoc);

	ctask = (struct logtask *)malloc(sizeof(struct logtask));
	ctask->fd = csoc;
	ctask->dispatch = nemolog_dispatch_message_task;

	os_epoll_add_fd(efd, csoc, EPOLLIN | EPOLLERR | EPOLLHUP, ctask);
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "socketpath",		required_argument,	NULL,		's' },
		{ "directory",		required_argument,	NULL,		'd' },
		{ 0 }
	};
	struct sockaddr_un addr;
	socklen_t size, namesize;
	struct logtask *task;
	struct epoll_event ep;
	struct epoll_event eps[16];
	char *socketpath = NULL;
	char *dirpath = NULL;
	int opt;
	int efd;
	int neps;
	int lsoc;
	int i;

	while (opt = getopt_long(argc, argv, "s:d:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 's':
				socketpath = strdup(optarg);
				break;

			case 'd':
				dirpath = strdup(optarg);
				break;

			default:
				break;
		}
	}

	if (socketpath == NULL)
		socketpath = strdup("/tmp/nemo.log.0");
	unlink(socketpath);

	if (dirpath != NULL) {
		time_t ttime;
		struct tm *stime;
		char filepath[256];
		char filename[128];

		time(&ttime);
		stime = localtime(&ttime);

		strftime(filename, sizeof(filename), "%Y-%m-%d.log", stime);
		snprintf(filepath, sizeof(filepath), "%s/%s", dirpath, filename);

		nemologfile = fopen(filepath, "a");
	}

	lsoc = socket(PF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (lsoc < 0)
		goto err0;

	os_fd_set_nonblocking_mode(lsoc);

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

	task = (struct logtask *)malloc(sizeof(struct logtask));
	task->fd = lsoc;
	task->dispatch = nemolog_dispatch_listen_task;

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
	if (nemologfile != NULL)
		fclose(nemologfile);

	free(socketpath);

	return 0;
}
