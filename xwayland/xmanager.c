#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <wayland-server.h>

#include <compz.h>
#include <shell.h>
#include <canvas.h>
#include <view.h>
#include <xserver.h>
#include <xmanager.h>
#include <selection.h>
#include <dnd.h>
#include <cursor.h>
#include <hashhelper.h>
#include <nemomisc.h>
#include <nemolog.h>

#define ICCCM_WITHDRAWN_STATE		0
#define ICCCM_NORMAL_STATE			1
#define ICCCM_ICONIC_STATE			3

#define _NET_WM_STATE_REMOVE		0
#define _NET_WM_STATE_ADD				1
#define _NET_WM_STATE_TOGGLE		2

static int nemoxmanager_update_state(int action, int *state)
{
	int new_state, changed;

	switch (action) {
		case _NET_WM_STATE_REMOVE:
			new_state = 0;
			break;

		case _NET_WM_STATE_ADD:
			new_state = 1;
			break;

		case _NET_WM_STATE_TOGGLE:
			new_state = !*state;
			break;

		default:
			return 0;
	}

	changed = (*state != new_state);
	*state = new_state;

	return changed;
}

static void nemoxmanager_set_window_manager_state(struct nemoxwindow *xwindow, int32_t state)
{
	struct nemoxmanager *xmanager = xwindow->xmanager;
	uint32_t property[2];

	property[0] = state;
	property[1] = XCB_WINDOW_NONE;

	xcb_change_property(xmanager->conn,
			XCB_PROP_MODE_REPLACE,
			xwindow->id,
			xmanager->atom.wm_state,
			xmanager->atom.wm_state,
			32,
			2,
			property);
}

static void nemoxmanager_set_net_window_manager_state(struct nemoxwindow *xwindow)
{
	struct nemoxmanager *xmanager = xwindow->xmanager;
	uint32_t property[3];
	int i;

	i = 0;

	if (xwindow->fullscreen)
		property[i++] = xmanager->atom.net_wm_state_fullscreen;
	if (xwindow->maximized_vert)
		property[i++] = xmanager->atom.net_wm_state_maximized_vert;
	if (xwindow->maximized_horz)
		property[i++] = xmanager->atom.net_wm_state_maximized_horz;

	xcb_change_property(xmanager->conn,
			XCB_PROP_MODE_REPLACE,
			xwindow->id,
			xmanager->atom.net_wm_state,
			XCB_ATOM_ATOM,
			32,
			i,
			property);
}

static void nemoxmanager_set_net_window_virtual_desktop(struct nemoxwindow *xwindow, int desktop)
{
	struct nemoxmanager *xmanager = xwindow->xmanager;

	if (desktop >= 0) {
		xcb_change_property(xmanager->conn,
				XCB_PROP_MODE_REPLACE,
				xwindow->id,
				xmanager->atom.net_wm_desktop,
				XCB_ATOM_CARDINAL,
				32,
				1,
				&desktop);
	} else {
		xcb_delete_property(xmanager->conn,
				xwindow->id,
				xmanager->atom.net_wm_desktop);
	}
}

static void nemoxmanager_set_net_active_window(struct nemoxmanager *xmanager, xcb_window_t window)
{
	xcb_change_property(xmanager->conn,
			XCB_PROP_MODE_REPLACE,
			xmanager->screen->root,
			xmanager->atom.net_active_window,
			xmanager->atom.window,
			32, 1,
			&window);
}

static void nemoxmanager_send_focus_window(struct nemoxmanager *xmanager, struct nemoxwindow *xwindow)
{
	xcb_client_message_event_t message;

	if (xwindow != NULL) {
		uint32_t values[1];

		message.response_type = XCB_CLIENT_MESSAGE;
		message.format = 32;
		message.window = xwindow->id;
		message.type = xmanager->atom.wm_protocols;
		message.data.data32[0] = xmanager->atom.wm_take_focus;
		message.data.data32[1] = XCB_TIME_CURRENT_TIME;

		xcb_send_event(xmanager->conn, 0, xwindow->id, XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT, (char *)&message);

		xcb_set_input_focus(xmanager->conn, XCB_INPUT_FOCUS_POINTER_ROOT, xwindow->id, XCB_TIME_CURRENT_TIME);

		values[0] = XCB_STACK_MODE_ABOVE;
		xcb_configure_window(xmanager->conn, xwindow->id, XCB_CONFIG_WINDOW_STACK_MODE, values);
	}
}

static int nemoxmanager_our_resource(struct nemoxmanager *xmanager, uint32_t id)
{
	const xcb_setup_t *setup;

	setup = xcb_get_setup(xmanager->conn);

	return (id & ~setup->resource_id_mask) == setup->resource_id_base;
}

static void nemoxmanager_create_window(struct nemoxmanager *xmanager, xcb_window_t id, int width, int height, int x, int y, int override)
{
	struct nemoxwindow *xwindow;
	uint32_t values[1];
	xcb_get_geometry_cookie_t geometry_cookie;
	xcb_get_geometry_reply_t *geometry_reply;

	xwindow = (struct nemoxwindow *)malloc(sizeof(struct nemoxwindow));
	if (xwindow == NULL) {
		nemolog_error("XWAYLAND", "failed to allocate xwindow");
		return;
	}
	memset(xwindow, 0, sizeof(struct nemoxwindow));

	wl_list_init(&xwindow->link);
	wl_list_init(&xwindow->canvas_destroy_listener.link);

	geometry_cookie = xcb_get_geometry(xmanager->conn, id);

	values[0] = XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_FOCUS_CHANGE;
	xcb_change_window_attributes(xmanager->conn, id, XCB_CW_EVENT_MASK, values);

	xwindow->xmanager = xmanager;
	xwindow->id = id;
	xwindow->properties_dirty = 1;
	xwindow->override_redirect = override;
	xwindow->width = width;
	xwindow->height = height;
	xwindow->x = x;
	xwindow->y = y;

	geometry_reply = xcb_get_geometry_reply(xmanager->conn, geometry_cookie, NULL);
	if (geometry_reply != NULL)
		xwindow->has_alpha = geometry_reply->depth == 32;

	free(geometry_reply);

	nemoxmanager_add_window(xmanager, id, xwindow);
}

static void nemoxmanager_destroy_window(struct nemoxwindow *xwindow)
{
	struct nemoxmanager *xmanager = xwindow->xmanager;

	wl_list_remove(&xwindow->link);
	wl_list_remove(&xwindow->canvas_destroy_listener.link);

	nemoxmanager_del_window(xmanager, xwindow->id);

	free(xwindow);
}

static void nemoxmanager_handle_button(struct nemoxmanager *xmanager, xcb_generic_event_t *event)
{
	xcb_button_press_event_t *button = (xcb_button_press_event_t *)event;
	struct nemoxwindow *xwindow;

	xwindow = nemoxmanager_get_window(xmanager, button->event);
	if (xwindow == NULL)
		return;
}

static void nemoxmanager_handle_enter(struct nemoxmanager *xmanager, xcb_generic_event_t *event)
{
	xcb_motion_notify_event_t *motion = (xcb_motion_notify_event_t *)event;
	struct nemoxwindow *xwindow;

	xwindow = nemoxmanager_get_window(xmanager, motion->event);
	if (xwindow == NULL)
		return;
}

static void nemoxmanager_handle_leave(struct nemoxmanager *xmanager, xcb_generic_event_t *event)
{
}

static void nemoxmanager_handle_motion(struct nemoxmanager *xmanager, xcb_generic_event_t *event)
{
}

static void nemoxmanager_handle_create_notify(struct nemoxmanager *xmanager, xcb_generic_event_t *event)
{
	xcb_create_notify_event_t *create_notify = (xcb_create_notify_event_t *)event;

#ifdef XWAYLAND_DEBUG_ON
	nemolog_message("XWAYLAND", "[CREATE_NOTIFY] window(%d) width(%d) height(%d) x(%d) y(%d) override_redirect(%d)\n",
			create_notify->window,
			create_notify->width,
			create_notify->height,
			create_notify->x,
			create_notify->y,
			create_notify->override_redirect);
#endif

	if (nemoxmanager_our_resource(xmanager, create_notify->window))
		return;

	nemoxmanager_create_window(xmanager,
			create_notify->window,
			create_notify->width,
			create_notify->height,
			create_notify->x,
			create_notify->y,
			create_notify->override_redirect);
}

static void nemoxmanager_handle_map_request(struct nemoxmanager *xmanager, xcb_generic_event_t *event)
{
	xcb_map_request_event_t *map_request = (xcb_map_request_event_t *)event;
	struct nemoxwindow *xwindow;

#ifdef XWAYLAND_DEBUG_ON
	nemolog_message("XWAYLAND", "[MAP_REQUEST]\n");
#endif

	if (nemoxmanager_our_resource(xmanager, map_request->window)) {
		nemolog_error("XWAYLAND", "xcb_map_request (window %d, ours)", map_request->window);
		return;
	}

	xwindow = nemoxmanager_get_window(xmanager, map_request->window);
	if (xwindow == NULL)
		return;

	nemoxmanager_read_properties(xwindow);

	nemoxmanager_set_window_manager_state(xwindow, ICCCM_NORMAL_STATE);
	nemoxmanager_set_net_window_manager_state(xwindow);
	nemoxmanager_set_net_window_virtual_desktop(xwindow, 0);

	xcb_map_window(xmanager->conn, map_request->window);
}

static void nemoxmanager_handle_map_notify(struct nemoxmanager *xmanager, xcb_generic_event_t *event)
{
	xcb_map_notify_event_t *map_notify = (xcb_map_notify_event_t *)event;

#ifdef XWAYLAND_DEBUG_ON
	nemolog_message("XWAYLAND", "[MAP_NOTIFY]\n");
#endif

	if (nemoxmanager_our_resource(xmanager, map_notify->window))
		return;
}

static void nemoxmanager_handle_unmap_notify(struct nemoxmanager *xmanager, xcb_generic_event_t *event)
{
	xcb_unmap_notify_event_t *unmap_notify = (xcb_unmap_notify_event_t *)event;
	struct nemoxwindow *xwindow;

#ifdef XWAYLAND_DEBUG_ON
	nemolog_message("XWAYLAND", "[UNMAP_NOTIFY]\n");
#endif

	if (nemoxmanager_our_resource(xmanager, unmap_notify->window))
		return;

	if (unmap_notify->response_type & SEND_EVENT_MASK)
		return;

	xwindow = nemoxmanager_get_window(xmanager, unmap_notify->window);
	if (xwindow == NULL)
		return;

	if (xwindow->canvas_id) {
		wl_list_remove(&xwindow->link);
		wl_list_init(&xwindow->link);

		xwindow->canvas_id = 0;
	}

	if (xmanager->focus == xwindow)
		xmanager->focus = NULL;

	if (xwindow->canvas != NULL) {
		wl_list_remove(&xwindow->canvas_destroy_listener.link);
		wl_list_init(&xwindow->canvas_destroy_listener.link);
	}

	xwindow->canvas = NULL;
	xwindow->bin = NULL;

	nemoxmanager_set_window_manager_state(xwindow, ICCCM_WITHDRAWN_STATE);
	nemoxmanager_set_net_window_virtual_desktop(xwindow, -1);
}

static void nemoxmanager_handle_reparent_notify(struct nemoxmanager *xmanager, xcb_generic_event_t *event)
{
	xcb_reparent_notify_event_t *reparent_notify = (xcb_reparent_notify_event_t *)event;
	struct nemoxwindow *xwindow;

#ifdef XWAYLAND_DEBUG_ON
	nemolog_message("XWAYLAND", "[REPARENT_NOTIFY]\n");
#endif

	if (reparent_notify->parent == xmanager->screen->root) {
		nemoxmanager_create_window(xmanager,
				reparent_notify->window,
				10, 10,
				reparent_notify->x,
				reparent_notify->y,
				reparent_notify->override_redirect);
	} else if (!nemoxmanager_our_resource(xmanager, reparent_notify->parent)) {
		xwindow = nemoxmanager_get_window(xmanager, reparent_notify->window);
		if (xwindow == NULL)
			return;

		nemoxmanager_destroy_window(xwindow);
	}
}

static void nemoxmanager_handle_configure_request(struct nemoxmanager *xmanager, xcb_generic_event_t *event)
{
	xcb_configure_request_event_t *configure_request = (xcb_configure_request_event_t *)event;
	struct nemoxwindow *xwindow;
	uint32_t mask, values[16];
	int i = 0;

#ifdef XWAYLAND_DEBUG_ON
	nemolog_message("XWAYLAND", "[CONFIGURE_REQUEST]\n");
#endif

	xwindow = nemoxmanager_get_window(xmanager, configure_request->window);
	if (xwindow == NULL)
		return;

	if (xwindow->fullscreen) {
		xcb_configure_notify_event_t configure_notify;

		configure_notify.response_type = XCB_CONFIGURE_NOTIFY;
		configure_notify.pad0 = 0;
		configure_notify.event = xwindow->id;
		configure_notify.window = xwindow->id;
		configure_notify.above_sibling = XCB_WINDOW_NONE;
		configure_notify.x = 0;
		configure_notify.y = 0;
		configure_notify.width = xwindow->width;
		configure_notify.height = xwindow->height;
		configure_notify.border_width = 0;
		configure_notify.override_redirect = 0;
		configure_notify.pad1 = 0;

		xcb_send_event(xmanager->conn, 0, xwindow->id, XCB_EVENT_MASK_STRUCTURE_NOTIFY, (char *)&configure_notify);

		return;
	}

	if (configure_request->value_mask & XCB_CONFIG_WINDOW_WIDTH)
		xwindow->width = configure_request->width;
	if (configure_request->value_mask & XCB_CONFIG_WINDOW_HEIGHT)
		xwindow->height = configure_request->height;

	values[i++] = 0;
	values[i++] = 0;
	values[i++] = xwindow->width;
	values[i++] = xwindow->height;
	values[i++] = 0;

	mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
		XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT |
		XCB_CONFIG_WINDOW_BORDER_WIDTH;
	if (configure_request->value_mask & XCB_CONFIG_WINDOW_SIBLING) {
		values[i++] = configure_request->sibling;
		mask |= XCB_CONFIG_WINDOW_SIBLING;
	}
	if (configure_request->value_mask & XCB_CONFIG_WINDOW_STACK_MODE) {
		values[i++] = configure_request->stack_mode;
		mask |= XCB_CONFIG_WINDOW_STACK_MODE;
	}

	xcb_configure_window(xmanager->conn, xwindow->id, mask, values);

	nemoxmanager_repaint_window(xwindow);
}

static void nemoxmanager_handle_configure_notify(struct nemoxmanager *xmanager, xcb_generic_event_t *event)
{
	xcb_configure_notify_event_t *configure_notify = (xcb_configure_notify_event_t *)event;
	struct nemoxwindow *xwindow;

#ifdef XWAYLAND_DEBUG_ON
	nemolog_message("XWAYLAND", "[CONFIGURE_NOTIFY] x(%d) y(%d) width(%d) height(%d)\n",
			configure_notify->x,
			configure_notify->y,
			configure_notify->width,
			configure_notify->height);
#endif

	xwindow = nemoxmanager_get_window(xmanager, configure_notify->window);
	if (xwindow == NULL)
		return;

	xwindow->x = configure_notify->x;
	xwindow->y = configure_notify->y;
	if (xwindow->override_redirect) {
		xwindow->width = configure_notify->width;
		xwindow->height = configure_notify->height;
	}
}

static void nemoxmanager_handle_destroy_notify(struct nemoxmanager *xmanager, xcb_generic_event_t *event)
{
	xcb_destroy_notify_event_t *destroy_notify = (xcb_destroy_notify_event_t *)event;
	struct nemoxwindow *xwindow;

#ifdef XWAYLAND_DEBUG_ON
	nemolog_message("XWAYLAND", "[DESTROY_NOTIFY]\n");
#endif

	if (nemoxmanager_our_resource(xmanager, destroy_notify->window))
		return;

	xwindow = nemoxmanager_get_window(xmanager, destroy_notify->window);
	if (xwindow == NULL)
		return;

	nemoxmanager_destroy_window(xwindow);
}

static void nemoxmanager_handle_mapping_notify(struct nemoxmanager *xmanager, xcb_generic_event_t *event)
{
#ifdef XWAYLAND_DEBUG_ON
	nemolog_message("XWAYLAND", "[MAPPING_NOTIFY]\n");
#endif
}

static void nemoxmanager_handle_property_notify(struct nemoxmanager *xmanager, xcb_generic_event_t *event)
{
	xcb_property_notify_event_t *property_notify = (xcb_property_notify_event_t *)event;
	struct nemoxwindow *xwindow;

#ifdef XWAYLAND_DEBUG_ON
	nemolog_message("XWAYLAND", "[PROPERTY_NOTIFY]\n");
#endif

	xwindow = nemoxmanager_get_window(xmanager, property_notify->window);
	if (xwindow == NULL)
		return;

	xwindow->properties_dirty = 1;

	if (property_notify->state != XCB_PROPERTY_DELETE) {
		xcb_get_property_reply_t *reply;
		xcb_get_property_cookie_t cookie;

		cookie = xcb_get_property(xmanager->conn, 0, property_notify->window, property_notify->atom, XCB_ATOM_ANY, 0, 2048);
		reply = xcb_get_property_reply(xmanager->conn, cookie, NULL);
		if (reply != NULL) {
		}
	}

	if (property_notify->atom == xmanager->atom.net_wm_name ||
			property_notify->atom == XCB_ATOM_WM_NAME)
		nemoxmanager_repaint_window(xwindow);
}

static void nemoxmanager_handle_moveresize(struct nemoxwindow *xwindow, xcb_client_message_event_t *client_message)
{
#ifdef XWAYLAND_DEBUG_ON
	nemolog_message("XWAYLAND", "[MOVERESIZE]\n");
#endif
}

static void nemoxmanager_handle_state(struct nemoxwindow *xwindow, xcb_client_message_event_t *client_message)
{
	struct nemoxmanager *xmanager = xwindow->xmanager;
	struct nemoshell *shell = xmanager->xserver->shell;
	uint32_t action, property;
	int maximized = (xwindow->maximized_horz && xwindow->maximized_vert);

#ifdef XWAYLAND_DEBUG_ON
	nemolog_message("XWAYLAND", "[STATE]\n");
#endif

	action = client_message->data.data32[0];
	property = client_message->data.data32[1];

	if (property == xmanager->atom.net_wm_state_fullscreen &&
			nemoxmanager_update_state(action, &xwindow->fullscreen)) {
		nemoxmanager_set_net_window_manager_state(xwindow);

		if (xwindow->fullscreen) {
			xwindow->saved_width = xwindow->width;
			xwindow->saved_height = xwindow->height;

			if (xwindow->bin != NULL) {
				nemoshell_set_fullscreen_bin(shell, xwindow->bin, nemoshell_get_fullscreen(shell, 0));
			}
		} else {
			if (xwindow->bin != NULL) {
				nemoshell_put_fullscreen_bin(shell, xwindow->bin);
			}
		}
	} else {
		if (property == xmanager->atom.net_wm_state_maximized_vert &&
				nemoxmanager_update_state(action, &xwindow->maximized_vert))
			nemoxmanager_set_net_window_manager_state(xwindow);
		if (property == xmanager->atom.net_wm_state_maximized_horz &&
				nemoxmanager_update_state(action, &xwindow->maximized_horz))
			nemoxmanager_set_net_window_manager_state(xwindow);

		if (maximized != (xwindow->maximized_vert && xwindow->maximized_horz)) {
			if (xwindow->maximized_vert && xwindow->maximized_horz) {
				xwindow->saved_width = xwindow->width;
				xwindow->saved_height = xwindow->height;

				if (xwindow->bin != NULL) {
					nemoshell_set_maximized_bin(shell, xwindow->bin, nemoshell_get_fullscreen(shell, 0));
				}
			} else {
				if (xwindow->bin != NULL) {
					nemoshell_put_maximized_bin(shell, xwindow->bin);
				}
			}
		}
	}
}

static void nemoxmanager_handle_surface_id(struct nemoxwindow *xwindow, xcb_client_message_event_t *client_message)
{
	struct nemoxmanager *xmanager = xwindow->xmanager;
	struct wl_resource *resource;

#ifdef XWAYLAND_DEBUG_ON
	nemolog_message("XWAYLAND", "[SURFACE_ID]\n");
#endif

	if (xwindow->canvas_id != 0) {
		nemolog_warning("XWAYLAND", "surface id for window %d is existed\n", xwindow->canvas_id);
		return;
	}

	uint32_t id = client_message->data.data32[0];
	resource = wl_client_get_object(xmanager->xserver->client, id);
	if (resource != NULL) {
		xwindow->canvas_id = 0;
		nemoxmanager_map_canvas(xwindow, (struct nemocanvas *)wl_resource_get_user_data(resource));
	} else {
		xwindow->canvas_id = id;
		wl_list_insert(&xmanager->unpaired_list, &xwindow->link);
	}
}

static void nemoxmanager_handle_client_message(struct nemoxmanager *xmanager, xcb_generic_event_t *event)
{
	xcb_client_message_event_t *client_message = (xcb_client_message_event_t *)event;
	struct nemoxwindow *xwindow;

#ifdef XWAYLAND_DEBUG_ON
	nemolog_message("XWAYLAND", "[CLIENT_MESSAGE]\n");
#endif

	xwindow = nemoxmanager_get_window(xmanager, client_message->window);
	if (xwindow == NULL)
		return;

	if (client_message->type == xmanager->atom.net_wm_moveresize) {
		nemoxmanager_handle_moveresize(xwindow, client_message);
	} else if (client_message->type == xmanager->atom.net_wm_state) {
		nemoxmanager_handle_state(xwindow, client_message);
	} else if (client_message->type == xmanager->atom.wl_surface_id) {
		nemoxmanager_handle_surface_id(xwindow, client_message);
	}
}

static void nemoxmanager_handle_focus_in(struct nemoxmanager *xmanager, xcb_generic_event_t *event)
{
	xcb_focus_in_event_t *focus = (xcb_focus_in_event_t *)event;

#ifdef XWAYLAND_DEBUG_ON
	nemolog_message("XWAYLAND", "[FOCUS_IN]\n");
#endif

	if (xmanager->focus == NULL || focus->event != xmanager->focus->id)
		nemoxmanager_send_focus_window(xmanager, xmanager->focus);
}

static int nemoxmanager_handle_event(int fd, uint32_t mask, void *data)
{
	struct nemoxmanager *xmanager = (struct nemoxmanager *)data;
	xcb_generic_event_t *event;
	int count = 0;

	while (event = xcb_poll_for_event(xmanager->conn), event != NULL) {
		if (nemoxmanager_handle_selection_event(xmanager, event) != 0) {
			free(event);
			count++;
			continue;
		}

		if (nemoxmanager_handle_dnd_event(xmanager, event) != 0) {
			free(event);
			count++;
			continue;
		}

		switch (EVENT_TYPE(event)) {
			case XCB_BUTTON_PRESS:
			case XCB_BUTTON_RELEASE:
				nemoxmanager_handle_button(xmanager, event);
				break;

			case XCB_ENTER_NOTIFY:
				nemoxmanager_handle_enter(xmanager, event);
				break;

			case XCB_LEAVE_NOTIFY:
				nemoxmanager_handle_leave(xmanager, event);
				break;

			case XCB_MOTION_NOTIFY:
				nemoxmanager_handle_motion(xmanager, event);
				break;

			case XCB_CREATE_NOTIFY:
				nemoxmanager_handle_create_notify(xmanager, event);
				break;

			case XCB_MAP_REQUEST:
				nemoxmanager_handle_map_request(xmanager, event);
				break;

			case XCB_MAP_NOTIFY:
				nemoxmanager_handle_map_notify(xmanager, event);
				break;

			case XCB_UNMAP_NOTIFY:
				nemoxmanager_handle_unmap_notify(xmanager, event);
				break;

			case XCB_REPARENT_NOTIFY:
				nemoxmanager_handle_reparent_notify(xmanager, event);
				break;

			case XCB_CONFIGURE_REQUEST:
				nemoxmanager_handle_configure_request(xmanager, event);
				break;

			case XCB_CONFIGURE_NOTIFY:
				nemoxmanager_handle_configure_notify(xmanager, event);
				break;

			case XCB_DESTROY_NOTIFY:
				nemoxmanager_handle_destroy_notify(xmanager, event);
				break;

			case XCB_MAPPING_NOTIFY:
				nemoxmanager_handle_mapping_notify(xmanager, event);
				break;

			case XCB_PROPERTY_NOTIFY:
				nemoxmanager_handle_property_notify(xmanager, event);
				break;

			case XCB_CLIENT_MESSAGE:
				nemoxmanager_handle_client_message(xmanager, event);
				break;

			case XCB_FOCUS_IN:
				nemoxmanager_handle_focus_in(xmanager, event);
				break;
		}

		free(event);
		count++;
	}

	xcb_flush(xmanager->conn);

	return count;
}

static void nemoxmanager_get_visual_and_colormap(struct nemoxmanager *xmanager)
{
	xcb_depth_iterator_t d_iter;
	xcb_visualtype_iterator_t vt_iter;
	xcb_visualtype_t *visualtype;

	d_iter = xcb_screen_allowed_depths_iterator(xmanager->screen);
	visualtype = NULL;
	while (d_iter.rem > 0) {
		if (d_iter.data->depth == 32) {
			vt_iter = xcb_depth_visuals_iterator(d_iter.data);
			visualtype = vt_iter.data;
			break;
		}

		xcb_depth_next(&d_iter);
	}

	if (visualtype == NULL) {
		nemolog_error("XWAYLAND", "no 32 bit visualtype");
		return;
	}

	xmanager->visual_id = visualtype->visual_id;
	xmanager->colormap = xcb_generate_id(xmanager->conn);
	xcb_create_colormap(xmanager->conn, XCB_COLORMAP_ALLOC_NONE, xmanager->colormap, xmanager->screen->root, xmanager->visual_id);
}

static void nemoxmanager_get_resources(struct nemoxmanager *xmanager)
{
#define F(field)		offsetof(struct nemoxmanager, field)
	static const struct { const char *name; int offset; } atoms[] = {
		{ "WM_PROTOCOLS", F(atom.wm_protocols) },
		{ "WM_NORMAL_HINTS", F(atom.wm_normal_hints) },
		{ "WM_TAKE_FOCUS", F(atom.wm_take_focus) },
		{ "WM_DELETE_WINDOW", F(atom.wm_delete_window) },
		{ "WM_STATE", F(atom.wm_state) },
		{ "WM_S0", F(atom.wm_s0) },
		{ "WM_CLIENT_MACHINE", F(atom.wm_client_machine) },
		{ "_NET_WM_CM_S0", F(atom.net_wm_cm_s0) },
		{ "_NET_WM_NAME", F(atom.net_wm_name) },
		{ "_NET_WM_PID", F(atom.net_wm_pid) },
		{ "_NET_WM_ICON", F(atom.net_wm_icon) },
		{ "_NET_WM_STATE", F(atom.net_wm_state) },
		{ "_NET_WM_STATE_MAXIMIZED_VERT", F(atom.net_wm_state_maximized_vert) },
		{ "_NET_WM_STATE_MAXIMIZED_HORZ", F(atom.net_wm_state_maximized_horz) },
		{ "_NET_WM_STATE_FULLSCREEN", F(atom.net_wm_state_fullscreen) },
		{ "_NET_WM_USER_TIME", F(atom.net_wm_user_time) },
		{ "_NET_WM_ICON_NAME", F(atom.net_wm_icon_name) },
		{ "_NET_WM_DESKTOP", F(atom.net_wm_desktop) },
		{ "_NET_WM_WINDOW_TYPE", F(atom.net_wm_window_type) },

		{ "_NET_WM_WINDOW_TYPE_DESKTOP", F(atom.net_wm_window_type_desktop) },
		{ "_NET_WM_WINDOW_TYPE_DOCK", F(atom.net_wm_window_type_dock) },
		{ "_NET_WM_WINDOW_TYPE_TOOLBAR", F(atom.net_wm_window_type_toolbar) },
		{ "_NET_WM_WINDOW_TYPE_MENU", F(atom.net_wm_window_type_menu) },
		{ "_NET_WM_WINDOW_TYPE_UTILITY", F(atom.net_wm_window_type_utility) },
		{ "_NET_WM_WINDOW_TYPE_SPLASH", F(atom.net_wm_window_type_splash) },
		{ "_NET_WM_WINDOW_TYPE_DIALOG", F(atom.net_wm_window_type_dialog) },
		{ "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU", F(atom.net_wm_window_type_dropdown) },
		{ "_NET_WM_WINDOW_TYPE_POPUP_MENU", F(atom.net_wm_window_type_popup) },
		{ "_NET_WM_WINDOW_TYPE_TOOLTIP", F(atom.net_wm_window_type_tooltip) },
		{ "_NET_WM_WINDOW_TYPE_NOTIFICATION", F(atom.net_wm_window_type_notification) },
		{ "_NET_WM_WINDOW_TYPE_COMBO", F(atom.net_wm_window_type_combo) },
		{ "_NET_WM_WINDOW_TYPE_DND", F(atom.net_wm_window_type_dnd) },
		{ "_NET_WM_WINDOW_TYPE_NORMAL", F(atom.net_wm_window_type_normal) },

		{ "_NET_WM_MOVERESIZE", F(atom.net_wm_moveresize) },
		{ "_NET_SUPPORTING_WM_CHECK", F(atom.net_supporting_wm_check) },
		{ "_NET_SUPPORTED", F(atom.net_supported) },
		{ "_NET_ACTIVE_WINDOW", F(atom.net_active_window) },
		{ "_MOTIF_WM_HINTS", F(atom.motif_wm_hints) },
		{ "CLIPBOARD", F(atom.clipboard) },
		{ "CLIPBOARD_MANAGER", F(atom.clipboard_manager) },
		{ "TARGETS", F(atom.targets) },
		{ "UTF8_STRING", F(atom.utf8_string) },
		{ "_WL_SELECTION", F(atom.wl_selection) },
		{ "INCR", F(atom.incr) },
		{ "TIMESTAMP", F(atom.timestamp) },
		{ "MULTIPLE", F(atom.multiple) },
		{ "UTF8_STRING", F(atom.utf8_string) },
		{ "COMPOUND_TEXT", F(atom.compound_text) },
		{ "TEXT", F(atom.text) },
		{ "STRING", F(atom.string) },
		{ "WINDOW", F(atom.window) },
		{ "text/plain;charset=utf-8", F(atom.text_plain_utf8) },
		{ "text/plain", F(atom.text_plain) },
		{ "XdndSelection", F(atom.xdnd_selection) },
		{ "XdndAware", F(atom.xdnd_aware) },
		{ "XdndEnter", F(atom.xdnd_enter) },
		{ "XdndLeave", F(atom.xdnd_leave) },
		{ "XdndDrop", F(atom.xdnd_drop) },
		{ "XdndStatus", F(atom.xdnd_status) },
		{ "XdndFinished", F(atom.xdnd_finished) },
		{ "XdndTypeList",	F(atom.xdnd_type_list) },
		{ "XdndActionCopy", F(atom.xdnd_action_copy) },
		{ "WL_SURFACE_ID", F(atom.wl_surface_id) }
	};
#undef F

	xcb_xfixes_query_version_cookie_t xfixes_cookie;
	xcb_xfixes_query_version_reply_t *xfixes_reply;
	xcb_intern_atom_cookie_t cookies[ARRAY_LENGTH(atoms)];
	xcb_intern_atom_reply_t *reply;
	xcb_render_query_pict_formats_reply_t *formats_reply;
	xcb_render_query_pict_formats_cookie_t formats_cookie;
	xcb_render_pictforminfo_t *formats;
	uint32_t i;

	xcb_prefetch_extension_data(xmanager->conn, &xcb_xfixes_id);
	xcb_prefetch_extension_data(xmanager->conn, &xcb_composite_id);

	formats_cookie = xcb_render_query_pict_formats(xmanager->conn);

	for (i = 0; i < ARRAY_LENGTH(atoms); i++) {
		cookies[i] = xcb_intern_atom(xmanager->conn, 0, strlen(atoms[i].name), atoms[i].name);
	}

	for (i = 0; i < ARRAY_LENGTH(atoms); i++) {
		reply = xcb_intern_atom_reply(xmanager->conn, cookies[i], NULL);
		*(xcb_atom_t *)((char *)xmanager + atoms[i].offset) = reply->atom;
		free(reply);
	}

	xmanager->xfixes = xcb_get_extension_data(xmanager->conn, &xcb_xfixes_id);
	if (!xmanager->xfixes || !xmanager->xfixes->present)
		nemolog_error("XWAYLAND", "xfixes not available");

	xfixes_cookie = xcb_xfixes_query_version(xmanager->conn, XCB_XFIXES_MAJOR_VERSION, XCB_XFIXES_MINOR_VERSION);
	xfixes_reply = xcb_xfixes_query_version_reply(xmanager->conn, xfixes_cookie, NULL);

	free(xfixes_reply);

	formats_reply = xcb_render_query_pict_formats_reply(xmanager->conn, formats_cookie, 0);
	if (formats_reply == NULL)
		return;

	formats = xcb_render_query_pict_formats_formats(formats_reply);
	for (i = 0; i < formats_reply->num_formats; i++) {
		if (formats[i].direct.red_mask != 0xff &&
				formats[i].direct.red_shift != 16)
			continue;
		if (formats[i].type == XCB_RENDER_PICT_TYPE_DIRECT &&
				formats[i].depth == 24)
			xmanager->format_rgb = formats[i];
		if (formats[i].type == XCB_RENDER_PICT_TYPE_DIRECT &&
				formats[i].depth == 32 &&
				formats[i].direct.alpha_mask == 0xff &&
				formats[i].direct.alpha_shift == 24)
			xmanager->format_rgba = formats[i];
	}

	free(formats_reply);
}

static void nemoxmanager_create_window_manager(struct nemoxmanager *xmanager)
{
	static const char name[] = "NEMOSHELL WM";

	xmanager->window = xcb_generate_id(xmanager->conn);
	xcb_create_window(xmanager->conn,
			XCB_COPY_FROM_PARENT,
			xmanager->window,
			xmanager->screen->root,
			0, 0,
			10, 10,
			0,
			XCB_WINDOW_CLASS_INPUT_OUTPUT,
			xmanager->screen->root_visual,
			0, NULL);

	xcb_change_property(xmanager->conn,
			XCB_PROP_MODE_REPLACE,
			xmanager->window,
			xmanager->atom.net_supporting_wm_check,
			XCB_ATOM_WINDOW,
			32, /* format */
			1, &xmanager->window);

	xcb_change_property(xmanager->conn,
			XCB_PROP_MODE_REPLACE,
			xmanager->window,
			xmanager->atom.net_wm_name,
			xmanager->atom.utf8_string,
			8, /* format */
			strlen(name), name);

	xcb_change_property(xmanager->conn,
			XCB_PROP_MODE_REPLACE,
			xmanager->screen->root,
			xmanager->atom.net_supporting_wm_check,
			XCB_ATOM_WINDOW,
			32, /* format */
			1, &xmanager->window);

	xcb_set_selection_owner(xmanager->conn,
			xmanager->window,
			xmanager->atom.wm_s0,
			XCB_TIME_CURRENT_TIME);

	xcb_set_selection_owner(xmanager->conn,
			xmanager->window,
			xmanager->atom.net_wm_cm_s0,
			XCB_TIME_CURRENT_TIME);
}

static void nemoxmanager_handle_create_surface(struct wl_listener *listener, void *data)
{
	struct nemoxmanager *xmanager = container_of(listener, struct nemoxmanager, create_surface_listener);
	struct nemocanvas *canvas = (struct nemocanvas *)data;
	struct nemoxwindow *xwindow;

	if (wl_resource_get_client(canvas->resource) != xmanager->xserver->client)
		return;

	wl_list_for_each(xwindow, &xmanager->unpaired_list, link) {
		if (xwindow->canvas_id == wl_resource_get_id(canvas->resource)) {
			nemoxmanager_map_canvas(xwindow, canvas);
			xwindow->canvas_id = 0;
			wl_list_remove(&xwindow->link);
			wl_list_init(&xwindow->link);
			break;
		}
	}
}

static void nemoxmanager_handle_activate(struct wl_listener *listener, void *data)
{
	struct nemoxmanager *xmanager = container_of(listener, struct nemoxmanager, activate_listener);
	struct nemoview *view = (struct nemoview *)data;
	struct nemoxwindow *xwindow = NULL;
	xcb_client_message_event_t message;

	if (view != NULL && view->canvas != NULL)
		xwindow = nemoxmanager_get_canvas_window(view->canvas);

	if (xwindow != NULL) {
		nemoxmanager_set_net_active_window(xmanager, xwindow->id);

		nemoxmanager_send_focus_window(xmanager, xwindow);
	}

	xmanager->focus = xwindow;

	xcb_flush(xmanager->conn);
}

static void nemoxmanager_handle_transform(struct wl_listener *listener, void *data)
{
	struct nemoxmanager *xmanager = container_of(listener, struct nemoxmanager, transform_listener);
	struct nemoxwindow *xwindow;
	struct nemoview *view = (struct nemoview *)data;
	uint32_t mask, values[2];

	if (xwindow == NULL || view == NULL || view->canvas == NULL)
		return;

	xwindow = nemoxmanager_get_canvas_window(view->canvas);
	if (xwindow == NULL)
		return;

	if (view == NULL || !nemoview_has_state(view, NEMOVIEW_MAP_STATE))
		return;

	if (xwindow->x != view->geometry.x ||
			xwindow->y != view->geometry.y) {
		values[0] = view->geometry.x;
		values[1] = view->geometry.y;
		mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y;

		xcb_flush(xmanager->conn);
	}
}

struct nemoxmanager *nemoxmanager_create(struct nemoxserver *xserver, int fd)
{
	struct nemoxmanager *xmanager;
	struct wl_event_loop *loop;
	xcb_screen_iterator_t s;
	uint32_t values[1];
	xcb_atom_t supported[6];

	xmanager = (struct nemoxmanager *)malloc(sizeof(struct nemoxmanager));
	if (xmanager == NULL)
		return NULL;
	memset(xmanager, 0, sizeof(struct nemoxmanager));

	xmanager->xserver = xserver;

	wl_list_init(&xmanager->unpaired_list);

	xmanager->window_table = hash_create(8);
	if (xmanager->window_table == NULL)
		goto err1;

	xmanager->conn = xcb_connect_to_fd(fd, NULL);
	if (xcb_connection_has_error(xmanager->conn)) {
		nemolog_error("XWAYLAND", "failed to xcb_connect_to_fd");
		close(fd);
		goto err2;
	}

	s = xcb_setup_roots_iterator(xcb_get_setup(xmanager->conn));
	xmanager->screen = s.data;

	loop = wl_display_get_event_loop(xserver->display);

	xmanager->source = wl_event_loop_add_fd(loop, fd, WL_EVENT_READABLE, nemoxmanager_handle_event, xmanager);
	wl_event_source_check(xmanager->source);

	nemoxmanager_get_resources(xmanager);
	nemoxmanager_get_visual_and_colormap(xmanager);

	values[0] =
		XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
		XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
		XCB_EVENT_MASK_PROPERTY_CHANGE;

	xcb_change_window_attributes(xmanager->conn, xmanager->screen->root, XCB_CW_EVENT_MASK, values);

	xcb_composite_redirect_subwindows(xmanager->conn, xmanager->screen->root, XCB_COMPOSITE_REDIRECT_MANUAL);

	supported[0] = xmanager->atom.net_wm_moveresize;
	supported[1] = xmanager->atom.net_wm_state;
	supported[2] = xmanager->atom.net_wm_state_fullscreen;
	supported[3] = xmanager->atom.net_wm_state_maximized_vert;
	supported[4] = xmanager->atom.net_wm_state_maximized_horz;
	supported[5] = xmanager->atom.net_active_window;

	xcb_change_property(xmanager->conn,
			XCB_PROP_MODE_REPLACE,
			xmanager->screen->root,
			xmanager->atom.net_supported,
			XCB_ATOM_ATOM,
			32,
			ARRAY_LENGTH(supported),
			supported);

	nemoxmanager_set_net_active_window(xmanager, XCB_WINDOW_NONE);

	nemoxmanager_init_selection(xmanager);
	nemoxmanager_init_dnd(xmanager);

	xcb_flush(xmanager->conn);

	xmanager->create_surface_listener.notify = nemoxmanager_handle_create_surface;
	wl_signal_add(&xserver->compz->create_surface_signal, &xmanager->create_surface_listener);

	xmanager->activate_listener.notify = nemoxmanager_handle_activate;
	wl_signal_add(&xserver->compz->activate_signal, &xmanager->activate_listener);

	xmanager->transform_listener.notify = nemoxmanager_handle_transform;
	wl_signal_add(&xserver->compz->transform_signal, &xmanager->transform_listener);

	nemoxmanager_create_cursors(xmanager);
	nemoxmanager_set_cursor(xmanager, xmanager->screen->root, NEMOX_CURSOR_LEFT_PTR);

	nemoxmanager_create_window_manager(xmanager);

	return xmanager;

err2:
	hash_destroy(xmanager->window_table);

err1:
	free(xmanager);

	return NULL;
}

void nemoxmanager_destroy(struct nemoxmanager *xmanager)
{
	hash_destroy(xmanager->window_table);

	nemoxmanager_destroy_cursors(xmanager);

	xcb_disconnect(xmanager->conn);

	wl_event_source_remove(xmanager->source);

	wl_list_remove(&xmanager->activate_listener.link);
	wl_list_remove(&xmanager->transform_listener.link);

	free(xmanager);
}

void nemoxmanager_repaint_window(struct nemoxwindow *xwindow)
{
	if (xwindow->canvas != NULL) {
		pixman_region32_fini(&xwindow->canvas->pending.opaque);
		pixman_region32_init(&xwindow->canvas->pending.opaque);

		nemocanvas_schedule_repaint(xwindow->canvas);
	}
}

static void xserver_handle_configure(void *data)
{
	struct nemoxwindow *xwindow = (struct nemoxwindow *)data;
	struct nemoxmanager *xmanager = xwindow->xmanager;
	uint32_t values[4];
	int x, y, width, height;

	values[0] = 0;
	values[1] = 0;
	values[2] = xwindow->width;
	values[3] = xwindow->height;
	xcb_configure_window(xmanager->conn,
			xwindow->id,
			XCB_CONFIG_WINDOW_X |
			XCB_CONFIG_WINDOW_Y |
			XCB_CONFIG_WINDOW_WIDTH |
			XCB_CONFIG_WINDOW_HEIGHT,
			values);

	xwindow->configure_source = NULL;

	nemoxmanager_repaint_window(xwindow);
}

static void xserver_send_configure(struct nemocanvas *canvas, int32_t width, int32_t height)
{
	struct nemoxwindow *xwindow = nemoxmanager_get_canvas_window(canvas);
	struct nemoxmanager *xmanager = xwindow->xmanager;

	xwindow->width = width;
	xwindow->height = height;

	if (xwindow->configure_source != NULL)
		return;

	xwindow->configure_source = wl_event_loop_add_idle(xmanager->xserver->loop, xserver_handle_configure, xwindow);
}

static void xserver_send_transform(struct nemocanvas *canvas, int visible, int32_t x, int32_t y, int32_t width, int32_t height)
{
}

static void xserver_send_layer(struct nemocanvas *canvas, int visible)
{
}

static void xserver_send_fullscreen(struct nemocanvas *canvas, const char *id, int32_t x, int32_t y, int32_t width, int32_t height)
{
}

static void xserver_send_close(struct nemocanvas *canvas)
{
}

static struct nemocanvas_callback xserver_callback = {
	xserver_send_configure,
	xserver_send_transform,
	xserver_send_layer,
	xserver_send_fullscreen,
	xserver_send_close
};

void nemoxmanager_map_window(struct nemoxmanager *xmanager, struct nemoxwindow *xwindow)
{
	struct nemoshell *shell = xmanager->xserver->shell;
	struct nemocanvas *canvas = xwindow->canvas;
	struct nemoxserver *xserver = xmanager->xserver;
	struct shellbin *bin;

	bin = nemoshell_create_bin(shell, canvas, &xserver_callback);
	if (bin == NULL) {
		wl_resource_post_no_memory(canvas->resource);
		return;
	}

	bin->type = NEMOSHELL_BIN_XWAYLAND_TYPE;
	bin->pid = xwindow->pid;

	nemoshell_bin_set_state(bin, NEMOSHELL_BIN_BINDABLE_STATE);

	nemoshell_use_client_uuid(shell, bin);

	xwindow->bin = bin;

	if (xwindow->fullscreen) {
		xwindow->saved_width = xwindow->width;
		xwindow->saved_height = xwindow->height;

		nemoshell_set_fullscreen_bin(shell, xwindow->bin, nemoshell_get_fullscreen(shell, 0));
	}
}

#define TYPE_WM_PROTOCOLS	XCB_ATOM_CUT_BUFFER0
#define TYPE_MOTIF_WM_HINTS	XCB_ATOM_CUT_BUFFER1
#define TYPE_NET_WM_STATE	XCB_ATOM_CUT_BUFFER2
#define TYPE_WM_NORMAL_HINTS	XCB_ATOM_CUT_BUFFER3

void nemoxmanager_read_properties(struct nemoxwindow *xwindow)
{
	struct nemoxmanager *xmanager = xwindow->xmanager;

#define F(field)		offsetof(struct nemoxwindow, field)
	const struct {
		xcb_atom_t atom;
		xcb_atom_t type;
		int offset;
	} props[] = {
		{ XCB_ATOM_WM_CLASS, XCB_ATOM_STRING, F(class) },
		{ XCB_ATOM_WM_NAME, XCB_ATOM_STRING, F(name) },
		{ XCB_ATOM_WM_TRANSIENT_FOR, XCB_ATOM_WINDOW, F(transient_for) },
		{ xmanager->atom.wm_protocols, TYPE_WM_PROTOCOLS, F(protocols) },
		{ xmanager->atom.wm_normal_hints, TYPE_WM_NORMAL_HINTS, F(protocols) },
		{ xmanager->atom.net_wm_state, TYPE_NET_WM_STATE },
		{ xmanager->atom.net_wm_window_type, XCB_ATOM_ATOM, F(type) },
		{ xmanager->atom.net_wm_name, XCB_ATOM_STRING, F(name) },
		{ xmanager->atom.net_wm_pid, XCB_ATOM_CARDINAL, F(pid) },
		{ xmanager->atom.motif_wm_hints, TYPE_MOTIF_WM_HINTS, 0 },
		{ xmanager->atom.wm_client_machine, XCB_ATOM_WM_CLIENT_MACHINE, F(machine) },
	};
#undef F

	xcb_get_property_cookie_t cookie[ARRAY_LENGTH(props)];
	xcb_get_property_reply_t *reply;
	xcb_atom_t *atom;
	uint32_t *xid;
	uint32_t i;
	void *p;

	if (!xwindow->properties_dirty)
		return;
	xwindow->properties_dirty = 0;

	for (i = 0; i < ARRAY_LENGTH(props); i++)
		cookie[i] = xcb_get_property(xmanager->conn,
				0, /* delete */
				xwindow->id,
				props[i].atom,
				XCB_ATOM_ANY, 0, 2048);

	xwindow->decorate = !xwindow->override_redirect;
	xwindow->size_hints.flags = 0;
	xwindow->motif_hints.flags = 0;
	xwindow->delete_window = 0;

	for (i = 0; i < ARRAY_LENGTH(props); i++)  {
		reply = xcb_get_property_reply(xmanager->conn, cookie[i], NULL);
		if (!reply)
			continue;
		if (reply->type == XCB_ATOM_NONE) {
			free(reply);
			continue;
		}

		p = ((char *)xwindow + props[i].offset);

		switch (props[i].type) {
			case XCB_ATOM_WM_CLIENT_MACHINE:
			case XCB_ATOM_STRING:
				if (*(char **)p)
					free(*(char **)p);

				*(char **)p = strndup(xcb_get_property_value(reply), xcb_get_property_value_length(reply));
				break;

			case XCB_ATOM_WINDOW:
				xid = xcb_get_property_value(reply);
				*(struct nemoxwindow **)p = nemoxmanager_get_window(xmanager, *xid);
				break;

			case XCB_ATOM_CARDINAL:
			case XCB_ATOM_ATOM:
				atom = xcb_get_property_value(reply);
				*(xcb_atom_t *)p = *atom;
				break;

			case TYPE_WM_PROTOCOLS:
				atom = xcb_get_property_value(reply);
				for (i = 0; i < reply->value_len; i++)
					if (atom[i] == xmanager->atom.wm_delete_window)
						xwindow->delete_window = 1;
				break;

			case TYPE_WM_NORMAL_HINTS:
				memcpy(&xwindow->size_hints, xcb_get_property_value(reply), sizeof(xwindow->size_hints));
				break;

			case TYPE_NET_WM_STATE:
				xwindow->fullscreen = 0;
				atom = xcb_get_property_value(reply);

				for (i = 0; i < reply->value_len; i++) {
					if (atom[i] == xmanager->atom.net_wm_state_fullscreen)
						xwindow->fullscreen = 1;
					else if (atom[i] == xmanager->atom.net_wm_state_maximized_vert)
						xwindow->maximized_vert = 1;
					else if (atom[i] == xmanager->atom.net_wm_state_maximized_horz)
						xwindow->maximized_horz = 1;
				}
				break;

			case TYPE_MOTIF_WM_HINTS:
				memcpy(&xwindow->motif_hints, xcb_get_property_value(reply), sizeof(xwindow->motif_hints));
				if (xwindow->motif_hints.flags & MWM_HINTS_DECORATIONS)
					xwindow->decorate = xwindow->motif_hints.decorations > 0;
				break;

			default:
				break;
		}

		free(reply);
	}
}

void nemoxmanager_add_window(struct nemoxmanager *xmanager, uint32_t id, struct nemoxwindow *xwindow)
{
	hash_set_value(xmanager->window_table, (uint64_t)id, (uint64_t)xwindow);
}

void nemoxmanager_del_window(struct nemoxmanager *xmanager, uint32_t id)
{
	hash_put_value(xmanager->window_table, (uint64_t)id);
}

struct nemoxwindow *nemoxmanager_get_window(struct nemoxmanager *xmanager, uint32_t id)
{
	uint64_t value;

	if (hash_get_value(xmanager->window_table, (uint64_t)id, &value) > 0)
		return (struct nemoxwindow *)value;

	return NULL;
}

static void nemoxmanager_handle_canvas_destroy(struct wl_listener *listener, void *data)
{
	struct nemoxwindow *xwindow = container_of(listener, struct nemoxwindow, canvas_destroy_listener);

	xwindow->bin = NULL;
	xwindow->canvas = NULL;

	wl_list_remove(&xwindow->canvas_destroy_listener.link);
	wl_list_init(&xwindow->canvas_destroy_listener.link);
}

struct nemoxwindow *nemoxmanager_get_canvas_window(struct nemocanvas *canvas)
{
	struct wl_listener *listener;

	listener = wl_signal_get(&canvas->destroy_signal, nemoxmanager_handle_canvas_destroy);
	if (listener != NULL)
		return container_of(listener, struct nemoxwindow, canvas_destroy_listener);

	return NULL;
}

void nemoxmanager_map_canvas(struct nemoxwindow *xwindow, struct nemocanvas *canvas)
{
	struct nemoxmanager *xmanager = xwindow->xmanager;

	nemoxmanager_read_properties(xwindow);

	if (xwindow->canvas != NULL)
		wl_list_remove(&xwindow->canvas_destroy_listener.link);

	xwindow->canvas = canvas;
	xwindow->canvas_destroy_listener.notify = nemoxmanager_handle_canvas_destroy;
	wl_signal_add(&canvas->destroy_signal, &xwindow->canvas_destroy_listener);

	nemoxmanager_repaint_window(xwindow);
	nemoxmanager_map_window(xmanager, xwindow);

	if (xwindow->override_redirect != 0 && xmanager->focus != NULL) {
		xwindow->bin->transient.x = xwindow->x;
		xwindow->bin->transient.y = xwindow->y;

		nemoshell_set_parent_bin(xwindow->bin, xmanager->focus->bin);
	}
}
