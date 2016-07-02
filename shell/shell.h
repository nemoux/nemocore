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
	NEMOSHELL_SURFACE_SCALABLE_FLAG = (1 << 1),
	NEMOSHELL_SURFACE_PICKABLE_FLAG = (1 << 2),
	NEMOSHELL_SURFACE_MAXIMIZABLE_FLAG = (1 << 3),
	NEMOSHELL_SURFACE_MINIMIZABLE_FLAG = (1 << 4),
	NEMOSHELL_SURFACE_BINDABLE_FLAG = (1 << 5),
	NEMOSHELL_SURFACE_ALL_FLAGS = NEMOSHELL_SURFACE_MOVABLE_FLAG | NEMOSHELL_SURFACE_SCALABLE_FLAG | NEMOSHELL_SURFACE_PICKABLE_FLAG | NEMOSHELL_SURFACE_MAXIMIZABLE_FLAG | NEMOSHELL_SURFACE_MINIMIZABLE_FLAG,
} NemoShellSurfaceFlag;

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

typedef int (*nemoshell_execute_command_t)(void *data, struct shellbin *bin, const char *name, const char *cmds, double x, double y, double r);
typedef int (*nemoshell_execute_action_t)(void *data, struct shellbin *bin, const char *id, double x, double y, double r);
typedef int (*nemoshell_execute_content_t)(void *data, struct shellbin *bin, uint32_t type, const char *path, double x, double y, double r);

typedef void (*nemoshell_transform_bin_t)(void *data, struct shellbin *bin);

typedef void (*nemoshell_destroy_client_t)(void *data, pid_t pid);
typedef void (*nemoshell_update_pointer_t)(void *data, struct nemopointer *pointer);

struct nemoshell {
	struct nemocompz *compz;

	struct nemolayer overlay_layer;
	struct nemolayer fullscreen_layer;
	struct nemolayer service_layer;
	struct nemolayer underlay_layer;
	struct nemolayer background_layer;

	struct nemolayer *default_layer;

	struct wl_list bin_list;
	uint32_t bin_ids;

	struct wl_listener pointer_focus_listener;
	struct wl_listener keyboard_focus_listener;
	struct wl_listener keypad_focus_listener;
	struct wl_listener touch_focus_listener;

	struct wl_listener child_signal_listener;
	struct wl_listener pointer_sprite_listener;

	struct wl_list fullscreen_list;

	struct wl_list clientstate_list;

	struct {
		uint32_t samples;
		uint32_t max_duration;

		double friction;
		double coefficient;
	} pitch;

	struct {
		double rotate_distance;
		double scale_distance;

		double fullscreen_scale;
		double resize_interval;
	} pick;

	struct {
		uint32_t min_width, min_height;
		uint32_t max_width, max_height;
	} bin;

	nemoshell_execute_command_t execute_command;
	nemoshell_execute_action_t execute_action;
	nemoshell_execute_content_t execute_content;
	nemoshell_transform_bin_t transform_bin;
	nemoshell_destroy_client_t destroy_client;
	nemoshell_update_pointer_t update_pointer;
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

	float x, y;
	float r;
	int32_t width, height;

	float dx, dy;

	int is_maximized;
	int is_fullscreen;

	uint32_t min_width, min_height;
	uint32_t max_width, max_height;
	int has_min_size, has_max_size;

	int has_pickscreen;
	int has_pitchscreen;

	uint32_t flags;

	uint32_t state_on;
	uint32_t state_off;
};

struct binstate {
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

	struct nemoshell *shell;
	struct nemocanvas *canvas;
	struct nemoview *view;
	struct nemolayer *layer;

	int32_t last_width, last_height;
	int32_t last_sx, last_sy;

	struct wl_listener canvas_destroy_listener;

	uint32_t min_width, min_height;
	uint32_t max_width, max_height;

	struct nemoclient *client;

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

	int on_pickscreen;
	int on_pitchscreen;

	struct {
		float x, y;
		float r;
		int32_t width, height;
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
		float dx, dy;
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

	struct binstate state, next_state, requested_state;
	int state_changed;
	int state_requested;

	int grabbed;
	int fixed;
};

struct shellscreen {
	char *id;

	uint32_t type;
	uint32_t focus;

	int fixed;

	int32_t sx, sy, sw, sh;
	int32_t dx, dy, dw, dh, dr;

	struct wl_list link;

	struct wl_list bin_list;
};

extern struct nemoshell *nemoshell_create(struct nemocompz *compz);
extern void nemoshell_destroy(struct nemoshell *shell);

extern void nemoshell_set_default_layer(struct nemoshell *shell, struct nemolayer *layer);

extern struct shellbin *nemoshell_create_bin(struct nemoshell *shell, struct nemocanvas *canvas, struct nemoclient *client);
extern void nemoshell_destroy_bin(struct shellbin *bin);

extern struct shellbin *nemoshell_get_bin(struct nemocanvas *canvas);
extern void nemoshell_set_parent_bin(struct shellbin *bin, struct shellbin *parent);
extern struct shellbin *nemoshell_get_bin_by_id(struct nemoshell *shell, uint32_t id);

extern void nemoshell_ping(struct shellbin *bin, uint32_t serial);
extern void nemoshell_pong(struct shellclient *sc, uint32_t serial);

extern struct shellclient *nemoshell_create_client(struct wl_client *client, struct nemoshell *shell, const struct wl_interface *interface, uint32_t id);
extern void nemoshell_destroy_client(struct shellclient *sc);
extern struct shellclient *nemoshell_get_client(struct wl_client *client);

extern void nemoshell_send_bin_close(struct shellbin *bin);
extern void nemoshell_send_bin_state(struct shellbin *bin);
extern void nemoshell_change_bin_state(struct shellbin *bin);
extern void nemoshell_clear_bin_state(struct shellbin *bin);

extern void nemoshell_send_xdg_state(struct shellbin *bin);

extern struct nemoview *nemoshell_get_default_view(struct nemocanvas *canvas);

extern struct clientstate *nemoshell_create_client_state(struct nemoshell *shell, uint32_t pid);
extern void nemoshell_destroy_client_state(struct nemoshell *shell, struct clientstate *state);
extern struct clientstate *nemoshell_get_client_state(struct nemoshell *shell, uint32_t pid);

extern int nemoshell_use_client_state(struct nemoshell *shell, struct shellbin *bin);

extern struct shellscreen *nemoshell_get_fullscreen(struct nemoshell *shell, const char *id);
extern void nemoshell_put_fullscreen(struct nemoshell *shell, const char *id);

extern struct shellscreen *nemoshell_get_fullscreen_on(struct nemoshell *shell, int32_t x, int32_t y, uint32_t type);

extern void nemoshell_set_toplevel_bin(struct nemoshell *shell, struct shellbin *bin);
extern void nemoshell_set_popup_bin(struct nemoshell *shell, struct shellbin *bin, struct shellbin *parent, int32_t x, int32_t y, uint32_t serial);
extern void nemoshell_set_fullscreen_bin_on_screen(struct nemoshell *shell, struct shellbin *bin, struct nemoscreen *screen);
extern void nemoshell_set_fullscreen_bin(struct nemoshell *shell, struct shellbin *bin, struct shellscreen *screen);
extern void nemoshell_put_fullscreen_bin(struct nemoshell *shell, struct shellbin *bin);
extern void nemoshell_set_maximized_bin_on_screen(struct nemoshell *shell, struct shellbin *bin, struct nemoscreen *screen);
extern void nemoshell_set_maximized_bin(struct nemoshell *shell, struct shellbin *bin, struct shellscreen *screen);
extern void nemoshell_put_maximized_bin(struct nemoshell *shell, struct shellbin *bin);

static inline void nemoshell_set_execute_command(struct nemoshell *shell, nemoshell_execute_command_t execute)
{
	shell->execute_command = execute;
}

static inline void nemoshell_set_execute_action(struct nemoshell *shell, nemoshell_execute_action_t execute)
{
	shell->execute_action = execute;
}

static inline void nemoshell_set_execute_content(struct nemoshell *shell, nemoshell_execute_content_t execute)
{
	shell->execute_content = execute;
}

static inline void nemoshell_set_transform_bin(struct nemoshell *shell, nemoshell_transform_bin_t dispatch)
{
	shell->transform_bin = dispatch;
}

static inline void nemoshell_set_destroy_client(struct nemoshell *shell, nemoshell_destroy_client_t dispatch)
{
	shell->destroy_client = dispatch;
}

static inline void nemoshell_set_update_pointer(struct nemoshell *shell, nemoshell_update_pointer_t dispatch)
{
	shell->update_pointer = dispatch;
}

static inline void nemoshell_set_userdata(struct nemoshell *shell, void *data)
{
	shell->userdata = data;
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

static inline void clientstate_set_position(struct clientstate *state, float x, float y)
{
	state->x = x;
	state->y = y;
}

static inline void clientstate_set_rotate(struct clientstate *state, float r)
{
	state->r = r;
}

static inline void clientstate_set_anchor(struct clientstate *state, float dx, float dy)
{
	state->dx = dx;
	state->dy = dy;
}

static inline void clientstate_set_size(struct clientstate *state, uint32_t width, uint32_t height)
{
	state->width = width;
	state->height = height;
}

static inline void clientstate_set_max_size(struct clientstate *state, uint32_t width, uint32_t height)
{
	state->max_width = width;
	state->max_height = height;
	state->has_max_size = 1;
}

static inline void clientstate_set_min_size(struct clientstate *state, uint32_t width, uint32_t height)
{
	state->min_width = width;
	state->min_height = height;
	state->has_min_size = 1;
}

static inline void clientstate_set_pickscreen(struct clientstate *state, int pickscreen)
{
	state->has_pickscreen = pickscreen;
}

static inline void clientstate_set_pitchscreen(struct clientstate *state, int pitchscreen)
{
	state->has_pitchscreen = pitchscreen;
}

static inline void clientstate_set_bin_flags(struct clientstate *state, uint32_t flags)
{
	state->flags = flags;
}

static inline void clientstate_set_view_state(struct clientstate *state, uint32_t _state)
{
	state->state_on |= _state;
}

static inline void clientstate_put_view_state(struct clientstate *state, uint32_t _state)
{
	state->state_off |= _state;
}

static inline void clientstate_set_maximized(struct clientstate *state, int is_maximized)
{
	state->is_maximized = is_maximized;
}

static inline void clientstate_set_fullscreen(struct clientstate *state, int is_fullscreen)
{
	state->is_fullscreen = is_fullscreen;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
