#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <assert.h>
#include <wayland-server.h>
#include <wayland-scaler-server-protocol.h>

#include <compz.h>
#include <canvas.h>
#include <scaler.h>

static void viewport_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void viewport_set(struct wl_client *client,
		struct wl_resource *resource,
		wl_fixed_t src_x,
		wl_fixed_t src_y,
		wl_fixed_t src_width,
		wl_fixed_t src_height,
		int32_t dst_width,
		int32_t dst_height)
{
	struct nemocanvas *canvas = (struct nemocanvas *)wl_resource_get_user_data(resource);

	assert(canvas->viewport_resource != NULL);

	if (wl_fixed_to_double(src_width) < 0 ||
			wl_fixed_to_double(src_height) < 0) {
		wl_resource_post_error(resource,
				WL_VIEWPORT_ERROR_BAD_VALUE,
				"source dimensions must be non-negative (%fx%f)",
				wl_fixed_to_double(src_width),
				wl_fixed_to_double(src_height));
		return;
	}

	if (dst_width <= 0 || dst_height <= 0) {
		wl_resource_post_error(resource,
				WL_VIEWPORT_ERROR_BAD_VALUE,
				"destination dimensions must be non-negative (%dx%d)",
				dst_width, dst_height);
		return;
	}

	canvas->pending.buffer_viewport.buffer.src_x = wl_fixed_to_double(src_x);
	canvas->pending.buffer_viewport.buffer.src_y = wl_fixed_to_double(src_y);
	canvas->pending.buffer_viewport.buffer.src_width = wl_fixed_to_double(src_width);
	canvas->pending.buffer_viewport.buffer.src_height = wl_fixed_to_double(src_height);
	canvas->pending.buffer_viewport.canvas.width = dst_width;
	canvas->pending.buffer_viewport.canvas.height = dst_height;
	canvas->pending.buffer_viewport.changed = 1;
}

static void viewport_set_source(struct wl_client *client, struct wl_resource *resource, wl_fixed_t src_x, wl_fixed_t src_y, wl_fixed_t src_width, wl_fixed_t src_height)
{
	struct nemocanvas *canvas = (struct nemocanvas *)wl_resource_get_user_data(resource);

	assert(canvas->viewport_resource != NULL);

	if (src_width == wl_fixed_from_int(-1) &&
			src_height == wl_fixed_from_int(-1)) {
		canvas->pending.buffer_viewport.buffer.src_width = -1;
		canvas->pending.buffer_viewport.changed = 1;
		return;
	}

	if (wl_fixed_to_double(src_width) <= 0 ||
			wl_fixed_to_double(src_height) <= 0) {
		wl_resource_post_error(resource,
				WL_VIEWPORT_ERROR_BAD_VALUE,
				"source dimensions must be non-negative (%fx%f)",
				wl_fixed_to_double(src_width),
				wl_fixed_to_double(src_height));
		return;
	}

	canvas->pending.buffer_viewport.buffer.src_x = wl_fixed_to_double(src_x);
	canvas->pending.buffer_viewport.buffer.src_y = wl_fixed_to_double(src_y);
	canvas->pending.buffer_viewport.buffer.src_width = wl_fixed_to_double(src_width);
	canvas->pending.buffer_viewport.buffer.src_height = wl_fixed_to_double(src_height);
	canvas->pending.buffer_viewport.changed = 1;
}

static void viewport_set_destination(struct wl_client *client,
		struct wl_resource *resource,
		int32_t dst_width,
		int32_t dst_height)
{
	struct nemocanvas *canvas = (struct nemocanvas *)wl_resource_get_user_data(resource);

	assert(canvas->viewport_resource != NULL);

	if (dst_width == -1 && dst_height == -1) {
		canvas->pending.buffer_viewport.canvas.width = -1;
		canvas->pending.buffer_viewport.changed = 1;
		return;
	}

	if (dst_width <= 0 || dst_height <= 0) {
		wl_resource_post_error(resource,
				WL_VIEWPORT_ERROR_BAD_VALUE,
				"destination dimensions must be non-negative (%dx%d)",
				dst_width, dst_height);
		return;
	}

	canvas->pending.buffer_viewport.canvas.width = dst_width;
	canvas->pending.buffer_viewport.canvas.height = dst_height;
	canvas->pending.buffer_viewport.changed = 1;
}

static const struct wl_viewport_interface viewport_implementation = {
	viewport_destroy,
	viewport_set,
	viewport_set_source,
	viewport_set_destination
};

static void nemoscaler_unbind_viewport(struct wl_resource *resource)
{
	struct nemocanvas *canvas = (struct nemocanvas *)wl_resource_get_user_data(resource);

	canvas->viewport_resource = NULL;
	canvas->pending.buffer_viewport.buffer.src_width = -1;
	canvas->pending.buffer_viewport.canvas.width = -1;
	canvas->pending.buffer_viewport.changed = 1;
}

static void scaler_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void scaler_get_viewport(struct wl_client *client, struct wl_resource *scaler, uint32_t id, struct wl_resource *surface_resource)
{
	struct nemocanvas *canvas = (struct nemocanvas *)wl_resource_get_user_data(surface_resource);
	struct wl_resource *resource;

	if (canvas->viewport_resource) {
		wl_resource_post_error(scaler, WL_SCALER_ERROR_VIEWPORT_EXISTS, "a viewport for that surface already exists");
		return;
	}

	resource = wl_resource_create(client, &wl_viewport_interface, 1, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource, &viewport_implementation, canvas, nemoscaler_unbind_viewport);

	canvas->viewport_resource = resource;
}

static const struct wl_scaler_interface scaler_implementation = {
	scaler_destroy,
	scaler_get_viewport
};

int nemoscaler_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct wl_resource *resource;

	resource = wl_resource_create(client, &wl_region_interface, version, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return -1;
	}

	wl_resource_set_implementation(resource, &scaler_implementation, NULL, NULL);

	return 0;
}
