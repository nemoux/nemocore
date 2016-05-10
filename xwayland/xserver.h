#ifndef	__NEMO_XWAYLAND_SERVER_H__
#define	__NEMO_XWAYLAND_SERVER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <xcb/xcb.h>
#include <xcb/xfixes.h>
#include <xcb/composite.h>

#include <task.h>

struct nemocompz;
struct nemoxmanager;
struct nemoxwindow;

struct nemoxserver {
	struct wl_display *display;
	struct wl_event_loop *loop;

	char *xserverpath;

	struct nemoshell *shell;
	struct nemocompz *compz;

	struct wl_event_source *sigchld_source;

	int abstract_fd;
	struct wl_event_source *abstract_source;
	int unix_fd;
	struct wl_event_source *unix_source;

	int wm_fd;

	struct wl_event_source *sigusr1_source;

	int xdisplay;

	struct nemotask task;

	struct wl_resource *resource;
	struct wl_client *client;

	struct nemoxmanager *xmanager;

	struct wl_listener destroy_listener;
};

extern struct nemoxserver *nemoxserver_create(struct nemoshell *shell, const char *xserverpath, int xdisplay);
extern void nemoxserver_destroy(struct nemoxserver *xserver);

static inline pid_t nemoxserver_get_pid(struct nemoxserver *xserver)
{
	return xserver->task.pid;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
