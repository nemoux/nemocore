#ifndef	__NEMO_XWAYLAND_WINDOW_MANAGER_H__
#define	__NEMO_XWAYLAND_WINDOW_MANAGER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <hashhelper.h>

struct wm_size_hints {
	uint32_t flags;
	int32_t x, y;
	int32_t width, height;
	int32_t min_width, min_height;
	int32_t max_width, max_height;
	int32_t width_inc, height_inc;
	struct {
		int32_t x;
		int32_t y;
	} min_aspect, max_aspect;
	int32_t base_width, base_height;
	int32_t win_gravity;
};

#define USPosition	(1L << 0)
#define USSize			(1L << 1)
#define PPosition		(1L << 2)
#define PSize				(1L << 3)
#define PMinSize		(1L << 4)
#define PMaxSize		(1L << 5)
#define PResizeInc	(1L << 6)
#define PAspect			(1L << 7)
#define PBaseSize		(1L << 8)
#define PWinGravity	(1L << 9)

struct motif_wm_hints {
	uint32_t flags;
	uint32_t functions;
	uint32_t decorations;
	int32_t input_mode;
	uint32_t status;
};

#define MWM_HINTS_FUNCTIONS     (1L << 0)
#define MWM_HINTS_DECORATIONS   (1L << 1)
#define MWM_HINTS_INPUT_MODE    (1L << 2)
#define MWM_HINTS_STATUS        (1L << 3)

#define MWM_FUNC_ALL            (1L << 0)
#define MWM_FUNC_RESIZE         (1L << 1)
#define MWM_FUNC_MOVE           (1L << 2)
#define MWM_FUNC_MINIMIZE       (1L << 3)
#define MWM_FUNC_MAXIMIZE       (1L << 4)
#define MWM_FUNC_CLOSE          (1L << 5)

#define MWM_DECOR_ALL           (1L << 0)
#define MWM_DECOR_BORDER        (1L << 1)
#define MWM_DECOR_RESIZEH       (1L << 2)
#define MWM_DECOR_TITLE         (1L << 3)
#define MWM_DECOR_MENU          (1L << 4)
#define MWM_DECOR_MINIMIZE      (1L << 5)
#define MWM_DECOR_MAXIMIZE      (1L << 6)

#define MWM_INPUT_MODELESS 0
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL 1
#define MWM_INPUT_SYSTEM_MODAL 2
#define MWM_INPUT_FULL_APPLICATION_MODAL 3
#define MWM_INPUT_APPLICATION_MODAL MWM_INPUT_PRIMARY_APPLICATION_MODAL

#define MWM_TEAROFF_WINDOW      (1L<<0)

#define _NET_WM_MOVERESIZE_SIZE_TOPLEFT      0
#define _NET_WM_MOVERESIZE_SIZE_TOP          1
#define _NET_WM_MOVERESIZE_SIZE_TOPRIGHT     2
#define _NET_WM_MOVERESIZE_SIZE_RIGHT        3
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT  4
#define _NET_WM_MOVERESIZE_SIZE_BOTTOM       5
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT   6
#define _NET_WM_MOVERESIZE_SIZE_LEFT         7
#define _NET_WM_MOVERESIZE_MOVE              8
#define _NET_WM_MOVERESIZE_SIZE_KEYBOARD     9
#define _NET_WM_MOVERESIZE_MOVE_KEYBOARD    10
#define _NET_WM_MOVERESIZE_CANCEL           11

#define SEND_EVENT_MASK			(0x80)
#define EVENT_TYPE(event)		((event)->response_type & ~SEND_EVENT_MASK)

struct nemoxserver;
struct nemoxmanager;
struct nemocanvas;
struct shellbin;
struct nemoview;

struct nemoxwindow {
	struct nemoxmanager *xmanager;
	xcb_window_t id;
	xcb_window_t frame_id;

	uint32_t canvas_id;
	struct nemocanvas *canvas;
	struct shellbin *bin;

	struct wl_list link;

	struct wl_listener canvas_destroy_listener;

	struct wl_event_source *configure_source;

	int properties_dirty;
	int pid;
	char *machine;
	char *class;
	char *name;
	struct nemoxwindow *transient_for;
	uint32_t protocols;
	xcb_atom_t type;

	int width, height;
	int x, y;
	int saved_width, saved_height;
	int decorate;
	int override_redirect;
	int fullscreen;
	int maximized_vert;
	int maximized_horz;
	int delete_window;
	int has_alpha;

	struct wm_size_hints size_hints;
	struct motif_wm_hints motif_hints;
};

struct nemoxmanager {
	struct nemoxserver *xserver;

	struct hash *window_table;

	struct wl_list unpaired_list;

	xcb_connection_t *conn;
	xcb_screen_t *screen;

	struct wl_event_source *source;

	struct wl_listener selection_listener;

	struct wl_listener create_surface_listener;
	struct wl_listener activate_listener;
	struct wl_listener transform_listener;

	struct nemoxwindow *focus;

	const xcb_query_extension_reply_t *xfixes;

	xcb_cursor_t *cursors;
	int last_cursor;

	xcb_window_t selection_window;
	xcb_window_t selection_owner;

	xcb_selection_request_event_t selection_request;
	xcb_atom_t selection_target;
	xcb_timestamp_t selection_timestamp;

	xcb_window_t window;
	xcb_render_pictforminfo_t format_rgb, format_rgba;
	xcb_visualid_t visual_id;
	xcb_colormap_t colormap;

	struct {
		xcb_atom_t wm_protocols;
		xcb_atom_t wm_normal_hints;
		xcb_atom_t wm_take_focus;
		xcb_atom_t wm_delete_window;
		xcb_atom_t wm_state;
		xcb_atom_t wm_s0;
		xcb_atom_t wm_client_machine;
		xcb_atom_t net_wm_cm_s0;
		xcb_atom_t net_wm_name;
		xcb_atom_t net_wm_pid;
		xcb_atom_t net_wm_icon;
		xcb_atom_t net_wm_state;
		xcb_atom_t net_wm_state_maximized_vert;
		xcb_atom_t net_wm_state_maximized_horz;
		xcb_atom_t net_wm_state_fullscreen;
		xcb_atom_t net_wm_user_time;
		xcb_atom_t net_wm_icon_name;
		xcb_atom_t net_wm_desktop;
		xcb_atom_t net_wm_window_type;
		xcb_atom_t net_wm_window_type_desktop;
		xcb_atom_t net_wm_window_type_dock;
		xcb_atom_t net_wm_window_type_toolbar;
		xcb_atom_t net_wm_window_type_menu;
		xcb_atom_t net_wm_window_type_utility;
		xcb_atom_t net_wm_window_type_splash;
		xcb_atom_t net_wm_window_type_dialog;
		xcb_atom_t net_wm_window_type_dropdown;
		xcb_atom_t net_wm_window_type_popup;
		xcb_atom_t net_wm_window_type_tooltip;
		xcb_atom_t net_wm_window_type_notification;
		xcb_atom_t net_wm_window_type_combo;
		xcb_atom_t net_wm_window_type_dnd;
		xcb_atom_t net_wm_window_type_normal;
		xcb_atom_t net_wm_moveresize;
		xcb_atom_t net_supporting_wm_check;
		xcb_atom_t net_supported;
		xcb_atom_t net_active_window;
		xcb_atom_t motif_wm_hints;
		xcb_atom_t clipboard;
		xcb_atom_t clipboard_manager;
		xcb_atom_t targets;
		xcb_atom_t utf8_string;
		xcb_atom_t wl_selection;
		xcb_atom_t incr;
		xcb_atom_t timestamp;
		xcb_atom_t multiple;
		xcb_atom_t compound_text;
		xcb_atom_t text;
		xcb_atom_t string;
		xcb_atom_t window;
		xcb_atom_t text_plain_utf8;
		xcb_atom_t text_plain;
		xcb_atom_t xdnd_selection;
		xcb_atom_t xdnd_aware;
		xcb_atom_t xdnd_enter;
		xcb_atom_t xdnd_leave;
		xcb_atom_t xdnd_drop;
		xcb_atom_t xdnd_status;
		xcb_atom_t xdnd_finished;
		xcb_atom_t xdnd_type_list;
		xcb_atom_t xdnd_action_copy;
		xcb_atom_t wl_surface_id;
	} atom;
};

extern struct nemoxmanager *nemoxmanager_create(struct nemoxserver *xserver, int fd);
extern void nemoxmanager_destroy(struct nemoxmanager *xmanager);

extern void nemoxmanager_repaint_window(struct nemoxwindow *xwindow);
extern void nemoxmanager_map_window(struct nemoxmanager *xmanager, struct nemoxwindow *xwindow);

extern void nemoxmanager_setup_view(struct nemoview *view, int32_t sx, int32_t sy);

extern void nemoxmanager_read_properties(struct nemoxwindow *xwindow);

extern void nemoxmanager_add_window(struct nemoxmanager *xmanager, uint32_t id, struct nemoxwindow *xwindow);
extern void nemoxmanager_del_window(struct nemoxmanager *xmanager, uint32_t id);
extern struct nemoxwindow *nemoxmanager_get_window(struct nemoxmanager *xmanager, uint32_t id);
extern struct nemoxwindow *nemoxmanager_get_canvas_window(struct nemocanvas *canvas);
extern void nemoxmanager_map_canvas(struct nemoxwindow *xwindow, struct nemocanvas *canvas);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
