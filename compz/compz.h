#ifndef	__NEMOCOMPZ_COMPZ_H__
#define	__NEMOCOMPZ_COMPZ_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <time.h>
#include <libudev.h>

#include <backend.h>
#include <layer.h>
#include <renderer.h>
#include <event.h>
#include <nemoitem.h>

#define	NEMOCOMPZ_NODE_MAX				(4)
#define	NEMOCOMPZ_POINTER_MAX			(32)

#define	NEMOCOMPZ_DEFAULT_FRAME_TIMEOUT		(1000.0f / 60.0f)

typedef enum {
	NEMOCOMPZ_NONE_STATE = 0,
	NEMOCOMPZ_RUNNING_STATE = 1,
	NEMOCOMPZ_EXIT_STATE = 2,
	NEMOCOMPZ_LAST_STATE
} NemoCompzState;

struct nemosession;
struct nemoseat;
struct nemolayer;
struct nemoanimation;
struct nemoeffect;
struct nemoscreen;

typedef void (*nemocompz_dispatch_idle_t)(void *data);

struct nemoproc {
	pid_t pid;
};

struct nemocompz {
	struct wl_display *display;
	struct wl_event_loop *loop;
	char *name;

	struct wl_signal destroy_signal;

	int state;
	int dirty;
	pixman_region32_t damage;

	pixman_region32_t scene;
	int has_scene;

	pixman_region32_t scope;
	int has_scope;

	struct wl_event_source *sigsrc[8];

	struct udev *udev;

	struct nemoseat *seat;
	struct nemosession *session;

	int use_pixman;
	struct nemorenderer *renderer;

	struct wl_list key_binding_list;
	struct wl_list button_binding_list;
	struct wl_list touch_binding_list;

	struct wl_list backend_list;
	struct wl_list screen_list;
	struct wl_list render_list;
	struct wl_list evdev_list;
	struct wl_list touch_list;
	struct wl_list tuio_list;
	struct wl_list virtuio_list;
	struct wl_list animation_list;
	struct wl_list effect_list;
	struct wl_list task_list;
	struct wl_list layer_list;
	struct wl_list canvas_list;
	struct wl_list actor_list;
	struct wl_list feedback_list;

	struct nemolayer cursor_layer;

	uint32_t screen_idpool;
	uint32_t render_idpool;

	uint32_t keyboard_ids;
	uint32_t pointer_ids;

	uint64_t touch_ids;

	int32_t nodemax;

	struct wl_signal session_signal;
	int session_active;

	struct wl_signal show_input_panel_signal;
	struct wl_signal hide_input_panel_signal;
	struct wl_signal update_input_panel_signal;

	struct wl_signal create_surface_signal;
	struct wl_signal activate_signal;
	struct wl_signal transform_signal;
	struct wl_signal kill_signal;
	struct wl_signal child_signal;

	clockid_t presentation_clock;

	struct wl_event_source *frame_timer;
	struct wl_list frame_list;
	uint32_t frame_timeout;
	int frame_done;

	struct wl_event_source *framerate_timer;

	struct nemoevent *event;

	struct nemoitem *configs;
};

extern struct nemocompz *nemocompz_create(void);
extern void nemocompz_destroy(struct nemocompz *compz);

extern int nemocompz_prepare(struct nemocompz *compz, const char *backend, const char *seat, int tty);

extern int nemocompz_run(struct nemocompz *compz);
extern void nemocompz_exit(struct nemocompz *compz);

extern void nemocompz_destroy_clients(struct nemocompz *compz);

extern void nemocompz_acculumate_damage(struct nemocompz *compz);
extern void nemocompz_flush_damage(struct nemocompz *compz);

extern void nemocompz_make_current(struct nemocompz *compz);

extern struct nemoscreen *nemocompz_get_main_screen(struct nemocompz *compz);
extern struct nemoscreen *nemocompz_get_screen_on(struct nemocompz *compz, float x, float y);
extern struct nemoscreen *nemocompz_get_screen(struct nemocompz *compz, uint32_t nodeid, uint32_t screenid);

extern void nemocompz_set_screen_frame_listener(struct nemocompz *compz, struct nemoscreen *screen);
extern void nemocompz_put_screen_frame_listener(struct nemocompz *compz);

extern int32_t nemocompz_get_scene_width(struct nemocompz *compz);
extern int32_t nemocompz_get_scene_height(struct nemocompz *compz);
extern void nemocompz_update_scene(struct nemocompz *compz);
extern void nemocompz_set_scene(struct nemocompz *compz, int32_t x, int32_t y, int32_t width, int32_t height);
extern void nemocompz_set_scope(struct nemocompz *compz, int32_t x, int32_t y, int32_t width, int32_t height);

extern void nemocompz_update_transform(struct nemocompz *compz);
extern void nemocompz_update_output(struct nemocompz *compz);

extern void nemocompz_dispatch_animation(struct nemocompz *compz, struct nemoanimation *animation);
extern void nemocompz_dispatch_effect(struct nemocompz *compz, struct nemoeffect *effect);

extern void nemocompz_dispatch_frame(struct nemocompz *compz);
extern void nemocompz_set_frame_timeout(struct nemocompz *compz, uint32_t timeout);

extern void nemocompz_dispatch_idle(struct nemocompz *compz, nemocompz_dispatch_idle_t dispatch, void *data);

extern struct nemoevent *nemocompz_get_main_event(struct nemocompz *compz);

extern int nemocompz_set_presentation_clock(struct nemocompz *compz, clockid_t id);
extern int nemocompz_set_presentation_clock_software(struct nemocompz *compz);
extern void nemocompz_get_presentation_clock(struct nemocompz *compz, struct timespec *ts);

extern int nemocompz_is_running(struct nemocompz *compz);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
