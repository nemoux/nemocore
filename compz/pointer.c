#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <assert.h>
#include <wayland-server.h>
#include <wayland-nemo-seat-server-protocol.h>

#include <pointer.h>
#include <seat.h>
#include <keyboard.h>
#include <compz.h>
#include <view.h>
#include <content.h>
#include <canvas.h>
#include <layer.h>
#include <picker.h>
#include <binding.h>
#include <nemomisc.h>

static void default_pointer_grab_focus(struct nemopointer_grab *grab)
{
	struct nemopointer *pointer = grab->pointer;
	struct nemoview *view;
	float sx, sy;

	if (pointer->button_count > 0)
		return;

	view = nemocompz_pick_view(pointer->seat->compz, pointer->x, pointer->y, &sx, &sy, NEMOVIEW_PICK_STATE);

	if (pointer->focus != view) {
		nemopointer_set_focus(pointer, view, sx, sy);
	}
}

static void default_pointer_grab_motion(struct nemopointer_grab *grab, uint32_t time, float x, float y)
{
	struct nemopointer *pointer = grab->pointer;

	nemopointer_move(pointer, x, y);

	if (pointer->focus != NULL) {
		nemoview_transform_from_global(pointer->focus, x, y, &pointer->sx, &pointer->sy);

		nemocontent_pointer_motion(pointer, pointer->focus->content, time, pointer->sx, pointer->sy);
	}
}

static void default_pointer_grab_axis(struct nemopointer_grab *grab, uint32_t time, uint32_t axis, float value)
{
	struct nemopointer *pointer = grab->pointer;

	if (pointer->focus != NULL) {
		nemocontent_pointer_axis(pointer, pointer->focus->content, time, axis, value);
	}
}

static void default_pointer_grab_button(struct nemopointer_grab *grab, uint32_t time, uint32_t button, uint32_t state)
{
	struct nemopointer *pointer = grab->pointer;
	struct nemoview *view;
	float sx, sy;

	if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
		view = nemocompz_pick_view(pointer->seat->compz, pointer->x, pointer->y, &sx, &sy, NEMOVIEW_PICK_STATE);
		if (pointer->focus != view) {
			nemopointer_set_focus(pointer, view, sx, sy);
		}

		if (pointer->focus != NULL) {
			nemocontent_pointer_button(pointer, pointer->focus->content, time, button, state);
		}
	} else {
		if (pointer->focus != NULL) {
			nemocontent_pointer_button(pointer, pointer->focus->content, time, button, state);
		}

		view = nemocompz_pick_view(pointer->seat->compz, pointer->x, pointer->y, &sx, &sy, NEMOVIEW_PICK_STATE);
		if (pointer->focus != view) {
			nemopointer_set_focus(pointer, view, sx, sy);
		}
	}
}

static void default_pointer_grab_cancel(struct nemopointer_grab *grab)
{
}

static const struct nemopointer_grab_interface default_pointer_grab_interface = {
	default_pointer_grab_focus,
	default_pointer_grab_motion,
	default_pointer_grab_axis,
	default_pointer_grab_button,
	default_pointer_grab_cancel
};

static void nemopointer_destroy_sprite(struct nemopointer *pointer)
{
	if (nemoview_has_state(pointer->sprite, NEMOVIEW_MAP_STATE))
		nemoview_unmap(pointer->sprite);

	wl_list_remove(&pointer->sprite_destroy_listener.link);
	wl_list_init(&pointer->sprite_destroy_listener.link);

	if (pointer->sprite->canvas != NULL) {
		pointer->sprite->canvas->configure = NULL;
		pointer->sprite->canvas->configure_private = NULL;

		nemoview_destroy(pointer->sprite);
	}

	pointer->sprite = NULL;
}

static void nemopointer_handle_focus_view_destroy(struct wl_listener *listener, void *data)
{
	struct nemopointer *pointer = (struct nemopointer *)container_of(listener, struct nemopointer, focus_view_listener);

	pointer->focus = NULL;

	nemopointer_set_focus(pointer, NULL, 0, 0);
}

static void nemopointer_handle_focus_resource_destroy(struct wl_listener *listener, void *data)
{
	struct nemopointer *pointer = (struct nemopointer *)container_of(listener, struct nemopointer, focus_resource_listener);

	pointer->focus = NULL;

	nemopointer_set_focus(pointer, NULL, 0, 0);
}

static void nemopointer_handle_sprite_destroy(struct wl_listener *listener, void *data)
{
	struct nemopointer *pointer = (struct nemopointer *)container_of(listener, struct nemopointer, sprite_destroy_listener);
	if (pointer->sprite != NULL)
		nemopointer_destroy_sprite(pointer);
}

static void nemopointer_handle_keyboard_destroy(struct wl_listener *listener, void *data)
{
	struct nemopointer *pointer = (struct nemopointer *)container_of(listener, struct nemopointer, keyboard_destroy_listener);

	wl_list_remove(&pointer->keyboard_destroy_listener.link);
	wl_list_init(&pointer->keyboard_destroy_listener.link);

	pointer->keyboard = NULL;
}

static void nemopointer_configure_canvas(struct nemocanvas *canvas, int32_t dx, int32_t dy)
{
	struct nemopointer *pointer = (struct nemopointer *)canvas->configure_private;

	if (canvas->base.width == 0)
		return;

	assert(canvas == pointer->sprite->canvas);

	pointer->hotspot_x = pointer->hotspot_x - dx;
	pointer->hotspot_y = pointer->hotspot_y - dy;

	nemoview_set_position(pointer->sprite,
			pointer->x - pointer->hotspot_x,
			pointer->y - pointer->hotspot_y);

	pixman_region32_clear(&canvas->pending.input);
	pixman_region32_clear(&canvas->base.input);

	if (!nemoview_has_state(pointer->sprite, NEMOVIEW_MAP_STATE)) {
		nemoview_attach_layer(pointer->sprite, canvas->compz->cursor_layer);
		nemoview_damage_below(pointer->sprite);
		nemoview_update_transform(pointer->sprite);
	}
}

static void wayland_pointer_set_cursor(struct wl_client *client, struct wl_resource *resource, uint32_t serial, struct wl_resource *surface_resource, int32_t x, int32_t y)
{
	struct nemoseat *seat = (struct nemoseat *)wl_resource_get_user_data(resource);
	struct nemopointer *pointer;
	struct nemocanvas *canvas = NULL;

	pointer = nemoseat_get_pointer_by_focus_serial(seat, serial);
	if (pointer == NULL)
		return;

	if (surface_resource != NULL)
		canvas = (struct nemocanvas *)wl_resource_get_user_data(surface_resource);

	if (pointer->focus == NULL ||
			pointer->focus->canvas == NULL ||
			pointer->focus->canvas->resource == NULL)
		return;

	if (wl_resource_get_client(pointer->focus->canvas->resource) != client)
		return;

	if (pointer->sprite != NULL)
		nemopointer_destroy_sprite(pointer);

	if (canvas == NULL)
		return;

	pointer->sprite = nemoview_create(seat->compz);
	if (pointer->sprite == NULL)
		return;

	nemoview_attach_canvas(pointer->sprite, canvas);

	wl_signal_add(&canvas->destroy_signal, &pointer->sprite_destroy_listener);

	canvas->configure = nemopointer_configure_canvas;
	canvas->configure_private = pointer;

	pointer->hotspot_x = x;
	pointer->hotspot_y = y;

	if (canvas->buffer_reference.buffer)
		nemopointer_configure_canvas(canvas, 0, 0);
}

static void wayland_pointer_release(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static const struct wl_pointer_interface wayland_pointer_implementation = {
	wayland_pointer_set_cursor,
	wayland_pointer_release
};

static void nemopointer_unbind_wayland(struct wl_resource *resource)
{
	wl_list_remove(wl_resource_get_link(resource));
}

int nemopointer_bind_wayland(struct wl_client *client, struct wl_resource *seat_resource, uint32_t id)
{
	struct nemoseat *seat = (struct nemoseat *)wl_resource_get_user_data(seat_resource);
	struct wl_resource *resource;

	resource = wl_resource_create(client, &wl_pointer_interface, wl_resource_get_version(seat_resource), id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return -1;
	}

	wl_resource_set_implementation(resource, &wayland_pointer_implementation, seat, nemopointer_unbind_wayland);

	wl_list_insert(&seat->pointer.resource_list, wl_resource_get_link(resource));

	return 0;
}

static void nemo_pointer_set_cursor(struct wl_client *client, struct wl_resource *resource, uint32_t serial, struct wl_resource *surface_resource, int32_t id, int32_t x, int32_t y)
{
	struct nemoseat *seat = (struct nemoseat *)wl_resource_get_user_data(resource);
	struct nemopointer *pointer;
	struct nemocanvas *canvas = NULL;

	pointer = nemoseat_get_pointer_by_focus_serial(seat, serial);
	if (pointer == NULL)
		return;

	if (surface_resource != NULL)
		canvas = (struct nemocanvas *)wl_resource_get_user_data(surface_resource);

	if (pointer->focus == NULL ||
			pointer->focus->canvas == NULL ||
			pointer->focus->canvas->resource == NULL)
		return;

	if (wl_resource_get_client(pointer->focus->canvas->resource) != client)
		return;

	if (pointer->sprite != NULL)
		nemopointer_destroy_sprite(pointer);

	if (canvas == NULL)
		return;

	pointer->sprite = nemoview_create(seat->compz);
	if (pointer->sprite == NULL)
		return;

	nemoview_attach_canvas(pointer->sprite, canvas);

	wl_signal_add(&canvas->destroy_signal, &pointer->sprite_destroy_listener);

	canvas->configure = nemopointer_configure_canvas;
	canvas->configure_private = pointer;

	pointer->hotspot_x = x;
	pointer->hotspot_y = y;

	if (canvas->buffer_reference.buffer)
		nemopointer_configure_canvas(canvas, 0, 0);
}

static void nemo_pointer_release(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static const struct nemo_pointer_interface nemo_pointer_implementation = {
	nemo_pointer_set_cursor,
	nemo_pointer_release
};

static void nemopointer_unbind_nemo(struct wl_resource *resource)
{
	wl_list_remove(wl_resource_get_link(resource));
}

int nemopointer_bind_nemo(struct wl_client *client, struct wl_resource *seat_resource, uint32_t id)
{
	struct nemoseat *seat = (struct nemoseat *)wl_resource_get_user_data(seat_resource);
	struct wl_resource *resource;

	resource = wl_resource_create(client, &nemo_pointer_interface, wl_resource_get_version(seat_resource), id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return -1;
	}

	wl_resource_set_implementation(resource, &nemo_pointer_implementation, seat, nemopointer_unbind_nemo);

	wl_list_insert(&seat->pointer.nemo_resource_list, wl_resource_get_link(resource));

	return 0;
}

struct nemopointer *nemopointer_create(struct nemoseat *seat, struct inputnode *node)
{
	struct nemopointer *pointer;

	pointer = (struct nemopointer *)malloc(sizeof(struct nemopointer));
	if (pointer == NULL)
		return NULL;
	memset(pointer, 0, sizeof(struct nemopointer));

	pointer->seat = seat;
	pointer->node = node;
	pointer->id = ++seat->compz->pointer_ids;

	wl_signal_init(&pointer->destroy_signal);

	pointer->default_grab.interface = &default_pointer_grab_interface;
	pointer->default_grab.pointer = pointer;
	pointer->grab = &pointer->default_grab;

	wl_list_init(&pointer->focus_resource_listener.link);
	pointer->focus_resource_listener.notify = nemopointer_handle_focus_resource_destroy;
	wl_list_init(&pointer->focus_view_listener.link);
	pointer->focus_view_listener.notify = nemopointer_handle_focus_view_destroy;
	wl_list_init(&pointer->sprite_destroy_listener.link);
	pointer->sprite_destroy_listener.notify = nemopointer_handle_sprite_destroy;
	wl_list_init(&pointer->keyboard_destroy_listener.link);
	pointer->keyboard_destroy_listener.notify = nemopointer_handle_keyboard_destroy;

	wl_list_insert(&seat->pointer.device_list, &pointer->link);

	return pointer;
}

void nemopointer_destroy(struct nemopointer *pointer)
{
	wl_signal_emit(&pointer->destroy_signal, pointer);

	nemopointer_set_focus(pointer, NULL, 0, 0);

	wl_list_remove(&pointer->focus_view_listener.link);
	wl_list_remove(&pointer->focus_resource_listener.link);
	wl_list_remove(&pointer->sprite_destroy_listener.link);

	wl_list_remove(&pointer->link);

	free(pointer);
}

void nemopointer_move(struct nemopointer *pointer, float x, float y)
{
	struct nemocompz *compz = pointer->seat->compz;

	if (pixman_region32_contains_point(&compz->scene, x, y, NULL)) {
		pointer->x = x;
		pointer->y = y;
	} else if (pixman_region32_contains_point(&compz->scene, pointer->x, y, NULL)) {
		pointer->y = y;
	} else if (pixman_region32_contains_point(&compz->scene, x, pointer->y, NULL)) {
		pointer->x = x;
	}

	if (pointer->sprite != NULL) {
		nemoview_set_position(pointer->sprite,
				pointer->x - pointer->hotspot_x,
				pointer->y - pointer->hotspot_y);
		nemoview_update_transform(pointer->sprite);
		nemoview_schedule_repaint(pointer->sprite);
	}

	pointer->grab->interface->focus(pointer->grab);
}

void nemopointer_set_focus(struct nemopointer *pointer, struct nemoview *view, float sx, float sy)
{
	if (view != NULL && nemoseat_has_pointer_resource_by_view(pointer->seat, view) == 0)
		view = NULL;

	if ((pointer->focus == NULL) || (view == NULL) || (pointer->focus->content != view->content)) {
		if (pointer->sprite != NULL)
			nemopointer_destroy_sprite(pointer);

		if (pointer->focus != NULL)
			nemocontent_pointer_leave(pointer, pointer->focus->content);

		pointer->focused = pointer->focus;
		pointer->focus = view;
		pointer->sx = sx;
		pointer->sy = sy;

		if (view != NULL)
			nemocontent_pointer_enter(pointer, view->content);

		wl_list_remove(&pointer->focus_view_listener.link);
		wl_list_init(&pointer->focus_view_listener.link);
		wl_list_remove(&pointer->focus_resource_listener.link);
		wl_list_init(&pointer->focus_resource_listener.link);

		if (view != NULL)
			wl_signal_add(&view->destroy_signal, &pointer->focus_view_listener);
		if (view != NULL && view->canvas != NULL && view->canvas->resource != NULL)
			wl_resource_add_destroy_listener(view->canvas->resource, &pointer->focus_resource_listener);

		wl_signal_emit(&pointer->seat->pointer.focus_signal, pointer);
	}
}

void nemopointer_notify_button(struct nemopointer *pointer, uint32_t time, int32_t button, enum wl_pointer_button_state state)
{
	if (pointer == NULL)
		return;

	if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
		if (pointer->button_count == 0) {
			pointer->grab_button = button;
			pointer->grab_time = time;
			pointer->grab_x = pointer->x;
			pointer->grab_y = pointer->y;
		}

		pointer->button_count++;
	} else {
		pointer->button_count--;
	}

	pointer->grab->interface->button(pointer->grab, time, button, state);

	if (pointer->button_count == 1) {
		pointer->grab_serial = wl_display_get_serial(pointer->seat->compz->display);
	}

	nemocompz_run_button_binding(pointer->seat->compz, pointer, time, button, state);
}

void nemopointer_notify_motion(struct nemopointer *pointer, uint32_t time, float dx, float dy)
{
	if (pointer == NULL)
		return;

	pointer->grab->interface->motion(pointer->grab, time, pointer->x + dx, pointer->y + dy);
}

void nemopointer_notify_motion_absolute(struct nemopointer *pointer, uint32_t time, float x, float y)
{
	if (pointer == NULL)
		return;

	pointer->grab->interface->motion(pointer->grab, time, x, y);
}

void nemopointer_notify_axis(struct nemopointer *pointer, uint32_t time, uint32_t axis, float value)
{
	if (pointer == NULL)
		return;

	pointer->grab->interface->axis(pointer->grab, time, axis, value);
}

void nemopointer_set_keyboard(struct nemopointer *pointer, struct nemokeyboard *keyboard)
{
	if (pointer->keyboard != NULL) {
		wl_list_remove(&pointer->keyboard_destroy_listener.link);
		wl_list_init(&pointer->keyboard_destroy_listener.link);
	}

	if (keyboard != NULL) {
		wl_signal_add(&keyboard->destroy_signal, &pointer->keyboard_destroy_listener);
	}

	pointer->keyboard = keyboard;
}

void nemopointer_set_keyboard_focus(struct nemopointer *pointer, struct nemoview *view)
{
	if (pointer->keyboard == NULL) {
		nemoseat_set_keyboard_focus(pointer->seat, view);
	} else {
		nemokeyboard_set_focus(pointer->keyboard, view);
	}
}

void nemopointer_start_grab(struct nemopointer *pointer, struct nemopointer_grab *grab)
{
	pointer->grab = grab;
	grab->pointer = pointer;
	pointer->grab->interface->focus(pointer->grab);
}

void nemopointer_end_grab(struct nemopointer *pointer)
{
	pointer->grab = &pointer->default_grab;
	pointer->grab->interface->focus(pointer->grab);
}

void nemopointer_cancel_grab(struct nemopointer *pointer)
{
	pointer->grab->interface->cancel(pointer->grab);
}
