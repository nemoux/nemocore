#ifndef	__NEMOSHELL_H__
#define	__NEMOSHELL_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <compz.h>
#include <pixman.h>
#include <layer.h>
#include <view.h>
#include <content.h>
#include <nemotoken.h>
#include <nemoitem.h>

typedef enum {
	NEMOSHELL_SURFACE_NONE_TYPE = 0,
	NEMOSHELL_SURFACE_NORMAL_TYPE = 1,
	NEMOSHELL_SURFACE_POPUP_TYPE = 2,
	NEMOSHELL_SURFACE_OVERLAY_TYPE = 3,
	NEMOSHELL_SURFACE_XWAYLAND_TYPE = 4,
	NEMOSHELL_SURFACE_LAST_TYPE
} NemoShellSurfaceType;

typedef enum {
	NEMOSHELL_SURFACE_MOVABLE_FLAG = (1 << 0),
	NEMOSHELL_SURFACE_RESIZABLE_FLAG = (1 << 1),
	NEMOSHELL_SURFACE_SCALABLE_FLAG = (1 << 2),
	NEMOSHELL_SURFACE_PICKABLE_FLAG = (1 << 3),
	NEMOSHELL_SURFACE_MAXIMIZABLE_FLAG = (1 << 4),
	NEMOSHELL_SURFACE_MINIMIZABLE_FLAG = (1 << 5),
	NEMOSHELL_SURFACE_ALL_FLAGS = NEMOSHELL_SURFACE_MOVABLE_FLAG | NEMOSHELL_SURFACE_RESIZABLE_FLAG | NEMOSHELL_SURFACE_SCALABLE_FLAG | NEMOSHELL_SURFACE_PICKABLE_FLAG | NEMOSHELL_SURFACE_MAXIMIZABLE_FLAG | NEMOSHELL_SURFACE_MINIMIZABLE_FLAG,
} NemoShellSurfaceFlag;

typedef enum {
	NEMOSHELL_BIN_BINDABLE_STATE = (1 << 0),
	NEMOSHELL_BIN_FIXED_STATE = (1 << 1),
	NEMOSHELL_BIN_PICKSCREEN_STATE = (1 << 2),
	NEMOSHELL_BIN_PITCHSCREEN_STATE = (1 << 3)
} NemoShellBinState;

typedef enum {
	NEMOSHELL_PICK_SCALE_FLAG = (1 << 0),
	NEMOSHELL_PICK_ROTATE_FLAG = (1 << 1),
	NEMOSHELL_PICK_TRANSLATE_FLAG = (1 << 2),
	NEMOSHELL_PICK_RESIZE_FLAG = (1 << 3),
	NEMOSHELL_PICK_ALL_FLAGS = NEMOSHELL_PICK_SCALE_FLAG | NEMOSHELL_PICK_ROTATE_FLAG | NEMOSHELL_PICK_TRANSLATE_FLAG | NEMOSHELL_PICK_RESIZE_FLAG
} NemoShellPickFlag;

typedef enum {
	NEMOSHELL_FULLSCREEN_NORMAL_TYPE = 0,
	NEMOSHELL_FULLSCREEN_PICK_TYPE = 1,
	NEMOSHELL_FULLSCREEN_PITCH_TYPE = 2,
	NEMOSHELL_FULLSCREEN_LAST_TYPE
} NemoShellFullscreenType;

typedef enum {
	NEMOSHELL_FULLSCREEN_NONE_FOCUS = 0,
	NEMOSHELL_FULLSCREEN_ALL_FOCUS = 1,
	NEMOSHELL_FULLSCREEN_LAST_FOCUS
} NemoShellFullscreenFocus;

struct shellbin;
struct clientstate;

typedef void (*nemoshell_destroy_client_t)(void *data, pid_t pid);
typedef void (*nemoshell_update_client_state_t)(void *data, struct shellbin *bin, struct clientstate *state);
typedef void (*nemoshell_enter_idle_t)(void *data);

struct nemoshell {
	struct nemocompz *compz;

	struct nemolayer overlay_layer;
	struct nemolayer fullscreen_layer;
	struct nemolayer service_layer;
	struct nemolayer underlay_layer;
	struct nemolayer background_layer;

	struct nemolayer *default_layer;

	struct wl_listener pointer_focus_listener;
	struct wl_listener keyboard_focus_listener;
	struct wl_listener keypad_focus_listener;
	struct wl_listener touch_focus_listener;

	struct wl_listener child_signal_listener;

	struct wl_listener idle_listener;

	struct wl_event_source *frame_timer;
	uint32_t frame_timeout;

	struct wl_list bin_list;
	struct wl_list fullscreen_list;
	struct wl_list stage_list;
	struct wl_list clientstate_list;

	struct {
		uint32_t samples;
		uint32_t max_duration;

		double friction;
		double coefficient;
	} pitch;

	struct {
		uint32_t flags;

		double rotate_distance;
		double scale_distance;

		double fullscreen_scale;
		double resize_interval;
	} pick;

	struct {
		uint32_t min_width, min_height;
		uint32_t max_width, max_height;
	} bin;

	nemoshell_destroy_client_t destroy_client;
	nemoshell_update_client_state_t update_client_state;
	nemoshell_enter_idle_t enter_idle;
	void *userdata;

	int is_logging_grab;
};

struct shellclient {
	struct wl_resource *resource;
	struct wl_client *client;
	struct nemoshell *shell;
	struct wl_listener destroy_listener;
	struct wl_event_source *ping_timer;
	uint32_t ping_serial;
	int unresponsive;
};

struct clientstate {
	uint32_t pid;

	struct wl_list link;

	struct itemone *one;
};

struct binconfig {
	int maximized;
	int fullscreen;
	int relative;
	int lowered;
};

struct shellbin {
	struct wl_resource *resource;
	struct wl_signal destroy_signal;
	struct shellclient *owner;

	pid_t pid;

	struct wl_signal ungrab_signal;
	struct wl_signal change_signal;
	struct wl_signal resize_signal;

	int type;

	uint32_t flags;
	uint32_t state;

	struct nemoshell *shell;
	struct nemocanvas *canvas;
	struct nemoview *view;
	struct nemolayer *layer;

	int32_t last_width, last_height;
	int32_t last_sx, last_sy;

	struct wl_listener canvas_destroy_listener;

	uint32_t min_width, min_height;
	uint32_t max_width, max_height;

	struct nemocanvas_callback *callback;

	struct wl_list link;

	struct shellbin *parent;
	struct wl_list children_list;
	struct wl_list children_link;

	struct wl_list screen_link;

	struct {
		float x, y;
		uint32_t serial;
	} popup;

	struct {
		float x, y;
		uint32_t flags;
	} transient;

	struct {
		enum wl_shell_surface_fullscreen_method type;
		uint32_t framerate;

		float x, y;
		int32_t width, height;
		float r;
		float px, py;
	} fullscreen;

	struct {
		float x, y;
		float r;
		int32_t width, height;

		struct nemoscreen *overlay;
	} screen;
	int has_screen;

	struct {
		int32_t x, y;
		int32_t width, height;
	} geometry, next_geometry;
	int has_set_geometry, has_next_geometry;

	struct {
		float x, y;
		float r;
		float sx, sy;
		float dx, dy;

		int has_position;
	} initial;

	struct {
		float ax, ay;

		uint32_t serial;

		int32_t width, height;
	} scale;
	int32_t has_scale;

	int32_t reset_pivot;
	int32_t reset_move;

	uint32_t resize_edges;

	uint32_t next_serial;
	uint32_t done_serial;

	struct binconfig config, next_config, requested_config;
	int config_changed;
	int config_requested;
};

struct shellscreen {
	char *id;

	uint32_t type;
	uint32_t focus;
	uint32_t target;

	int fixed;

	int32_t sx, sy, sw, sh;
	int32_t dx, dy, dw, dh, dr;

	uint32_t nodeid, screenid;
	int has_screen;

	struct wl_list link;

	struct wl_list bin_list;
	struct wl_signal kill_signal;
};

struct shellstage {
	char *id;

	struct wl_list link;

	int32_t sx, sy, sw, sh;

	int32_t dr;
};

extern struct nemoshell *nemoshell_create(struct nemocompz *compz);
extern void nemoshell_destroy(struct nemoshell *shell);

extern void nemoshell_set_frame_timeout(struct nemoshell *shell, uint32_t timeout);

extern void nemoshell_set_default_layer(struct nemoshell *shell, struct nemolayer *layer);

extern struct shellbin *nemoshell_create_bin(struct nemoshell *shell, struct nemocanvas *canvas, struct nemocanvas_callback *callback);
extern void nemoshell_destroy_bin(struct shellbin *bin);

extern struct shellbin *nemoshell_get_bin(struct nemocanvas *canvas);
extern void nemoshell_set_parent_bin(struct shellbin *bin, struct shellbin *parent);

extern struct shellbin *nemoshell_get_bin_by_pid(struct nemoshell *shell, uint32_t pid);
extern struct shellbin *nemoshell_get_bin_by_uuid(struct nemoshell *shell, const char *uuid);

extern void nemoshell_ping(struct shellbin *bin, uint32_t serial);
extern void nemoshell_pong(struct shellclient *sc, uint32_t serial);

extern struct shellclient *nemoshell_create_client(struct wl_client *client, struct nemoshell *shell, const struct wl_interface *interface, uint32_t id);
extern void nemoshell_destroy_client(struct shellclient *sc);
extern struct shellclient *nemoshell_get_client(struct wl_client *client);

extern void nemoshell_send_bin_close(struct shellbin *bin);
extern void nemoshell_send_bin_config(struct shellbin *bin);
extern void nemoshell_change_bin_config(struct shellbin *bin);
extern void nemoshell_clear_bin_config(struct shellbin *bin);

extern void nemoshell_send_xdg_config(struct shellbin *bin);

extern struct nemoview *nemoshell_get_default_view(struct nemocanvas *canvas);

extern struct clientstate *nemoshell_create_client_state(struct nemoshell *shell, uint32_t pid);
extern void nemoshell_destroy_client_state(struct nemoshell *shell, struct clientstate *state);
extern struct clientstate *nemoshell_get_client_state(struct nemoshell *shell, uint32_t pid);

extern int nemoshell_use_client_state(struct nemoshell *shell, struct shellbin *bin);
extern int nemoshell_use_client_uuid(struct nemoshell *shell, struct shellbin *bin);
extern int nemoshell_use_client_stage(struct nemoshell *shell, struct shellbin *bin);

extern struct shellscreen *nemoshell_get_fullscreen(struct nemoshell *shell, const char *id);
extern void nemoshell_put_fullscreen(struct nemoshell *shell, const char *id);
extern struct shellscreen *nemoshell_get_fullscreen_on(struct nemoshell *shell, int32_t x, int32_t y, uint32_t type);

extern struct shellstage *nemoshell_get_stage(struct nemoshell *shell, const char *id);
extern void nemoshell_put_stage(struct nemoshell *shell, const char *id);
extern struct shellstage *nemoshell_get_stage_on(struct nemoshell *shell, int32_t x, int32_t y);

extern void nemoshell_set_fullscreen_bin_legacy(struct nemoshell *shell, struct shellbin *bin, struct nemoscreen *screen);
extern void nemoshell_set_fullscreen_bin_overlay(struct nemoshell *shell, struct shellbin *bin, const char *id, int fixed, struct nemoscreen *screen);
extern void nemoshell_set_fullscreen_bin(struct nemoshell *shell, struct shellbin *bin, struct shellscreen *screen);
extern void nemoshell_put_fullscreen_bin(struct nemoshell *shell, struct shellbin *bin);
extern void nemoshell_set_maximized_bin_legacy(struct nemoshell *shell, struct shellbin *bin, struct nemoscreen *screen);
extern void nemoshell_set_maximized_bin(struct nemoshell *shell, struct shellbin *bin, struct shellscreen *screen);
extern void nemoshell_put_maximized_bin(struct nemoshell *shell, struct shellbin *bin);
extern void nemoshell_kill_fullscreen_bin(struct nemoshell *shell, uint32_t target);

static inline void nemoshell_set_destroy_client(struct nemoshell *shell, nemoshell_destroy_client_t dispatch)
{
	shell->destroy_client = dispatch;
}

static inline void nemoshell_set_update_client_state(struct nemoshell *shell, nemoshell_update_client_state_t dispatch)
{
	shell->update_client_state = dispatch;
}

static inline void nemoshell_set_enter_idle(struct nemoshell *shell, nemoshell_enter_idle_t dispatch)
{
	shell->enter_idle = dispatch;
}

static inline void nemoshell_set_userdata(struct nemoshell *shell, void *data)
{
	shell->userdata = data;
}

static inline void nemoshell_bin_set_state(struct shellbin *bin, uint32_t state)
{
	bin->state |= state;
}

static inline void nemoshell_bin_put_state(struct shellbin *bin, uint32_t state)
{
	bin->state &= ~state;
}

static inline int nemoshell_bin_has_state(struct shellbin *bin, uint32_t state)
{
	return bin->state & state;
}

static inline int nemoshell_bin_has_state_all(struct shellbin *bin, uint32_t state)
{
	return (bin->state & state) == state;
}

static inline void nemoshell_bin_set_flags(struct shellbin *bin, uint32_t flags)
{
	bin->flags |= flags;
}

static inline void nemoshell_bin_put_flags(struct shellbin *bin, uint32_t flags)
{
	bin->flags &= ~flags;
}

static inline int nemoshell_bin_has_flags(struct shellbin *bin, uint32_t flags)
{
	return bin->flags & flags;
}

static inline int nemoshell_bin_has_flags_all(struct shellbin *bin, uint32_t flags)
{
	return (bin->flags & flags) == flags;
}

static inline int32_t nemoshell_bin_get_geometry_width(struct shellbin *bin)
{
	return bin->has_set_geometry == 0 ? bin->view->content->width : bin->geometry.width;
}

static inline int32_t nemoshell_bin_get_geometry_height(struct shellbin *bin)
{
	return bin->has_set_geometry == 0 ? bin->view->content->height : bin->geometry.height;
}

static inline void clientstate_set_pid(struct clientstate *state, uint32_t pid)
{
	state->pid = pid;
}

static inline void clientstate_set_one(struct clientstate *state, struct itemone *one)
{
	nemoitem_one_copy(state->one, one);
}

static inline void clientstate_set_sattr(struct clientstate *state, const char *name, const char *value)
{
	nemoitem_one_set_attr(state->one, name, value);
}

static inline void clientstate_set_fattr(struct clientstate *state, const char *name, float value)
{
	nemoitem_one_set_attr_format(state->one, name, "%f", value);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
