#ifndef	__NEMOSHELL_H__
#define	__NEMOSHELL_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <compz.h>
#include <pixman.h>
#include <layer.h>
#include <nemoitem.h>

typedef enum {
	NEMO_SHELL_SURFACE_NONE_TYPE = 0,
	NEMO_SHELL_SURFACE_NORMAL_TYPE = 1,
	NEMO_SHELL_SURFACE_POPUP_TYPE = 2,
	NEMO_SHELL_SURFACE_OVERLAY_TYPE = 3,
	NEMO_SHELL_SURFACE_XWAYLAND_TYPE = 4,
	NEMO_SHELL_SURFACE_LAST_TYPE
} NemoShellSurfaceType;

typedef enum {
	NEMO_SHELL_SURFACE_MOVABLE_FLAG = (1 << 0),
	NEMO_SHELL_SURFACE_RESIZABLE_FLAG = (1 << 1),
	NEMO_SHELL_SURFACE_SCALABLE_FLAG = (1 << 2),
	NEMO_SHELL_SURFACE_PICKABLE_FLAG = (1 << 3),
	NEMO_SHELL_SURFACE_MAXIMIZABLE_FLAG = (1 << 4),
	NEMO_SHELL_SURFACE_MINIMIZABLE_FLAG = (1 << 5),
	NEMO_SHELL_SURFACE_BINDABLE_FLAG = (1 << 6),
	NEMO_SHELL_SURFACE_ALL_FLAGS = NEMO_SHELL_SURFACE_MOVABLE_FLAG | NEMO_SHELL_SURFACE_RESIZABLE_FLAG | NEMO_SHELL_SURFACE_SCALABLE_FLAG | NEMO_SHELL_SURFACE_PICKABLE_FLAG | NEMO_SHELL_SURFACE_MAXIMIZABLE_FLAG | NEMO_SHELL_SURFACE_MINIMIZABLE_FLAG,
} NemoShellSurfaceFlag;

typedef enum {
	NEMO_SHELL_FULLSCREEN_NORMAL_TYPE = 0,
	NEMO_SHELL_FULLSCREEN_PICK_TYPE = 1,
	NEMO_SHELL_FULLSCREEN_PITCH_TYPE = 2,
	NEMO_SHELL_FULLSCREEN_LAST_TYPE
} NemoShellFullscreenType;

typedef enum {
	NEMO_SHELL_FULLSCREEN_NONE_FOCUS = 0,
	NEMO_SHELL_FULLSCREEN_ALL_FOCUS = 1,
	NEMO_SHELL_FULLSCREEN_LAST_FOCUS
} NemoShellFullscreenFocus;

typedef enum {
	NEMO_SHELL_FADEIN_ALPHA_FLAG = (1 << 0),
	NEMO_SHELL_FADEIN_SCALE_FLAG = (1 << 1)
} NemoShellFadeinFlag;

struct shellbin;

typedef int (*nemoshell_execute_command_t)(void *data, struct shellbin *bin, const char *name, const char *cmds, uint32_t type, double x, double y, double r);
typedef int (*nemoshell_execute_action_t)(void *data, struct shellbin *bin, uint32_t group, uint32_t action, uint32_t type, double x, double y, double r);
typedef int (*nemoshell_execute_content_t)(void *data, struct shellbin *bin, uint32_t type, const char *path, double x, double y, double r);

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

	struct wl_listener show_input_panel_listener;
	struct wl_listener hide_input_panel_listener;
	struct wl_listener update_input_panel_listener;
	int showing_input_panels;

	struct wl_listener child_signal_listener;

	struct wl_list fullscreen_list;

	struct wl_list clientstate_list;

	struct {
		struct nemocanvas *canvas;
		pixman_box32_t cursor;
	} textinput;

	struct {
		struct wl_resource *binding;
		struct wl_list bin_list;
	} inputpanel;

	struct {
		uint32_t samples;

		double friction;
		double coefficient;
	} pitch;

	struct {
		double max_rotate;
		double max_scale;
		double fullscreen_scale;

		double resize_interval;

		double rotate_degree;
		double scale_degree;

		double min_distance;
	} pick;

	struct {
		uint32_t min_width, min_height;
		uint32_t max_width, max_height;
	} bin;

	struct nemoitem *configs;

	nemoshell_execute_command_t execute_command;
	nemoshell_execute_action_t execute_action;
	nemoshell_execute_content_t execute_content;
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

	uint32_t flags;

	uint32_t input_type;

	uint32_t fadein_type;
	uint32_t fadein_ease;
	uint32_t fadein_delay;
	uint32_t fadein_duration;
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
	struct wl_listener fullscreen_opaque_listener;

	uint32_t resize_edges;
	uint32_t reset_scale;
	float px, py;
	float ax, ay;

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
	} fullscreen;

	int on_pickscreen;
	int on_pitchscreen;
	int on_opaquescreen;

	struct {
		float x, y;
		float r;
		int32_t width, height;
	} screen;
	int has_screen;

	struct {
		float x, y;
		float r;
		float dx, dy;
		int32_t width, height;
	} geometry, next_geometry;
	int has_set_geometry, has_next_geometry;

	struct binstate state, next_state, requested_state;
	int state_changed;
	int state_requested;

	int grabbed;
	int fixed;
};

struct shellscreen {
	uint32_t id;
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

extern void nemoshell_ping(struct shellbin *bin, uint32_t serial);
extern void nemoshell_pong(struct shellclient *sc, uint32_t serial);

extern struct shellclient *nemoshell_create_client(struct wl_client *client, struct nemoshell *shell, const struct wl_interface *interface, uint32_t id);
extern void nemoshell_destroy_client(struct shellclient *sc);
extern struct shellclient *nemoshell_get_client(struct wl_client *client);

extern void nemoshell_send_bin_state(struct shellbin *bin);
extern void nemoshell_send_bin_configure(struct shellbin *bin);
extern void nemoshell_change_bin_next_state(struct shellbin *bin);
extern void nemoshell_clear_bin_next_state(struct shellbin *bin);

extern struct nemoview *nemoshell_get_default_view(struct nemocanvas *canvas);

extern struct clientstate *nemoshell_create_client_state(struct nemoshell *shell, uint32_t pid);
extern void nemoshell_destroy_client_state(struct nemoshell *shell, struct clientstate *state);
extern struct clientstate *nemoshell_get_client_state(struct nemoshell *shell, uint32_t pid);

extern int nemoshell_use_client_state(struct nemoshell *shell, struct shellbin *bin, struct wl_client *client);
extern int nemoshell_use_client_state_by_pid(struct nemoshell *shell, struct shellbin *bin, pid_t pid);

extern void nemoshell_load_fullscreens(struct nemoshell *shell);
extern struct shellscreen *nemoshell_get_fullscreen(struct nemoshell *shell, uint32_t id);
extern struct shellscreen *nemoshell_get_fullscreen_on(struct nemoshell *shell, int32_t x, int32_t y, uint32_t type);

extern void nemoshell_set_toplevel_bin(struct nemoshell *shell, struct shellbin *bin);
extern void nemoshell_set_popup_bin(struct nemoshell *shell, struct shellbin *bin, struct shellbin *parent, int32_t x, int32_t y, uint32_t serial);
extern void nemoshell_set_fullscreen_bin_on_screen(struct nemoshell *shell, struct shellbin *bin, struct nemoscreen *screen);
extern void nemoshell_set_fullscreen_bin(struct nemoshell *shell, struct shellbin *bin, struct shellscreen *screen);
extern void nemoshell_put_fullscreen_bin(struct nemoshell *shell, struct shellbin *bin);
extern void nemoshell_set_fullscreen_opaque(struct nemoshell *shell, struct shellbin *bin);
extern void nemoshell_put_fullscreen_opaque(struct nemoshell *shell, struct shellbin *bin);
extern void nemoshell_set_maximized_bin_on_screen(struct nemoshell *shell, struct shellbin *bin, struct nemoscreen *screen);
extern void nemoshell_set_maximized_bin(struct nemoshell *shell, struct shellbin *bin, struct shellscreen *screen);
extern void nemoshell_put_maximized_bin(struct nemoshell *shell, struct shellbin *bin);

extern void nemoshell_load_gestures(struct nemoshell *shell);

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

static inline void nemoshell_set_userdata(struct nemoshell *shell, void *data)
{
	shell->userdata = data;
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

static inline void clientstate_set_bin_flags(struct clientstate *state, uint32_t flags)
{
	state->flags = flags;
}

static inline void clientstate_set_input_type(struct clientstate *state, uint32_t type)
{
	state->input_type = type;
}

static inline void clientstate_set_fadein_style(struct clientstate *state, uint32_t type, uint32_t ease, uint32_t delay, uint32_t duration)
{
	state->fadein_type = type;
	state->fadein_ease = ease;
	state->fadein_delay = delay;
	state->fadein_duration = duration;
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
