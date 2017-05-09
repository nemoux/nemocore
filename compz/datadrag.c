#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <datadrag.h>
#include <datadevice.h>
#include <dataoffer.h>
#include <canvas.h>
#include <content.h>
#include <view.h>
#include <compz.h>
#include <seat.h>
#include <pointer.h>
#include <touch.h>
#include <picker.h>
#include <nemomisc.h>

static void datadrag_configure_pointer_canvas(struct nemocanvas *canvas, int32_t sx, int32_t sy)
{
	struct nemodatadrag *drag = (struct nemodatadrag *)canvas->configure_private;
	struct nemopointer *pointer = drag->base.pointer.pointer;

	if (!nemoview_has_state(drag->icon, NEMOVIEW_MAP_STATE) && canvas->buffer_reference.buffer) {
		nemoview_attach_layer(drag->icon, canvas->compz->cursor_layer);
		nemoview_damage_below(drag->icon);
		nemoview_update_transform(drag->icon);

		pixman_region32_clear(&canvas->pending.input);
	}

	drag->dx += sx;
	drag->dy += sy;

	nemoview_set_position(drag->icon, pointer->x + drag->dx, pointer->y + drag->dy);
}

static void datadrag_handle_focus_destroy(struct wl_listener *listener, void *data)
{
	struct nemodatadrag *drag = (struct nemodatadrag *)container_of(listener, struct nemodatadrag, focus_listener);

	drag->focus_resource = NULL;
}

static void datadrag_set_focus(struct nemodatadrag *drag, struct nemoseat *seat, struct nemoview *view, float sx, float sy)
{
	struct wl_resource *resource;
	struct wl_resource *offer = NULL;
	uint32_t serial;

	if (drag->focus && view && drag->focus->canvas == view->canvas) {
		drag->focus = view;
		return;
	}

	if (drag->focus_resource) {
		wl_data_device_send_leave(drag->focus_resource);
		wl_list_remove(&drag->focus_listener.link);
		drag->focus_resource = NULL;
		drag->focus = NULL;
	}

	if (view == NULL || view->canvas == NULL || view->canvas->resource == NULL)
		return;

	if (drag->data_source == NULL &&
			wl_resource_get_client(view->canvas->resource) != drag->client)
		return;

	resource = wl_resource_find_for_client(&seat->drag_resource_list, wl_resource_get_client(view->canvas->resource));
	if (resource == NULL)
		return;

	serial = wl_display_next_serial(seat->compz->display);

	if (drag->data_source) {
		offer = dataoffer_create(drag->data_source, resource);
		if (offer == NULL)
			return;
	}

	wl_data_device_send_enter(resource, serial, view->canvas->resource,
			wl_fixed_from_double(sx), wl_fixed_from_double(sy), offer);

	drag->focus = view;
	drag->focus_listener.notify = datadrag_handle_focus_destroy;
	wl_resource_add_destroy_listener(resource, &drag->focus_listener);
	drag->focus_resource = resource;
}

static void datadrag_end_drag(struct nemodatadrag *drag, struct nemoseat *seat)
{
	if (drag->icon) {
		if (nemoview_has_state(drag->icon, NEMOVIEW_MAP_STATE))
			nemoview_unmap(drag->icon);

		drag->icon->canvas->configure = NULL;
		pixman_region32_clear(&drag->icon->canvas->pending.input);
		wl_list_remove(&drag->icon_destroy_listener.link);
		nemoview_destroy(drag->icon);
	}

	datadrag_set_focus(drag, seat, NULL, 0, 0);
}

static void datadrag_end_pointer_drag(struct nemodatadrag *drag)
{
	struct nemopointer *pointer = drag->base.pointer.pointer;

	datadrag_end_drag(drag, pointer->seat);
	nemopointer_end_grab(pointer);
	free(drag);
}

static void datadrag_pointer_grab_focus(struct nemopointer_grab *base)
{
	struct nemodatadrag *drag = (struct nemodatadrag *)container_of(base, struct nemodatadrag, base.pointer);
	struct nemopointer *pointer = drag->base.pointer.pointer;
	struct nemoview *view;
	float sx, sy;

	view = nemocompz_pick_view(pointer->seat->compz, pointer->x, pointer->y, &sx, &sy, NEMOVIEW_PICK_STATE);

	if (drag->focus != view) {
		datadrag_set_focus(drag, pointer->seat, view, sx, sy);
	}
}

static void datadrag_pointer_grab_motion(struct nemopointer_grab *base, uint32_t time, float x, float y)
{
	struct nemodatadrag *drag = (struct nemodatadrag *)container_of(base, struct nemodatadrag, base.pointer);
	struct nemopointer *pointer = drag->base.pointer.pointer;
	float sx, sy;

	nemopointer_move(pointer, x, y);

	if (drag->icon) {
		nemoview_set_position(drag->icon, pointer->x + drag->dx, pointer->y + drag->dy);
		nemoview_update_transform(drag->icon);
		nemoview_schedule_repaint(drag->icon);
	}

	if (drag->focus_resource) {
		nemoview_transform_from_global(drag->focus, pointer->x, pointer->y, &sx, &sy);

		wl_data_device_send_motion(drag->focus_resource, time,
				wl_fixed_from_double(sx), wl_fixed_from_double(sy));
	}
}

static void datadrag_pointer_grab_axis(struct nemopointer_grab *base, uint32_t time, uint32_t axis, float value)
{
}

static void datadrag_pointer_grab_button(struct nemopointer_grab *base, uint32_t time, uint32_t button, uint32_t state)
{
	struct nemodatadrag *drag = (struct nemodatadrag *)container_of(base, struct nemodatadrag, base.pointer);
	struct nemopointer *pointer = drag->base.pointer.pointer;

	if (drag->focus_resource &&
			pointer->grab_button == button &&
			state == WL_POINTER_BUTTON_STATE_RELEASED) {
		wl_data_device_send_drop(drag->focus_resource);
	}

	if (pointer->button_count == 0 &&
			state == WL_POINTER_BUTTON_STATE_RELEASED) {
		if (drag->data_source) {
			wl_list_remove(&drag->data_source_listener.link);
		}

		datadrag_end_pointer_drag(drag);
	}
}

static void datadrag_pointer_grab_cancel(struct nemopointer_grab *base)
{
	struct nemodatadrag *drag = (struct nemodatadrag *)container_of(base, struct nemodatadrag, base.pointer);

	if (drag->data_source) {
		wl_list_remove(&drag->data_source_listener.link);
	}

	datadrag_end_pointer_drag(drag);
}

static const struct nemopointer_grab_interface datadrag_pointer_grab_interface = {
	datadrag_pointer_grab_focus,
	datadrag_pointer_grab_motion,
	datadrag_pointer_grab_axis,
	datadrag_pointer_grab_button,
	datadrag_pointer_grab_cancel
};

static void datadrag_handle_icon_destroy(struct wl_listener *listener, void *data)
{
	struct nemodatadrag *drag = (struct nemodatadrag *)container_of(listener, struct nemodatadrag, icon_destroy_listener);

	drag->icon = NULL;
}

static void datadrag_handle_pointer_data_source_destroy(struct wl_listener *listener, void *data)
{
	struct nemodatadrag *drag = (struct nemodatadrag *)container_of(listener, struct nemodatadrag, data_source_listener);

	datadrag_end_pointer_drag(drag);
}

int datadrag_start_pointer_grab(struct nemopointer *pointer, struct nemodatasource *source, struct nemocanvas *icon, struct wl_client *client)
{
	struct nemodatadrag *drag;

	drag = (struct nemodatadrag *)malloc(sizeof(struct nemodatadrag));
	if (drag == NULL)
		return -1;
	memset(drag, 0, sizeof(struct nemodatadrag));

	drag->client = client;
	drag->data_source = source;

	drag->base.pointer.interface = &datadrag_pointer_grab_interface;

	if (icon != NULL) {
		drag->icon = nemoview_create(icon->compz);
		if (drag->icon == NULL)
			goto err1;

		nemoview_attach_canvas(drag->icon, icon);

		drag->icon_destroy_listener.notify = datadrag_handle_icon_destroy;
		wl_signal_add(&icon->destroy_signal, &drag->icon_destroy_listener);

		icon->configure = datadrag_configure_pointer_canvas;
		icon->configure_private = (void *)drag;
	} else {
		drag->icon = NULL;
	}

	if (source != NULL) {
		drag->data_source_listener.notify = datadrag_handle_pointer_data_source_destroy;
		wl_signal_add(&source->destroy_signal, &drag->data_source_listener);
	}

	nemopointer_set_focus(pointer, NULL, 0, 0);
	nemopointer_start_grab(pointer, &drag->base.pointer);

	return 0;

err1:
	free(drag);

	return -1;
}

int datadrag_start_touchpoint_grab(struct touchpoint *tp, struct nemodatasource *source, struct nemocanvas *icon, struct wl_client *client)
{
	return 0;
}
