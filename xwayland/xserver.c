#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <wayland-server.h>

#include <compz.h>
#include <shell.h>
#include <canvas.h>
#include <view.h>
#include <xserver.h>
#include <xmanager.h>
#include <task.h>
#include <nemomisc.h>
#include <nemolog.h>

static int nemoxserver_handle_sigusr1(int signum, void *data)
{
	struct nemoxserver *xserver = (struct nemoxserver *)data;

	nemoxmanager_create(xserver, xserver->wm_fd);
	wl_event_source_remove(xserver->sigusr1_source);

	return 1;
}

static int nemoxserver_handle_event(int listenfd, uint32_t mask, void *data)
{
	struct nemoxserver *xserver = (struct nemoxserver *)data;
	char display[8], s[8], abstract_fd[8], unix_fd[8], wm_fd[8];
	int sv[2], wm[2], fd;

	if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sv) < 0) {
		nemolog_error("XWAYLAND", "failed to create socketpair for wayland");
		return 1;
	}

	if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, wm) < 0) {
		nemolog_error("XWAYLAND", "failed to create socketpair for X");
		return 1;
	}

	xserver->task.pid = fork();
	if (xserver->task.pid == 0) {
		const char *path = xserver->xserverpath;

		fd = dup(sv[1]);
		if (fd < 0)
			return 1;

		snprintf(s, sizeof(s), "%d", fd);
		setenv("WAYLAND_SOCKET", s, 1);

		snprintf(display, sizeof(display), ":%d", xserver->xdisplay);

		fd = dup(xserver->abstract_fd);
		if (fd < 0)
			return 1;
		snprintf(abstract_fd, sizeof(abstract_fd), "%d", fd);

		fd = dup(xserver->unix_fd);
		if (fd < 0)
			return 1;
		snprintf(unix_fd, sizeof(unix_fd), "%d", fd);

		fd = dup(wm[1]);
		if (fd < 0)
			return 1;
		snprintf(wm_fd, sizeof(wm_fd), "%d", fd);

		signal(SIGUSR1, SIG_IGN);

		if (execl(path, path, display, "-rootless", "-listen", abstract_fd, "-listen", unix_fd, "-wm", wm_fd, "-terminate", NULL) < 0)
			nemolog_error("XWAYLAND", "failed to execute xserver");

		exit(EXIT_FAILURE);	
	} else if (xserver->task.pid == -1) {
		nemolog_error("XWAYLAND", "failed to fork");
	} else {
		close(sv[1]);
		xserver->client = wl_client_create(xserver->display, sv[0]);

		close(wm[1]);
		xserver->wm_fd = wm[0];

		nemotask_watch(xserver->compz, &xserver->task);

		wl_event_source_remove(xserver->abstract_source);
		wl_event_source_remove(xserver->unix_source);
	}

	return 1;
}

static int nemoxserver_bind_to_abstract_socket(int xdisplay)
{
	struct sockaddr_un addr;
	socklen_t size, namesize;
	int fd;

	fd = socket(PF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (fd < 0)
		return -1;

	addr.sun_family = AF_LOCAL;
	namesize = snprintf(addr.sun_path, sizeof(addr.sun_path), "%c/tmp/.X11-unix/X%d", 0, xdisplay);
	size = offsetof(struct sockaddr_un, sun_path) + namesize;
	if (bind(fd, (struct sockaddr *)&addr, size) < 0) {
		nemolog_error("XWAYLAND", "failed to bind to @%s: %s", addr.sun_path + 1, strerror(errno));
		close(fd);
		return -1;
	}

	if (listen(fd, 1) < 0) {
		close(fd);
		return -1;
	}

	return fd;
}

static int nemoxserver_bind_to_unix_socket(int xdisplay)
{
	struct sockaddr_un addr;
	socklen_t size, namesize;
	int fd;

	fd = socket(PF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (fd < 0)
		return -1;

	addr.sun_family = AF_LOCAL;
	namesize = snprintf(addr.sun_path, sizeof(addr.sun_path), "/tmp/.X11-unix/X%d", xdisplay) + 1;
	size = offsetof(struct sockaddr_un, sun_path) + namesize;
	unlink(addr.sun_path);
	if (bind(fd, (struct sockaddr *)&addr, size) < 0) {
		nemolog_error("XWAYLAND", "failed to bind to %s (%s)", addr.sun_path, strerror(errno));
		close(fd);
		return -1;
	}

	if (listen(fd, 1) < 0) {
		unlink(addr.sun_path);
		close(fd);
		return -1;
	}

	return fd;
}

static int nemoxserver_create_lockfile(int xdisplay, char *lockfile, size_t lsize)
{
	char pid[16], *end;
	int fd, size;
	pid_t other;

	snprintf(lockfile, lsize, "/tmp/.X%d-lock", xdisplay);
	fd = open(lockfile, O_WRONLY | O_CLOEXEC | O_CREAT | O_EXCL, 0444);
	if (fd < 0 && errno == EEXIST) {
		fd = open(lockfile, O_CLOEXEC | O_RDONLY);
		if (fd < 0 || read(fd, pid, 11) != 11) {
			nemolog_error("XWAYLAND", "can't read lock file %s: %s", lockfile, strerror(errno));
			if (fd >= 0)
				close(fd);

			errno = EEXIST;
			return -1;
		}

		other = strtol(pid, &end, 0);
		if (end != pid + 10) {
			nemolog_error("XWAYLAND", "can't parse lock file %s", lockfile);
			close(fd);
			errno = EEXIST;
			return -1;
		}

		if (kill(other, 0) < 0 && errno == ESRCH) {
			nemolog_error("XWAYLAND", "unlinking stale lock file %s", lockfile);
			close(fd);
			if (unlink(lockfile))
				errno = EEXIST;
			else
				errno = EAGAIN;
			return -1;
		}

		close(fd);
		errno = EEXIST;
		return -1;
	} else if (fd < 0) {
		nemolog_error("XWAYLAND", "failed to create lock file %s: %s", lockfile, strerror(errno));
		return -1;
	}

	size = snprintf(pid, sizeof(pid), "%10d\n", getpid());
	if (write(fd, pid, size) != size) {
		unlink(lockfile);
		close(fd);
		return -1;
	}

	close(fd);

	return 0;
}

static void nemoxserver_handle_compz_destroy(struct wl_listener *listener, void *data)
{
	struct nemoxserver *xserver = container_of(listener, struct nemoxserver, destroy_listener);

	if (xserver == NULL)
		return;

	if (xserver->loop != NULL)
		nemoxserver_shutdown(xserver);

	free(xserver);
}

static void nemoxserver_cleanup(struct nemotask *task, int status)
{
	struct nemoxserver *xserver = container_of(task, struct nemoxserver, task);

	xserver->task.pid = 0;
	xserver->client = NULL;
	xserver->resource = NULL;

	xserver->abstract_source = wl_event_loop_add_fd(xserver->loop, xserver->abstract_fd, WL_EVENT_READABLE, nemoxserver_handle_event, xserver);
	xserver->unix_source = wl_event_loop_add_fd(xserver->loop, xserver->unix_fd, WL_EVENT_READABLE, nemoxserver_handle_event, xserver);

	if (xserver->xmanager != NULL) {
		nemoxmanager_destroy(xserver->xmanager);
		xserver->xmanager = NULL;
	} else {
		nemoxserver_shutdown(xserver);
	}
}

struct nemoxserver *nemoxserver_create(struct nemoshell *shell, const char *xserverpath)
{
	struct nemocompz *compz = shell->compz;
	struct wl_display *display = compz->display;
	struct nemoxserver *xserver;
	char lockfile[256], dispname[8];
	
	if (xserverpath == NULL)
		return NULL;

	xserver = (struct nemoxserver *)malloc(sizeof(struct nemoxserver));
	if (xserver == NULL)
		return NULL;
	memset(xserver, 0, sizeof(struct nemoxserver));

	xserver->task.cleanup = nemoxserver_cleanup;
	xserver->display = display;
	xserver->compz = compz;
	xserver->shell = shell;

	xserver->xdisplay = 0;

retry:
	if (nemoxserver_create_lockfile(xserver->xdisplay, lockfile, sizeof(lockfile)) < 0) {
		if (errno == EAGAIN) {
			goto retry;
		} else if (errno == EEXIST) {
			xserver->xdisplay++;
			goto retry;
		} else {
			goto err1;
		}
	}

	xserver->abstract_fd = nemoxserver_bind_to_abstract_socket(xserver->xdisplay);
	if (xserver->abstract_fd < 0 && errno == EADDRINUSE) {
		xserver->xdisplay++;
		unlink(lockfile);
		goto retry;
	}

	xserver->unix_fd = nemoxserver_bind_to_unix_socket(xserver->xdisplay);
	if (xserver->unix_fd < 0) {
		unlink(lockfile);
		goto err2;
	}

	snprintf(dispname, sizeof(dispname), ":%d", xserver->xdisplay);
	setenv("DISPLAY", dispname, 1);

	xserver->xserverpath = strdup(xserverpath);
	xserver->loop = wl_display_get_event_loop(display);
	xserver->abstract_source = wl_event_loop_add_fd(xserver->loop, xserver->abstract_fd, WL_EVENT_READABLE, nemoxserver_handle_event, xserver);
	xserver->unix_source = wl_event_loop_add_fd(xserver->loop, xserver->unix_fd, WL_EVENT_READABLE, nemoxserver_handle_event, xserver);

	xserver->sigusr1_source = wl_event_loop_add_signal(xserver->loop, SIGUSR1, nemoxserver_handle_sigusr1, xserver);

	xserver->destroy_listener.notify = nemoxserver_handle_compz_destroy;
	wl_signal_add(&compz->destroy_signal, &xserver->destroy_listener);

	return xserver;

err2:
	close(xserver->abstract_fd);

err1:
	free(xserver);

	return NULL;
}

void nemoxserver_shutdown(struct nemoxserver *xserver)
{
	char path[256];

	snprintf(path, sizeof(path), "/tmp/.X%d-lock", xserver->xdisplay);
	unlink(path);
	snprintf(path, sizeof(path), "/tmp/.X11-unix/X%d", xserver->xdisplay);
	unlink(path);

	if (xserver->xserverpath != NULL)
		free(xserver->xserverpath);

	if (xserver->task.pid == 0) {
		wl_event_source_remove(xserver->abstract_source);
		wl_event_source_remove(xserver->unix_source);
	}

	wl_list_remove(&xserver->destroy_listener.link);

	close(xserver->unix_fd);
	close(xserver->abstract_fd);

	if (xserver->xmanager != NULL)
		nemoxmanager_destroy(xserver->xmanager);

	xserver->loop = NULL;
}
