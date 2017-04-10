#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <signal.h>
#include <sys/socket.h>
#include <wayland-server.h>

#include <compz.h>
#include <task.h>
#include <waylandhelper.h>
#include <nemomisc.h>

struct wl_client *nemotask_launch(struct nemocompz *compz, struct nemotask *task, const char *path, nemotask_cleanup_t cleanup)
{
	int sv[2];
	pid_t pid;
	struct wl_client *client;

	if (os_socketpair_cloexec(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
		return NULL;
	}

	pid = fork();
	if (pid == -1) {
		close(sv[0]);
		close(sv[1]);
		return NULL;
	}

	if (pid == 0) {
		sigset_t allsigs;

		sigfillset(&allsigs);
		sigprocmask(SIG_UNBLOCK, &allsigs, NULL);

		env_set_integer("WAYLAND_SOCKET", sv[1]);

		execl(path, path, NULL);

		exit(EXIT_FAILURE);
	}

	close(sv[1]);

	client = wl_client_create(compz->display, sv[0]);
	if (client == NULL) {
		close(sv[0]);
		return NULL;
	}

	task->pid = pid;
	task->cleanup = cleanup;

	wl_list_insert(&compz->task_list, &task->link);

	return client;
}

void nemotask_watch(struct nemocompz *compz, struct nemotask *task)
{
	wl_list_insert(&compz->task_list, &task->link);
}
