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
	NEMO_SHELL_SURFACE_PICKABLE_FLAG = (1 << 2),
	NEMO_SHELL_SURFACE_MAXIMIZABLE_FLAG = (1 << 3),
	NEMO_SHELL_SURFACE_MINIMIZABLE_FLAG = (1 << 4),
	NEMO_SHELL_SURFACE_ALL_FLAGS = NEMO_SHELL_SURFACE_MOVABLE_FLAG | NEMO_SHELL_SURFACE_RESIZABLE_FLAG | NEMO_SHELL_SURFACE_PICKABLE_FLAG | NEMO_SHELL_SURFACE_MAXIMIZABLE_FLAG | NEMO_SHELL_SURFACE_MINIMIZABLE_FLAG,
} NemoShellSurfaceFlag;

struct nemoshell {
	struct nemocompz *compz;

	struct nemolayer overlay_layer;
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

	struct wl_list fullscreen_list;

	struct {
		struct nemocanvas *canvas;
		pixman_box32_t cursor;
	} textinput;

	struct {
		struct wl_resource *binding;
		struct wl_list bin_list;
	} inputpanel;

	struct nemoitem *configs;
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
	struct wl_listener destroy_listener;

	float x, y;
	float r;
	int32_t width, height;

	float dx, dy;

	int is_maximized;
	int is_fullscreen;

	uint32_t flags;
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

	uint32_t resize_edges;
	uint32_t reset_scale;

	uint32_t min_width, min_height;
	uint32_t max_width, max_height;

	struct nemoclient *client;

	struct shellbin *parent;
	struct wl_list children_list;
	struct wl_list children_link;

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

	struct {
		float x, y;
		int32_t width, height;
	} screen;
	int has_screen;

	struct {
		float x, y;
		int32_t width, height;
	} geometry, next_geometry;
	int has_set_geometry, has_next_geometry;

	struct binstate state, next_state, requested_state;
	int state_changed;
	int state_requested;

	int grabbed;
};

struct shellscreen {
	uint32_t id;

	int32_t sx, sy, sw, sh;
	int32_t dx, dy, dw, dh;

	struct wl_list link;
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

extern struct clientstate *nemoshell_create_client_state(struct wl_client *client);
extern void nemoshell_destroy_client_state(struct clientstate *state);
extern struct clientstate *nemoshell_get_client_state(struct wl_client *client);
extern void nemoshell_set_client_state(struct shellbin *bin, struct clientstate *state);

extern void nemoshell_load_fullscreens(struct nemoshell *shell);
extern struct shellscreen *nemoshell_get_fullscreen(struct nemoshell *shell, uint32_t id);
extern struct shellscreen *nemoshell_get_fullscreen_on(struct nemoshell *shell, int32_t x, int32_t y);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
