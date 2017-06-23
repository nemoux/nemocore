#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <assert.h>
#include <limits.h>
#include <linux/input.h>
#include <wayland-server.h>
#include <wayland-nemo-seat-server-protocol.h>

#include <canvas.h>
#include <compz.h>
#include <subcanvas.h>
#include <view.h>
#include <region.h>
#include <screen.h>
#include <renderer.h>
#include <seat.h>
#include <pointer.h>
#include <keyboard.h>
#include <keypad.h>
#include <touch.h>
#include <presentation.h>
#include <waylandhelper.h>
#include <nemomisc.h>

static void nemocanvas_state_handle_buffer_destroy(struct wl_listener *listener, void *data)
{
	struct nemocanvas_state *state = (struct nemocanvas_state *)container_of(listener, struct nemocanvas_state, buffer_destroy_listener);

	state->buffer = NULL;
}

void nemocanvas_state_prepare(struct nemocanvas_state *state)
{
	state->newly_attached = 0;
	state->buffer = NULL;
	state->buffer_destroy_listener.notify = nemocanvas_state_handle_buffer_destroy;
	state->sx = 0;
	state->sy = 0;

	pixman_region32_init(&state->damage);
	pixman_region32_init(&state->opaque);
	pixman_region32_init_rect(&state->input, INT32_MIN, INT32_MIN, UINT32_MAX, UINT32_MAX);

	wl_list_init(&state->frame_callback_list);
	wl_list_init(&state->feedback_list);

	state->buffer_viewport.buffer.transform = WL_OUTPUT_TRANSFORM_NORMAL;
	state->buffer_viewport.buffer.scale = 1;
	state->buffer_viewport.buffer.src_width = -1;
	state->buffer_viewport.canvas.width = -1;
	state->buffer_viewport.changed = 0;
}

void nemocanvas_state_finish(struct nemocanvas_state *state)
{
	struct nemoframe_callback *cb, *cnext;

	wl_list_for_each_safe(cb, cnext, &state->frame_callback_list, link)
		wl_resource_destroy(cb->resource);

	pixman_region32_fini(&state->input);
	pixman_region32_fini(&state->opaque);
	pixman_region32_fini(&state->damage);

	if (state->buffer != NULL)
		wl_list_remove(&state->buffer_destroy_listener.link);

	wl_list_remove(&state->feedback_list);

	state->buffer = NULL;
}

void nemocanvas_state_set_buffer(struct nemocanvas_state *state, struct nemobuffer *buffer)
{
	if (state->buffer == buffer)
		return;

	if (state->buffer != NULL)
		wl_list_remove(&state->buffer_destroy_listener.link);
	state->buffer = buffer;
	if (state->buffer != NULL)
		wl_signal_add(&state->buffer->destroy_signal, &state->buffer_destroy_listener);
}

static void nemobuffer_reference_handle_buffer_destroy(struct wl_listener *listener, void *data)
{
	struct nemobuffer_reference *ref = (struct nemobuffer_reference *)container_of(listener, struct nemobuffer_reference, destroy_listener);

	assert((struct nemobuffer *)data == ref->buffer);

	ref->buffer = NULL;
}

void nemobuffer_reference(struct nemobuffer_reference *ref, struct nemobuffer *buffer)
{
	if (ref->buffer && buffer != ref->buffer) {
		ref->buffer->busy_count--;
		if (ref->buffer->busy_count == 0) {
			assert(wl_resource_get_client(ref->buffer->resource));

			wl_resource_queue_event(ref->buffer->resource, WL_BUFFER_RELEASE);
		}

		wl_list_remove(&ref->destroy_listener.link);
	}

	if (buffer && buffer != ref->buffer) {
		buffer->busy_count++;

		wl_signal_add(&buffer->destroy_signal, &ref->destroy_listener);
	}

	ref->buffer = buffer;
	ref->destroy_listener.notify = nemobuffer_reference_handle_buffer_destroy;
}

static void nemobuffer_handle_buffer_destroy(struct wl_listener *listener, void *data)
{
	struct nemobuffer *buffer = (struct nemobuffer *)container_of(listener, struct nemobuffer, destroy_listener);

	wl_signal_emit(&buffer->destroy_signal, buffer);

	free(buffer);
}

struct nemobuffer *nemobuffer_from_resource(struct wl_resource *resource)
{
	struct nemobuffer *buffer;
	struct wl_listener *listener;

	listener = wl_resource_get_destroy_listener(resource, nemobuffer_handle_buffer_destroy);
	if (listener != NULL)
		return (struct nemobuffer *)container_of(listener, struct nemobuffer, destroy_listener);

	buffer = (struct nemobuffer *)malloc(sizeof(struct nemobuffer));
	if (buffer == NULL)
		return NULL;
	memset(buffer, 0, sizeof(struct nemobuffer));

	buffer->resource = resource;
	wl_signal_init(&buffer->destroy_signal);
	buffer->destroy_listener.notify = nemobuffer_handle_buffer_destroy;
	buffer->y_inverted = 1;
	wl_resource_add_destroy_listener(resource, &buffer->destroy_listener);

	return buffer;
}

static void nemocanvas_unbind_canvas(struct wl_resource *resource)
{
	struct nemocanvas *canvas = (struct nemocanvas *)wl_resource_get_user_data(resource);

	canvas->resource = NULL;

	nemocanvas_destroy(canvas);
}

static void surface_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void surface_attach(struct wl_client *client, struct wl_resource *resource, struct wl_resource *buffer_resource, int32_t sx, int32_t sy)
{
	struct nemocanvas *canvas = (struct nemocanvas *)wl_resource_get_user_data(resource);
	struct nemobuffer *buffer = NULL;

	if (buffer_resource != NULL) {
		buffer = nemobuffer_from_resource(buffer_resource);
		if (buffer == NULL) {
			wl_client_post_no_memory(client);
			return;
		}
	}

	nemocanvas_state_set_buffer(&canvas->pending, buffer);

	canvas->pending.sx = sx;
	canvas->pending.sy = sy;
	canvas->pending.newly_attached = 1;
}

static void surface_damage(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct nemocanvas *canvas = (struct nemocanvas *)wl_resource_get_user_data(resource);

	pixman_region32_union_rect(&canvas->pending.damage, &canvas->pending.damage, x, y, width, height);
}

static void nemocanvas_unbind_frame_callback(struct wl_resource *resource)
{
	struct nemoframe_callback *cb = (struct nemoframe_callback *)wl_resource_get_user_data(resource);

	wl_list_remove(&cb->link);

	free(cb);
}

static void surface_frame(struct wl_client *client, struct wl_resource *resource, uint32_t callback)
{
	struct nemocanvas *canvas = (struct nemocanvas *)wl_resource_get_user_data(resource);
	struct nemoframe_callback *cb;

	cb = (struct nemoframe_callback *)malloc(sizeof(struct nemoframe_callback));
	if (cb == NULL) {
		wl_resource_post_no_memory(resource);
		return;
	}
	memset(cb, 0, sizeof(struct nemoframe_callback));

	cb->resource = wl_resource_create(client, &wl_callback_interface, 1, callback);
	if (cb->resource == NULL) {
		wl_resource_post_no_memory(resource);
		free(cb);
		return;
	}

	wl_resource_set_implementation(cb->resource, NULL, cb, nemocanvas_unbind_frame_callback);

	wl_list_insert(canvas->pending.frame_callback_list.prev, &cb->link);
}

static void surface_set_opaque_region(struct wl_client *client, struct wl_resource *resource, struct wl_resource *region_resource)
{
	struct nemocanvas *canvas = (struct nemocanvas *)wl_resource_get_user_data(resource);
	struct nemoregion *region;

	if (region_resource != NULL) {
		region = wl_resource_get_user_data(region_resource);
		pixman_region32_copy(&canvas->pending.opaque, &region->region);
	} else {
		pixman_region32_clear(&canvas->pending.opaque);
	}
}

static void surface_set_input_region(struct wl_client *client, struct wl_resource *resource, struct wl_resource *region_resource)
{
	struct nemocanvas *canvas = (struct nemocanvas *)wl_resource_get_user_data(resource);
	struct nemoregion *region;

	if (region_resource != NULL) {
		region = wl_resource_get_user_data(region_resource);
		pixman_region32_copy(&canvas->pending.input, &region->region);
	} else {
		pixman_region32_fini(&canvas->pending.input);
		pixman_region32_init_rect(&canvas->pending.input, INT32_MIN, INT32_MIN, UINT32_MAX, UINT32_MAX);
	}
}

static void surface_commit(struct wl_client *client, struct wl_resource *resource)
{
	struct nemocanvas *canvas = (struct nemocanvas *)wl_resource_get_user_data(resource);
	struct nemosubcanvas *sub = nemosubcanvas_get_subcanvas(canvas);

	if (sub != NULL) {
		nemosubcanvas_commit(sub);
		return;
	}

	nemocanvas_commit(canvas);

	wl_list_for_each(sub, &canvas->subcanvas_list, parent_link) {
		if (sub->canvas != canvas)
			nemosubcanvas_commit_parent(sub, 0);
	}
}

static void surface_set_buffer_transform(struct wl_client *client, struct wl_resource *resource, int transform)
{
	struct nemocanvas *canvas = (struct nemocanvas *)wl_resource_get_user_data(resource);

	if (transform < 0 || transform > WL_OUTPUT_TRANSFORM_FLIPPED_270) {
		wl_resource_post_error(resource,
				WL_SURFACE_ERROR_INVALID_TRANSFORM,
				"invalid buffer transform %d", transform);
		return;
	}

	canvas->pending.buffer_viewport.buffer.transform = transform;
	canvas->pending.buffer_viewport.changed = 1;
}

static void surface_set_buffer_scale(struct wl_client *client, struct wl_resource *resource, int32_t scale)
{
	struct nemocanvas *canvas = (struct nemocanvas *)wl_resource_get_user_data(resource);

	if (scale < 1) {
		wl_resource_post_error(resource,
				WL_SURFACE_ERROR_INVALID_SCALE,
				"invalid scale %d", scale);
		return;
	}

	canvas->pending.buffer_viewport.buffer.scale = scale;
	canvas->pending.buffer_viewport.changed = 1;
}

static void surface_damage_buffer(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct nemocanvas *canvas = (struct nemocanvas *)wl_resource_get_user_data(resource);

	pixman_region32_union_rect(&canvas->pending.damage, &canvas->pending.damage, x, y, width, height);
}

static const struct wl_surface_interface surface_implementation = {
	surface_destroy,
	surface_attach,
	surface_damage,
	surface_frame,
	surface_set_opaque_region,
	surface_set_input_region,
	surface_commit,
	surface_set_buffer_transform,
	surface_set_buffer_scale,
	surface_damage_buffer
};

static void nemocanvas_apply_viewport_transform(struct nemocanvas *canvas, pixman_transform_t *transform)
{
	struct nemobuffer_viewport *vp = &canvas->buffer_viewport;
	double src_width, src_height;
	double src_x, src_y;

	if (vp->buffer.src_width == -1) {
		if (vp->canvas.width == -1)
			return;

		src_x = 0.0f;
		src_y = 0.0f;
		src_width = canvas->width_from_buffer;
		src_height = canvas->height_from_buffer;
	} else {
		src_x = vp->buffer.src_x;
		src_y = vp->buffer.src_y;
		src_width = vp->buffer.src_width;
		src_height = vp->buffer.src_height;
	}

	pixman_transform_scale(transform, NULL,
			pixman_double_to_fixed(src_width / canvas->base.width),
			pixman_double_to_fixed(src_height / canvas->base.height));
	pixman_transform_translate(transform, NULL,
			pixman_double_to_fixed(src_x),
			pixman_double_to_fixed(src_y));
}

static void nemocanvas_get_viewport_transform(struct nemocontent *content, pixman_transform_t *transform)
{
	struct nemocanvas *canvas = (struct nemocanvas *)container_of(content, struct nemocanvas, base);
	struct nemobuffer_viewport *vp = &canvas->buffer_viewport;
	pixman_fixed_t width, height;

	nemocanvas_apply_viewport_transform(canvas, transform);

	width = pixman_int_to_fixed(canvas->width_from_buffer);
	height = pixman_int_to_fixed(canvas->height_from_buffer);

	switch (vp->buffer.transform) {
		case WL_OUTPUT_TRANSFORM_FLIPPED:
		case WL_OUTPUT_TRANSFORM_FLIPPED_90:
		case WL_OUTPUT_TRANSFORM_FLIPPED_180:
		case WL_OUTPUT_TRANSFORM_FLIPPED_270:
			pixman_transform_scale(transform, NULL, pixman_int_to_fixed(-1), pixman_int_to_fixed(1));
			pixman_transform_translate(transform, NULL, width, 0);
			break;
	}

	switch (vp->buffer.transform) {
		default:
		case WL_OUTPUT_TRANSFORM_NORMAL:
		case WL_OUTPUT_TRANSFORM_FLIPPED:
			break;

		case WL_OUTPUT_TRANSFORM_90:
		case WL_OUTPUT_TRANSFORM_FLIPPED_90:
			pixman_transform_rotate(transform, NULL, 0, pixman_fixed_1);
			pixman_transform_translate(transform, NULL, height, 0);
			break;

		case WL_OUTPUT_TRANSFORM_180:
		case WL_OUTPUT_TRANSFORM_FLIPPED_180:
			pixman_transform_rotate(transform, NULL, -pixman_fixed_1, 0);
			pixman_transform_translate(transform, NULL, width, height);
			break;

		case WL_OUTPUT_TRANSFORM_270:
		case WL_OUTPUT_TRANSFORM_FLIPPED_270:
			pixman_transform_rotate(transform, NULL, 0, -pixman_fixed_1);
			pixman_transform_translate(transform, NULL, 0, width);
			break;
	}

	pixman_transform_scale(transform, NULL,
			pixman_double_to_fixed(vp->buffer.scale),
			pixman_double_to_fixed(vp->buffer.scale));
}

static int32_t nemocanvas_get_buffer_scale(struct nemocontent *content)
{
	struct nemocanvas *canvas = (struct nemocanvas *)container_of(content, struct nemocanvas, base);

	return canvas->buffer_viewport.buffer.scale;
}

static void nemocanvas_transform_to_buffer(struct nemocanvas *canvas, float sx, float sy, float *bx, float *by)
{
	struct nemobuffer_viewport *vp = &canvas->buffer_viewport;
	double src_width, src_height;
	double src_x, src_y;

	if (vp->buffer.src_width == -1) {
		if (vp->canvas.width == -1) {
			*bx = sx;
			*by = sy;
			return;
		}

		src_x = 0.0f;
		src_y = 0.0f;
		src_width = canvas->width_from_buffer;
		src_height = canvas->height_from_buffer;
	} else {
		src_x = vp->buffer.src_x;
		src_y = vp->buffer.src_y;
		src_width = vp->buffer.src_width;
		src_height = vp->buffer.src_height;
	}

	*bx = sx * src_width / canvas->base.width + src_x;
	*by = sy * src_height / canvas->base.height + src_y;
}

static void nemocanvas_transform_to_buffer_point(struct nemocontent *content, float sx, float sy, float *bx, float *by)
{
	struct nemocanvas *canvas = (struct nemocanvas *)container_of(content, struct nemocanvas, base);
	struct nemobuffer_viewport *vp = &canvas->buffer_viewport;

	nemocanvas_transform_to_buffer(canvas, sx, sy, bx, by);

	wayland_transform_point(
			canvas->width_from_buffer, canvas->height_from_buffer,
			vp->buffer.transform, vp->buffer.scale, *bx, *by, bx, by);
}

static pixman_box32_t nemocanvas_transform_to_buffer_rect(struct nemocontent *content, pixman_box32_t rect)
{
	struct nemocanvas *canvas = (struct nemocanvas *)container_of(content, struct nemocanvas, base);
	struct nemobuffer_viewport *vp = &canvas->buffer_viewport;
	float x, y;

	nemocanvas_transform_to_buffer(canvas, rect.x1, rect.y1, &x, &y);
	rect.x1 = floorf(x);
	rect.y1 = floorf(y);

	nemocanvas_transform_to_buffer(canvas, rect.x2, rect.y2, &x, &y);
	rect.x2 = floorf(x);
	rect.y2 = floorf(y);

	return wayland_transform_rect(
			canvas->width_from_buffer, canvas->height_from_buffer,
			vp->buffer.transform, vp->buffer.scale, rect);
}

static void nemocanvas_update_output(struct nemocontent *content)
{
	struct nemocanvas *canvas = (struct nemocanvas *)container_of(content, struct nemocanvas, base);
	struct nemoview *view;
	uint32_t node_mask = 0;
	uint32_t screen_mask = 0;

	wl_list_for_each(view, &canvas->view_list, link) {
		node_mask |= view->node_mask;
		screen_mask |= view->screen_mask;
	}

	pixman_region32_union_rect(&content->damage, &content->damage,
			0, 0, content->width, content->height);

	content->dirty = 1;
	content->screen_dirty = screen_mask;

	content->node_mask = node_mask;
	content->screen_mask = screen_mask;
}

static void nemocanvas_update_transform(struct nemocontent *content, int visible, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct nemocanvas *canvas = (struct nemocanvas *)container_of(content, struct nemocanvas, base);

	if (canvas->update_transform != NULL)
		canvas->update_transform(canvas, visible, x, y, width, height);
}

static void nemocanvas_update_layer(struct nemocontent *content, int visible)
{
	struct nemocanvas *canvas = (struct nemocanvas *)container_of(content, struct nemocanvas, base);

	if (canvas->update_layer != NULL)
		canvas->update_layer(canvas, visible);
}

static void nemocanvas_update_fullscreen(struct nemocontent *content, const char *id, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct nemocanvas *canvas = (struct nemocanvas *)container_of(content, struct nemocanvas, base);

	if (canvas->update_fullscreen != NULL)
		canvas->update_fullscreen(canvas, id, x, y, width, height);
}

static int nemocanvas_read_pixels(struct nemocontent *content, pixman_format_code_t format, void *pixels)
{
	struct nemocanvas *canvas = (struct nemocanvas *)container_of(content, struct nemocanvas, base);
	struct nemorenderer *renderer = canvas->compz->renderer;

	return renderer->read_canvas(renderer, canvas, format, pixels);
}

static void nemocanvas_pointer_enter(struct nemopointer *pointer, struct nemocontent *content)
{
	struct nemocanvas *canvas = (struct nemocanvas *)container_of(content, struct nemocanvas, base);
	struct wl_list *resource_list;
	struct wl_client *client = wl_resource_get_client(canvas->resource);
	struct wl_resource *resource;
	uint32_t serial;

	serial = wl_display_next_serial(pointer->seat->compz->display);

	resource_list = &pointer->seat->pointer.resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			wl_pointer_send_enter(resource, serial, canvas->resource,
					wl_fixed_from_double(pointer->sx),
					wl_fixed_from_double(pointer->sy));
		}
	}

	resource_list = &pointer->seat->pointer.nemo_resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			nemo_pointer_send_enter(resource, serial, canvas->resource,
					pointer->id,
					wl_fixed_from_double(pointer->sx),
					wl_fixed_from_double(pointer->sy));
		}
	}

	pointer->focus_serial = serial;
}

static void nemocanvas_pointer_leave(struct nemopointer *pointer, struct nemocontent *content)
{
	struct nemocanvas *canvas = (struct nemocanvas *)container_of(content, struct nemocanvas, base);
	struct wl_list *resource_list;
	struct wl_client *client = wl_resource_get_client(canvas->resource);
	struct wl_resource *resource;
	uint32_t serial;

	serial = wl_display_next_serial(pointer->seat->compz->display);

	resource_list = &pointer->seat->pointer.resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			wl_pointer_send_leave(resource, serial, canvas->resource);
		}
	}

	resource_list = &pointer->seat->pointer.nemo_resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			nemo_pointer_send_leave(resource, serial, canvas->resource, pointer->id);
		}
	}
}

static void nemocanvas_pointer_motion(struct nemopointer *pointer, struct nemocontent *content, uint32_t time, float x, float y)
{
	struct nemocanvas *canvas = (struct nemocanvas *)container_of(content, struct nemocanvas, base);
	struct wl_list *resource_list;
	struct wl_client *client = wl_resource_get_client(canvas->resource);
	struct wl_resource *resource;

	resource_list = &pointer->seat->pointer.resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			wl_pointer_send_motion(resource, time,
					wl_fixed_from_double(x),
					wl_fixed_from_double(y));
		}
	}

	resource_list = &pointer->seat->pointer.nemo_resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			nemo_pointer_send_motion(resource, time, canvas->resource,
					pointer->id,
					wl_fixed_from_double(x),
					wl_fixed_from_double(y));
		}
	}
}

static void nemocanvas_pointer_axis(struct nemopointer *pointer, struct nemocontent *content, uint32_t time, uint32_t axis, float value)
{
	struct nemocanvas *canvas = (struct nemocanvas *)container_of(content, struct nemocanvas, base);
	struct wl_list *resource_list;
	struct wl_client *client = wl_resource_get_client(canvas->resource);
	struct wl_resource *resource;

	resource_list = &pointer->seat->pointer.resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			wl_pointer_send_axis(resource, time,
					axis,
					wl_fixed_from_double(value));
		}
	}

	resource_list = &pointer->seat->pointer.nemo_resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			nemo_pointer_send_axis(resource, time, canvas->resource,
					pointer->id,
					axis,
					wl_fixed_from_double(value));
		}
	}
}

static void nemocanvas_pointer_button(struct nemopointer *pointer, struct nemocontent *content, uint32_t time, uint32_t button, uint32_t state)
{
	struct nemocanvas *canvas = (struct nemocanvas *)container_of(content, struct nemocanvas, base);
	struct wl_list *resource_list;
	struct wl_client *client = wl_resource_get_client(canvas->resource);
	struct wl_resource *resource;
	uint32_t serial;

	serial = wl_display_next_serial(pointer->seat->compz->display);

	resource_list = &pointer->seat->pointer.resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			wl_pointer_send_button(resource, serial, time, button, state);
		}
	}

	resource_list = &pointer->seat->pointer.nemo_resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			nemo_pointer_send_button(resource, serial, time, canvas->resource,
					pointer->id,
					button, state);
		}
	}
}

static void nemocanvas_keyboard_enter(struct nemokeyboard *keyboard, struct nemocontent *content)
{
	struct nemocanvas *canvas = (struct nemocanvas *)container_of(content, struct nemocanvas, base);
	struct wl_list *resource_list;
	struct wl_client *client = wl_resource_get_client(canvas->resource);
	struct wl_resource *resource;
	uint32_t serial;

	serial = wl_display_next_serial(keyboard->seat->compz->display);

	resource_list = &keyboard->seat->keyboard.resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			wl_keyboard_send_modifiers(resource, serial,
					keyboard->xkb->mods_depressed,
					keyboard->xkb->mods_latched,
					keyboard->xkb->mods_locked,
					keyboard->xkb->group);
			wl_keyboard_send_enter(resource, serial, canvas->resource, &keyboard->xkb->keys);
		}
	}

	resource_list = &keyboard->seat->keyboard.nemo_resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			nemo_keyboard_send_modifiers(resource, serial, canvas->resource,
					keyboard->id,
					keyboard->xkb->mods_depressed,
					keyboard->xkb->mods_latched,
					keyboard->xkb->mods_locked,
					keyboard->xkb->group);
			nemo_keyboard_send_enter(resource, serial, canvas->resource, keyboard->id, &keyboard->xkb->keys);
		}
	}

	keyboard->focus_serial = serial;
}

static void nemocanvas_keyboard_leave(struct nemokeyboard *keyboard, struct nemocontent *content)
{
	struct nemocanvas *canvas = (struct nemocanvas *)container_of(content, struct nemocanvas, base);
	struct wl_list *resource_list;
	struct wl_client *client = wl_resource_get_client(canvas->resource);
	struct wl_resource *resource;
	uint32_t serial;

	serial = wl_display_next_serial(keyboard->seat->compz->display);

	resource_list = &keyboard->seat->keyboard.resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			wl_keyboard_send_leave(resource, serial, canvas->resource);
		}
	}

	resource_list = &keyboard->seat->keyboard.nemo_resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			nemo_keyboard_send_leave(resource, serial, canvas->resource, keyboard->id);
		}
	}
}

static void nemocanvas_keyboard_key(struct nemokeyboard *keyboard, struct nemocontent *content, uint32_t time, uint32_t key, uint32_t state)
{
	struct nemocanvas *canvas = (struct nemocanvas *)container_of(content, struct nemocanvas, base);
	struct wl_list *resource_list;
	struct wl_client *client = wl_resource_get_client(canvas->resource);
	struct wl_resource *resource;
	uint32_t serial;

	serial = wl_display_next_serial(keyboard->seat->compz->display);

	resource_list = &keyboard->seat->keyboard.resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			wl_keyboard_send_key(resource, serial, time, key, state);
		}
	}

	resource_list = &keyboard->seat->keyboard.nemo_resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			nemo_keyboard_send_key(resource, serial, time, canvas->resource, keyboard->id, key, state);
		}
	}
}

static void nemocanvas_keyboard_modifiers(struct nemokeyboard *keyboard, struct nemocontent *content, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
	struct nemocanvas *canvas = (struct nemocanvas *)container_of(content, struct nemocanvas, base);
	struct wl_list *resource_list;
	struct wl_client *client = wl_resource_get_client(canvas->resource);
	struct wl_resource *resource;
	uint32_t serial;

	serial = wl_display_next_serial(keyboard->seat->compz->display);

	resource_list = &keyboard->seat->keyboard.resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			wl_keyboard_send_modifiers(resource, serial,
					mods_depressed,
					mods_latched,
					mods_locked,
					group);
		}
	}

	resource_list = &keyboard->seat->keyboard.nemo_resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			nemo_keyboard_send_modifiers(resource, serial, canvas->resource,
					keyboard->id,
					mods_depressed,
					mods_latched,
					mods_locked,
					group);
		}
	}
}

static void nemocanvas_keypad_enter(struct nemokeypad *keypad, struct nemocontent *content)
{
	struct nemocanvas *canvas = (struct nemocanvas *)container_of(content, struct nemocanvas, base);
	struct wl_list *resource_list;
	struct wl_client *client = wl_resource_get_client(canvas->resource);
	struct wl_resource *resource;
	uint32_t serial;

	serial = wl_display_next_serial(keypad->seat->compz->display);

	resource_list = &keypad->seat->keyboard.resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			wl_keyboard_send_modifiers(resource, serial,
					keypad->xkb->mods_depressed,
					keypad->xkb->mods_latched,
					keypad->xkb->mods_locked,
					keypad->xkb->group);
			wl_keyboard_send_enter(resource, serial, canvas->resource, &keypad->xkb->keys);
		}
	}

	resource_list = &keypad->seat->keyboard.nemo_resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			nemo_keyboard_send_modifiers(resource, serial, canvas->resource,
					keypad->id,
					keypad->xkb->mods_depressed,
					keypad->xkb->mods_latched,
					keypad->xkb->mods_locked,
					keypad->xkb->group);
			nemo_keyboard_send_enter(resource, serial, canvas->resource, keypad->id, &keypad->xkb->keys);
		}
	}

	keypad->focus_serial = serial;
}

static void nemocanvas_keypad_leave(struct nemokeypad *keypad, struct nemocontent *content)
{
	struct nemocanvas *canvas = (struct nemocanvas *)container_of(content, struct nemocanvas, base);
	struct wl_list *resource_list;
	struct wl_client *client = wl_resource_get_client(canvas->resource);
	struct wl_resource *resource;
	uint32_t serial;

	serial = wl_display_next_serial(keypad->seat->compz->display);

	resource_list = &keypad->seat->keyboard.resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			wl_keyboard_send_leave(resource, serial, canvas->resource);
		}
	}

	resource_list = &keypad->seat->keyboard.nemo_resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			nemo_keyboard_send_leave(resource, serial, canvas->resource, keypad->id);
		}
	}
}

static void nemocanvas_keypad_key(struct nemokeypad *keypad, struct nemocontent *content, uint32_t time, uint32_t key, uint32_t state)
{
	struct nemocanvas *canvas = (struct nemocanvas *)container_of(content, struct nemocanvas, base);
	struct wl_list *resource_list;
	struct wl_client *client = wl_resource_get_client(canvas->resource);
	struct wl_resource *resource;
	uint32_t serial;

	serial = wl_display_next_serial(keypad->seat->compz->display);

	resource_list = &keypad->seat->keyboard.resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			wl_keyboard_send_key(resource, serial, time, key, state);
		}
	}

	resource_list = &keypad->seat->keyboard.nemo_resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			nemo_keyboard_send_key(resource, serial, time, canvas->resource, keypad->id, key, state);
		}
	}
}

static void nemocanvas_keypad_modifiers(struct nemokeypad *keypad, struct nemocontent *content, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
	struct nemocanvas *canvas = (struct nemocanvas *)container_of(content, struct nemocanvas, base);
	struct wl_list *resource_list;
	struct wl_client *client = wl_resource_get_client(canvas->resource);
	struct wl_resource *resource;
	uint32_t serial;

	serial = wl_display_next_serial(keypad->seat->compz->display);

	resource_list = &keypad->seat->keyboard.resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			wl_keyboard_send_modifiers(resource, serial,
					mods_depressed,
					mods_latched,
					mods_locked,
					group);
		}
	}

	resource_list = &keypad->seat->keyboard.nemo_resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			nemo_keyboard_send_modifiers(resource, serial, canvas->resource,
					keypad->id,
					mods_depressed,
					mods_latched,
					mods_locked,
					group);
		}
	}
}

static void nemocanvas_touch_down(struct touchpoint *tp, struct nemocontent *content, uint32_t time, uint64_t touchid, float x, float y, float gx, float gy)
{
	struct nemocanvas *canvas = (struct nemocanvas *)container_of(content, struct nemocanvas, base);
	struct wl_list *resource_list;
	struct wl_client *client = wl_resource_get_client(canvas->resource);
	struct wl_resource *resource;
	uint32_t serial, nserial;
	int done = 0;

	serial = wl_display_next_serial(tp->touch->seat->compz->display);

	resource_list = &tp->touch->seat->touch.resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			if (canvas->touchid0 == 0) {
				canvas->touchid0 = touchid;
				touchid = 0;
			}

			wl_touch_send_down(resource, serial, time,
					canvas->resource,
					touchid,
					wl_fixed_from_double(x),
					wl_fixed_from_double(y));

			done = 1;
		}
	}

	resource_list = &tp->touch->seat->touch.nemo_resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			nemo_touch_send_down(resource, serial, time,
					canvas->resource,
					touchid,
					wl_fixed_from_double(x),
					wl_fixed_from_double(y),
					wl_fixed_from_double(gx),
					wl_fixed_from_double(gy));

			done = 1;
		}
	}

#ifdef NEMOUX_WITH_TOUCH_TO_POINTER
	if (done == 0) {
		resource_list = &tp->touch->seat->pointer.resource_list;

		nserial = wl_display_next_serial(tp->touch->seat->compz->display);

		wl_resource_for_each(resource, resource_list) {
			if (wl_resource_get_client(resource) == client) {
				wl_pointer_send_enter(resource, serial, canvas->resource,
						wl_fixed_from_double(x),
						wl_fixed_from_double(y));
				wl_pointer_send_motion(resource, time,
						wl_fixed_from_double(x),
						wl_fixed_from_double(y));
				wl_pointer_send_button(resource, nserial, time, BTN_LEFT, WL_POINTER_BUTTON_STATE_PRESSED);
			}
		}

		resource_list = &tp->touch->seat->pointer.nemo_resource_list;

		wl_resource_for_each(resource, resource_list) {
			if (wl_resource_get_client(resource) == client) {
				nemo_pointer_send_enter(resource, serial, canvas->resource,
						touchid,
						wl_fixed_from_double(x),
						wl_fixed_from_double(y));
				nemo_pointer_send_motion(resource, time, canvas->resource,
						touchid,
						wl_fixed_from_double(x),
						wl_fixed_from_double(y));
				nemo_pointer_send_button(resource, nserial, time, canvas->resource,
						touchid,
						BTN_LEFT, WL_POINTER_BUTTON_STATE_PRESSED);
			}
		}
	}
#endif
}

static void nemocanvas_touch_up(struct touchpoint *tp, struct nemocontent *content, uint32_t time, uint64_t touchid)
{
	struct nemocanvas *canvas = (struct nemocanvas *)container_of(content, struct nemocanvas, base);
	struct wl_list *resource_list;
	struct wl_client *client = wl_resource_get_client(canvas->resource);
	struct wl_resource *resource;
	uint32_t serial, nserial;
	int done = 0;

	serial = wl_display_next_serial(tp->touch->seat->compz->display);

	resource_list = &tp->touch->seat->touch.resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			if (canvas->touchid0 == touchid) {
				canvas->touchid0 = 0;
				touchid = 0;
			}

			wl_touch_send_up(resource, serial, time, touchid);

			done = 1;
		}
	}

	resource_list = &tp->touch->seat->touch.nemo_resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			nemo_touch_send_up(resource, serial, time, canvas->resource, touchid);

			done = 1;
		}
	}

#ifdef NEMOUX_WITH_TOUCH_TO_POINTER
	if (done == 0) {
		resource_list = &tp->touch->seat->pointer.resource_list;

		nserial = wl_display_next_serial(tp->touch->seat->compz->display);

		wl_resource_for_each(resource, resource_list) {
			if (wl_resource_get_client(resource) == client) {
				wl_pointer_send_button(resource, serial, time, BTN_LEFT, WL_POINTER_BUTTON_STATE_RELEASED);
				wl_pointer_send_leave(resource, nserial, canvas->resource);
			}
		}

		resource_list = &tp->touch->seat->pointer.nemo_resource_list;

		wl_resource_for_each(resource, resource_list) {
			if (wl_resource_get_client(resource) == client) {
				nemo_pointer_send_button(resource, serial, time, canvas->resource,
						touchid,
						BTN_LEFT, WL_POINTER_BUTTON_STATE_RELEASED);
				nemo_pointer_send_leave(resource, nserial, canvas->resource, touchid);
			}
		}
	}
#endif
}

static void nemocanvas_touch_motion(struct touchpoint *tp, struct nemocontent *content, uint32_t time, uint64_t touchid, float x, float y, float gx, float gy)
{
	struct nemocanvas *canvas = (struct nemocanvas *)container_of(content, struct nemocanvas, base);
	struct wl_list *resource_list;
	struct wl_client *client = wl_resource_get_client(canvas->resource);
	struct wl_resource *resource;
	int done = 0;

	resource_list = &tp->touch->seat->touch.resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			if (canvas->touchid0 == touchid) {
				touchid = 0;
			}

			wl_touch_send_motion(resource, time,
					touchid,
					wl_fixed_from_double(x),
					wl_fixed_from_double(y));

			done = 1;
		}
	}

	resource_list = &tp->touch->seat->touch.nemo_resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			nemo_touch_send_motion(resource, time,
					canvas->resource,
					touchid,
					wl_fixed_from_double(x),
					wl_fixed_from_double(y),
					wl_fixed_from_double(gx),
					wl_fixed_from_double(gy));

			done = 1;
		}
	}

#ifdef NEMOUX_WITH_TOUCH_TO_POINTER
	if (done == 0) {
		resource_list = &tp->touch->seat->pointer.resource_list;

		wl_resource_for_each(resource, resource_list) {
			if (wl_resource_get_client(resource) == client) {
				wl_pointer_send_motion(resource, time,
						wl_fixed_from_double(x),
						wl_fixed_from_double(y));
			}
		}

		resource_list = &tp->touch->seat->pointer.nemo_resource_list;

		wl_resource_for_each(resource, resource_list) {
			if (wl_resource_get_client(resource) == client) {
				nemo_pointer_send_motion(resource, time, canvas->resource,
						touchid,
						wl_fixed_from_double(x),
						wl_fixed_from_double(y));
			}
		}
	}
#endif
}

static void nemocanvas_touch_pressure(struct touchpoint *tp, struct nemocontent *content, uint32_t time, uint64_t touchid, float p)
{
	struct nemocanvas *canvas = (struct nemocanvas *)container_of(content, struct nemocanvas, base);
	struct wl_list *resource_list;
	struct wl_client *client = wl_resource_get_client(canvas->resource);
	struct wl_resource *resource;

	resource_list = &tp->touch->seat->touch.nemo_resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			nemo_touch_send_pressure(resource, time,
					canvas->resource,
					touchid,
					wl_fixed_from_double(p));
		}
	}
}

static void nemocanvas_touch_frame(struct touchpoint *tp, struct nemocontent *content)
{
	struct nemocanvas *canvas = (struct nemocanvas *)container_of(content, struct nemocanvas, base);
	struct wl_list *resource_list;
	struct wl_client *client = wl_resource_get_client(canvas->resource);
	struct wl_resource *resource;

	resource_list = &tp->touch->seat->touch.resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			wl_touch_send_frame(resource);
		}
	}

	resource_list = &tp->touch->seat->touch.nemo_resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == client) {
			nemo_touch_send_frame(resource);
		}
	}
}

struct nemocanvas *nemocanvas_create(struct wl_client *client, struct wl_resource *compositor_resource, uint32_t id)
{
	struct nemocompz *compz = (struct nemocompz *)wl_resource_get_user_data(compositor_resource);
	struct nemocanvas *canvas;

	canvas = (struct nemocanvas *)malloc(sizeof(struct nemocanvas));
	if (canvas == NULL) {
		wl_resource_post_no_memory(compositor_resource);
		return NULL;
	}
	memset(canvas, 0, sizeof(struct nemocanvas));

	wl_signal_init(&canvas->destroy_signal);
	wl_signal_init(&canvas->damage_signal);

	canvas->compz = compz;
	canvas->resource = NULL;

	canvas->base.get_viewport_transform = nemocanvas_get_viewport_transform;
	canvas->base.get_buffer_scale = nemocanvas_get_buffer_scale;
	canvas->base.transform_to_buffer_point = nemocanvas_transform_to_buffer_point;
	canvas->base.transform_to_buffer_rect = nemocanvas_transform_to_buffer_rect;

	canvas->base.update_output = nemocanvas_update_output;
	canvas->base.update_transform = nemocanvas_update_transform;
	canvas->base.update_layer = nemocanvas_update_layer;
	canvas->base.update_fullscreen = nemocanvas_update_fullscreen;
	canvas->base.read_pixels = nemocanvas_read_pixels;

	canvas->base.pointer_enter = nemocanvas_pointer_enter;
	canvas->base.pointer_leave = nemocanvas_pointer_leave;
	canvas->base.pointer_motion = nemocanvas_pointer_motion;
	canvas->base.pointer_axis = nemocanvas_pointer_axis;
	canvas->base.pointer_button = nemocanvas_pointer_button;

	canvas->base.keyboard_enter = nemocanvas_keyboard_enter;
	canvas->base.keyboard_leave = nemocanvas_keyboard_leave;
	canvas->base.keyboard_key = nemocanvas_keyboard_key;
	canvas->base.keyboard_modifiers = nemocanvas_keyboard_modifiers;

	canvas->base.keypad_enter = nemocanvas_keypad_enter;
	canvas->base.keypad_leave = nemocanvas_keypad_leave;
	canvas->base.keypad_key = nemocanvas_keypad_key;
	canvas->base.keypad_modifiers = nemocanvas_keypad_modifiers;

	canvas->base.touch_down = nemocanvas_touch_down;
	canvas->base.touch_up = nemocanvas_touch_up;
	canvas->base.touch_motion = nemocanvas_touch_motion;
	canvas->base.touch_pressure = nemocanvas_touch_pressure;
	canvas->base.touch_frame = nemocanvas_touch_frame;

	canvas->buffer_viewport.buffer.transform = WL_OUTPUT_TRANSFORM_NORMAL;
	canvas->buffer_viewport.buffer.scale = 1;
	canvas->buffer_viewport.buffer.src_width = -1;
	canvas->buffer_viewport.canvas.width = -1;

	nemocanvas_state_prepare(&canvas->pending);

	canvas->viewport_resource = NULL;

	wl_list_init(&canvas->link);
	wl_list_insert(&compz->canvas_list, &canvas->link);

	wl_list_init(&canvas->feedback_link);

	wl_list_init(&canvas->view_list);
	wl_list_init(&canvas->frame_callback_list);
	wl_list_init(&canvas->feedback_list);

	wl_list_init(&canvas->subcanvas_list);

	nemocontent_prepare(&canvas->base, compz->nodemax);

	canvas->resource = wl_resource_create(client, &wl_surface_interface, wl_resource_get_version(compositor_resource), id);
	if (canvas->resource == NULL) {
		nemocanvas_destroy(canvas);
		wl_resource_post_no_memory(compositor_resource);
		return NULL;
	}

	wl_resource_set_implementation(canvas->resource, &surface_implementation, canvas, nemocanvas_unbind_canvas);

	wl_signal_emit(&compz->create_surface_signal, canvas);

	return canvas;
}

void nemocanvas_destroy(struct nemocanvas *canvas)
{
	struct nemoframe_callback *cb, *cnext;
	struct nemoview *view, *nview;

	wl_signal_emit(&canvas->destroy_signal, &canvas->resource);

	assert(wl_list_empty(&canvas->subcanvas_list));

	wl_list_for_each_safe(view, nview, &canvas->view_list, link) {
		wl_list_remove(&view->link);
		wl_list_init(&view->link);
	}

	wl_list_remove(&canvas->link);
	wl_list_remove(&canvas->feedback_link);

	nemocanvas_state_finish(&canvas->pending);

	nemocontent_finish(&canvas->base);

	nemobuffer_reference(&canvas->buffer_reference, NULL);

	wl_list_for_each_safe(cb, cnext, &canvas->frame_callback_list, link)
		wl_resource_destroy(cb->resource);

	nemopresentation_discard_feedback_list(&canvas->feedback_list);

	free(canvas);
}

void nemocanvas_size_from_buffer(struct nemocanvas *canvas)
{
	struct nemobuffer_viewport *vp = &canvas->buffer_viewport;
	int32_t width, height;

	if (!canvas->buffer_reference.buffer) {
		canvas->width_from_buffer = 0;
		canvas->height_from_buffer = 0;
		return;
	}

	switch (vp->buffer.transform) {
		case WL_OUTPUT_TRANSFORM_90:
		case WL_OUTPUT_TRANSFORM_270:
		case WL_OUTPUT_TRANSFORM_FLIPPED_90:
		case WL_OUTPUT_TRANSFORM_FLIPPED_270:
			width = canvas->buffer_reference.buffer->height / vp->buffer.scale;
			height = canvas->buffer_reference.buffer->width / vp->buffer.scale;
			break;

		default:
			width = canvas->buffer_reference.buffer->width / vp->buffer.scale;
			height = canvas->buffer_reference.buffer->height / vp->buffer.scale;
			break;
	}

	canvas->width_from_buffer = width;
	canvas->height_from_buffer = height;
}

void nemocanvas_set_size(struct nemocanvas *canvas, int32_t width, int32_t height)
{
	struct nemoview *view;

	if (canvas->base.width == width && canvas->base.height == height)
		return;

	canvas->base.width = width;
	canvas->base.height = height;

	wl_list_for_each(view, &canvas->view_list, link) {
		nemoview_transform_dirty(view);
	}
}

void nemocanvas_update_size(struct nemocanvas *canvas)
{
	struct nemobuffer_viewport *vp = &canvas->buffer_viewport;
	int32_t width, height;

	width = canvas->width_from_buffer;
	height = canvas->height_from_buffer;

	if (width != 0 && vp->canvas.width != -1) {
		nemocanvas_set_size(canvas, vp->canvas.width, vp->canvas.height);
		return;
	}

	if (width != 0 && vp->buffer.src_width != -1) {
		int32_t w = vp->buffer.src_width;
		int32_t h = vp->buffer.src_height;

		nemocanvas_set_size(canvas, w ? w : 1, h ? h : 1);
		return;
	}

	nemocanvas_set_size(canvas, width, height);
}

void nemocanvas_schedule_repaint(struct nemocanvas *canvas)
{
	struct nemoscreen *screen;

	wl_list_for_each(screen, &canvas->compz->screen_list, link) {
		if (canvas->base.screen_mask & (1 << screen->id))
			nemoscreen_schedule_repaint(screen);
	}
}

void nemocanvas_commit_state(struct nemocanvas *canvas, struct nemocanvas_state *state)
{
	pixman_region32_t opaque;
	pixman_box32_t *extents;

	canvas->buffer_viewport = state->buffer_viewport;

	if (state->newly_attached) {
		nemobuffer_reference(&canvas->buffer_reference, state->buffer);

		if (state->buffer != NULL) {
			struct nemorenderer *renderer = canvas->compz->renderer;

			renderer->prepare_buffer(renderer, state->buffer);
		}

		nemocanvas_size_from_buffer(canvas);

		nemopresentation_discard_feedback_list(&canvas->feedback_list);
	}
	nemocanvas_state_set_buffer(state, NULL);

	if (state->newly_attached || state->buffer_viewport.changed) {
		nemocanvas_update_size(canvas);
		if (canvas->configure != NULL)
			canvas->configure(canvas, state->sx, state->sy);
	}

	state->sx = 0;
	state->sy = 0;
	state->newly_attached = 0;
	state->buffer_viewport.changed = 0;

	pixman_region32_union(&canvas->base.damage, &canvas->base.damage, &state->damage);
	pixman_region32_intersect_rect(&canvas->base.damage, &canvas->base.damage,
			0, 0, canvas->base.width, canvas->base.height);
	pixman_region32_clear(&state->damage);

	extents = pixman_region32_extents(&canvas->base.damage);
	if (extents->x2 - extents->x1 > 0 && extents->y2 - extents->y1 > 0) {
		canvas->base.dirty = 1;
		canvas->base.screen_dirty = canvas->base.screen_mask;

		canvas->frame_damage = canvas->frame_damage + (extents->x2 - extents->x1) * (extents->y2 - extents->y1);
		canvas->frame_count++;
	}

	pixman_region32_init(&opaque);
	pixman_region32_intersect_rect(&opaque, &state->opaque,
			0, 0, canvas->base.width, canvas->base.height);

	if (!pixman_region32_equal(&opaque, &canvas->base.opaque)) {
		struct nemoview *view;

		pixman_region32_copy(&canvas->base.opaque, &opaque);

		wl_list_for_each(view, &canvas->view_list, link) {
			nemoview_transform_dirty(view);
		}
	}

	pixman_region32_fini(&opaque);

	if (canvas->base.region.has_input == 0) {
		pixman_region32_intersect_rect(&canvas->base.input, &state->input,
				0, 0, canvas->base.width + 1, canvas->base.height + 1);
	} else {
		pixman_region32_intersect_rect(&canvas->base.input, &canvas->base.region.input,
				0, 0, canvas->base.width + 1, canvas->base.height + 1);
	}

	wl_list_insert_list(&canvas->frame_callback_list, &state->frame_callback_list);
	wl_list_init(&state->frame_callback_list);

	wl_list_insert_list(&canvas->feedback_list, &state->feedback_list);
	wl_list_init(&state->feedback_list);

	if (!wl_list_empty(&canvas->feedback_list)) {
		wl_list_remove(&canvas->feedback_link);
		wl_list_insert(&canvas->compz->feedback_list, &canvas->feedback_link);
	}
}

void nemocanvas_commit(struct nemocanvas *canvas)
{
	nemocanvas_commit_state(canvas, &canvas->pending);

	nemocanvas_schedule_repaint(canvas);
}

void nemocanvas_reset_pending_buffer(struct nemocanvas *canvas)
{
	nemocanvas_state_set_buffer(&canvas->pending, NULL);

	canvas->pending.sx = 0;
	canvas->pending.sy = 0;
	canvas->pending.newly_attached = 0;
	canvas->pending.buffer_viewport.changed = 0;
}

void nemocanvas_flush_damage(struct nemocanvas *canvas)
{
	struct rendernode *node;

	if (!canvas->base.dirty)
		return;

	wl_list_for_each(node, &canvas->compz->render_list, link) {
		if (canvas->base.node_mask & (1 << node->id)) {
			if (node->pixman != NULL) {
				struct nemorenderer *renderer = node->pixman;

				if (canvas->buffer_reference.buffer != renderer->get_canvas_buffer(renderer, canvas)) {
					renderer->attach_canvas(renderer, canvas);
				}

				renderer->flush_canvas(renderer, canvas);
			}
			if (node->opengl != NULL) {
				struct nemorenderer *renderer = node->opengl;

				if (canvas->buffer_reference.buffer != renderer->get_canvas_buffer(renderer, canvas)) {
					renderer->attach_canvas(renderer, canvas);
				}

				renderer->flush_canvas(renderer, canvas);
			}
		}
	}

	wl_signal_emit(&canvas->damage_signal, canvas);
}

struct nemoview *nemocanvas_get_default_view(struct nemocanvas *canvas)
{
	if (wl_list_empty(&canvas->view_list))
		return NULL;

	return (struct nemoview *)container_of(canvas->view_list.next, struct nemoview, link);
}
