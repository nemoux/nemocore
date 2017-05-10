#ifndef	__NEMO_XWAYLAND_SERVER_H__
#define	__NEMO_XWAYLAND_SERVER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <xcb/xcb.h>
#include <xcb/xfixes.h>
#include <xcb/composite.h>

#include <compz.h>

typedef enum {
	NEMOXSERVER_NONE_STATE = 0,
	NEMOXSERVER_INIT_STATE = 1,
	NEMOXSERVER_READY_STATE = 2,
	NEMOXSERVER_USED_STATE = 3,
	NEMOXSERVER_LAST_STATE
} NemoXServerState;

struct nemoxmanager;
struct nemoxwindow;

struct nemoxserver {
	struct wl_display *display;
	struct wl_event_loop *loop;

	int state;

	struct wl_list link;

	struct wl_signal sigusr1_signal;

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

	char *rendernode;

	struct nemotask task;

	struct wl_resource *resource;
	struct wl_client *client;

	struct nemoxmanager *xmanager;

	struct wl_listener destroy_listener;
};

extern struct nemoxserver *nemoxserver_create(struct nemoshell *shell, const char *xserverpath, int xdisplay);
extern void nemoxserver_destroy(struct nemoxserver *xserver);

extern int nemoxserver_execute(struct nemoxserver *xserver);

extern void nemoxserver_set_rendernode(struct nemoxserver *xserver, const char *rendernode);

static inline pid_t nemoxserver_get_pid(struct nemoxserver *xserver)
{
	return xserver->task.pid;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
