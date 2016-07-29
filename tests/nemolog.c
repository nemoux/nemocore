#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <getopt.h>

#include <oshelper.h>
#include <udphelper.h>
#include <nemomisc.h>

struct logtask;

typedef void (*nemolog_dispatch_task)(int efd, struct logtask *task);

struct nemolog {
	int usoc;

	char ip[64];
	int port;
};

struct logtask {
	int fd;

	nemolog_dispatch_task dispatch;

	struct nemolog *log;
};

static void nemolog_dispatch_message_task(int efd, struct logtask *task)
{
	struct nemolog *log = task->log;
	struct epoll_event ep;
	char msg[4096];
	int len;

	len = read(task->fd, msg, sizeof(msg) - 8);
	if (len <= 0) {
		if (errno == EAGAIN) {
			ep.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP;
			ep.data.ptr = (void *)task;
			epoll_ctl(efd, EPOLL_CTL_MOD, task->fd, &ep);
		} else {
			epoll_ctl(efd, EPOLL_CTL_DEL, task->fd, &ep);

			close(task->fd);

			free(task);
		}
	} else {
		msg[len] = '\0';

		if (log->port == 0)
			printf("%s", msg);
		else
			udp_send_to(log->usoc, log->ip, log->port, msg, strlen(msg) + 1);
	}
}

static void nemolog_dispatch_local_listen_task(int efd, struct logtask *task)
{
	struct sockaddr_un addr;
	struct logtask *ctask;
	struct epoll_event ep;
	socklen_t size;
	int csoc;

	csoc = accept(task->fd, (struct sockaddr *)&addr, &size);
	if (csoc <= 0)
		return;

	os_set_nonblocking_mode(csoc);

	ctask = (struct logtask *)malloc(sizeof(struct logtask));
	ctask->fd = csoc;
	ctask->dispatch = nemolog_dispatch_message_task;
	ctask->log = task->log;

	os_epoll_add_fd(efd, csoc, EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP, ctask);
}

static void nemolog_dispatch_udp_listen_task(int efd, struct logtask *task)
{
	struct nemolog *log = task->log;
	char msg[1024];

	udp_recv_from(log->usoc, log->ip, &log->port, msg, sizeof(msg));
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "socketpath",		required_argument,	NULL,		's' },
		{ "port",					required_argument,	NULL,		'p' },
		{ 0 }
	};
	struct nemolog *log;
	struct sockaddr_un addr;
	socklen_t size, namesize;
	struct logtask *task;
	struct epoll_event ep;
	struct epoll_event eps[16];
	char *socketpath = NULL;
	int port = 70000;
	int opt;
	int efd;
	int neps;
	int lsoc;
	int usoc;
	int i;

	while (opt = getopt_long(argc, argv, "s:p:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 's':
				socketpath = strdup(optarg);
				break;

			case 'p':
				port = strtoul(optarg, NULL, 10);
				break;

			default:
				break;
		}
	}

	if (socketpath == NULL)
		socketpath = strdup("/tmp/nemo.log.0");

	unlink(socketpath);

	log = (struct nemolog *)malloc(sizeof(struct nemolog));
	if (log == NULL)
		return -1;
	memset(log, 0, sizeof(struct nemolog));

	strcpy(log->ip, "0.0.0.0");
	log->port = 0;

	lsoc = socket(PF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (lsoc < 0)
		goto out0;

	os_set_nonblocking_mode(lsoc);

	addr.sun_family = AF_LOCAL;
	namesize = snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", socketpath);
	size = offsetof(struct sockaddr_un, sun_path) + namesize;

	if (bind(lsoc, (struct sockaddr *)&addr, size) < 0)
		goto out1;

	if (listen(lsoc, 16) < 0)
		goto out1;

	log->usoc = usoc = udp_create_socket("0.0.0.0", port);
	if (usoc < 0)
		goto out1;

	os_set_nonblocking_mode(usoc);

	efd = os_epoll_create_cloexec();
	if (efd < 0)
		goto out2;

	task = (struct logtask *)malloc(sizeof(struct logtask));
	task->fd = lsoc;
	task->dispatch = nemolog_dispatch_local_listen_task;
	task->log = log;

	os_epoll_add_fd(efd, lsoc, EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP, task);

	task = (struct logtask *)malloc(sizeof(struct logtask));
	task->fd = usoc;
	task->dispatch = nemolog_dispatch_udp_listen_task;
	task->log = log;

	os_epoll_add_fd(efd, usoc, EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP, task);

	while (1) {
		neps = epoll_wait(efd, eps, ARRAY_LENGTH(eps), -1);

		for (i = 0; i < neps; i++) {
			task = eps[i].data.ptr;
			task->dispatch(efd, task);
		}
	}

	close(efd);

out2:
	close(usoc);

out1:
	close(lsoc);

out0:
	free(socketpath);

	return 0;
}
