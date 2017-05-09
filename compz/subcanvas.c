#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <assert.h>
#include <wayland-server.h>

#include <compz.h>
#include <canvas.h>
#include <view.h>
#include <subcompz.h>
#include <subcanvas.h>
#include <screen.h>
#include <presentation.h>
#include <nemomisc.h>

static void nemosubcanvas_configure(struct nemocanvas *canvas, int32_t dx, int32_t dy)
{
	struct nemosubcanvas *sub = nemosubcanvas_get_subcanvas(canvas);

	assert(sub);

	if (!nemoview_has_state(sub->view, NEMOVIEW_MAP_STATE)) {
		nemoview_set_position(sub->view, sub->view->geometry.x + dx, sub->view->geometry.y + dy);
		nemoview_update_transform(sub->view);
		nemoview_damage_below(sub->view);
	}
}

static void subsurface_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void subsurface_set_position(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y)
{
	struct nemosubcanvas *sub = (struct nemosubcanvas *)wl_resource_get_user_data(resource);

	if (sub == NULL)
		return;

	nemoview_set_position(sub->view, x, y);
	nemoview_update_transform(sub->view);
	nemoview_damage_below(sub->view);
}

static void subsurface_place_above(struct wl_client *client, struct wl_resource *resource, struct wl_resource *sibling_resource)
{
	struct nemosubcanvas *sub = (struct nemosubcanvas *)wl_resource_get_user_data(resource);
	struct nemocanvas *canvas = (struct nemocanvas *)wl_resource_get_user_data(sibling_resource);
	struct nemosubcanvas *sibling;

	if (sub == NULL)
		return;

	sibling = nemosubcanvas_get_subcanvas(canvas);
	if (sibling == NULL) {
		wl_list_remove(&sub->parent_link);
		wl_list_insert(&canvas->subcanvas_list, &sub->parent_link);
	} else {
		wl_list_remove(&sub->parent_link);
		wl_list_insert(sibling->parent_link.prev, &sub->parent_link);
	}
}

static void subsurface_place_below(struct wl_client *client, struct wl_resource *resource, struct wl_resource *sibling_resource)
{
	struct nemosubcanvas *sub = (struct nemosubcanvas *)wl_resource_get_user_data(resource);
	struct nemocanvas *canvas = (struct nemocanvas *)wl_resource_get_user_data(sibling_resource);
	struct nemosubcanvas *sibling;

	if (sub == NULL)
		return;

	sibling = nemosubcanvas_get_subcanvas(canvas);
	if (sibling == NULL) {
		wl_list_remove(&sub->parent_link);
		wl_list_insert(&canvas->subcanvas_list, &sub->parent_link);
	} else {
		wl_list_remove(&sub->parent_link);
		wl_list_insert(&sibling->parent_link, &sub->parent_link);
	}
}

static void subsurface_set_sync(struct wl_client *client, struct wl_resource *resource)
{
	struct nemosubcanvas *sub = (struct nemosubcanvas *)wl_resource_get_user_data(resource);

	if (sub != NULL)
		sub->sync = 1;
}

static void subsurface_set_desync(struct wl_client *client, struct wl_resource *resource)
{
	struct nemosubcanvas *sub = (struct nemosubcanvas *)wl_resource_get_user_data(resource);

	if (sub != NULL) {
		sub->sync = 0;

		if (!nemosubcanvas_is_sync(sub))
			nemosubcanvas_commit_sync(sub);
	}
}

static const struct wl_subsurface_interface subsurface_implementation = {
	subsurface_destroy,
	subsurface_set_position,
	subsurface_place_above,
	subsurface_place_below,
	subsurface_set_sync,
	subsurface_set_desync
};

static void nemosubcanvas_unbind_subcanvas(struct wl_resource *resource)
{
	struct nemosubcanvas *sub = (struct nemosubcanvas *)wl_resource_get_user_data(resource);

	if (sub != NULL)
		nemosubcanvas_destroy(sub);
}

struct nemosubcanvas *nemosubcanvas_create(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface_resource, struct wl_resource *parent_resource)
{
	struct nemocanvas *canvas = (struct nemocanvas *)wl_resource_get_user_data(surface_resource);
	struct nemocanvas *parent = (struct nemocanvas *)wl_resource_get_user_data(parent_resource);
	struct nemosubcanvas *sub;

	if (canvas == parent) {
		wl_resource_post_error(resource,
				WL_SUBCOMPOSITOR_ERROR_BAD_SURFACE,
				"wl_surface@%d cannot be its own parent",
				wl_resource_get_id(surface_resource));
		return NULL;
	}

	if (nemosubcanvas_get_subcanvas(canvas)) {
		wl_resource_post_error(resource,
				WL_SUBCOMPOSITOR_ERROR_BAD_SURFACE,
				"wl_surface@%d is already a subsurface",
				wl_resource_get_id(surface_resource));
		return NULL;
	}

	if (canvas->configure != NULL) {
		wl_resource_post_error(resource,
				WL_SUBCOMPOSITOR_ERROR_BAD_SURFACE,
				"wl_surface@%d already has a role",
				wl_resource_get_id(surface_resource));
		return NULL;
	}

	if (nemosubcanvas_get_main_canvas(parent) == canvas) {
		wl_resource_post_error(resource,
				WL_SUBCOMPOSITOR_ERROR_BAD_SURFACE,
				"wl_surface@%d is an ancestor of parent",
				wl_resource_get_id(surface_resource));
		return NULL;
	}

	sub = (struct nemosubcanvas *)malloc(sizeof(struct nemosubcanvas));
	if (sub == NULL) {
		wl_resource_post_no_memory(resource);
		return NULL;
	}
	memset(sub, 0, sizeof(struct nemosubcanvas));

	sub->resource = wl_resource_create(client, &wl_subsurface_interface, 1, id);
	if (sub->resource == NULL) {
		wl_resource_post_no_memory(resource);
		free(sub);
		return NULL;
	}

	wl_resource_set_implementation(sub->resource, &subsurface_implementation, sub, nemosubcanvas_unbind_subcanvas);
	nemosubcanvas_link_canvas(sub, canvas);
	nemosubcanvas_link_parent(sub, parent);
	nemocanvas_state_prepare(&sub->cached);
	sub->buffer_reference.buffer = NULL;
	sub->sync = 1;

	sub->view = nemoview_create(canvas->compz);
	if (sub->view == NULL) {
		wl_resource_post_error(resource,
				WL_SUBCOMPOSITOR_ERROR_BAD_SURFACE,
				"wl_surface@%d is failed to create view",
				wl_resource_get_id(surface_resource));
		return NULL;
	}

	nemoview_attach_canvas(sub->view, canvas);

	nemoview_set_parent(sub->view, nemocanvas_get_default_view(parent));

	canvas->configure = nemosubcanvas_configure;
	canvas->configure_private = sub;

	return sub;
}

void nemosubcanvas_destroy(struct nemosubcanvas *sub)
{
	if (sub->resource) {
		if (sub->parent)
			nemosubcanvas_unlink_parent(sub);

		nemocanvas_state_finish(&sub->cached);
		nemobuffer_reference(&sub->buffer_reference, NULL);

		sub->canvas->configure = NULL;
		sub->canvas->configure_private = NULL;
	} else {
		wl_list_remove(&sub->parent_link);
	}

	wl_list_remove(&sub->canvas_destroy_listener.link);

	nemoview_destroy(sub->view);

	free(sub);
}

struct nemosubcanvas *nemosubcanvas_get_subcanvas(struct nemocanvas *canvas)
{
	if (canvas->configure == nemosubcanvas_configure)
		return canvas->configure_private;

	return NULL;
}

struct nemocanvas *nemosubcanvas_get_main_canvas(struct nemocanvas *canvas)
{
	struct nemosubcanvas *sub;

	while (canvas && (sub = nemosubcanvas_get_subcanvas(canvas)))
		canvas = sub->parent;

	return canvas;
}

static void nemosubcanvas_handle_canvas_destroy(struct wl_listener *listener, void *data)
{
	struct nemosubcanvas *sub = (struct nemosubcanvas *)container_of(listener, struct nemosubcanvas, canvas_destroy_listener);

	if (sub->resource)
		wl_resource_set_user_data(sub->resource, NULL);

	nemosubcanvas_destroy(sub);
}

void nemosubcanvas_link_canvas(struct nemosubcanvas *sub, struct nemocanvas *canvas)
{
	sub->canvas = canvas;
	sub->canvas_destroy_listener.notify = nemosubcanvas_handle_canvas_destroy;
	wl_signal_add(&canvas->destroy_signal, &sub->canvas_destroy_listener);
}

static void nemosubcanvas_handle_parent_destroy(struct wl_listener *listener, void *data)
{
	struct nemosubcanvas *sub = (struct nemosubcanvas *)container_of(listener, struct nemosubcanvas, parent_destroy_listener);

	nemosubcanvas_unlink_parent(sub);
}

void nemosubcanvas_link_parent(struct nemosubcanvas *sub, struct nemocanvas *parent)
{
	sub->parent = parent;
	sub->parent_destroy_listener.notify = nemosubcanvas_handle_parent_destroy;
	wl_signal_add(&parent->destroy_signal, &sub->parent_destroy_listener);

	wl_list_insert(&parent->subcanvas_list, &sub->parent_link);
}

void nemosubcanvas_unlink_parent(struct nemosubcanvas *sub)
{
	wl_list_remove(&sub->parent_link);
	wl_list_remove(&sub->parent_destroy_listener.link);

	sub->parent = NULL;
}

void nemosubcanvas_commit_from_cache(struct nemosubcanvas *sub)
{
	struct nemocanvas *canvas = sub->canvas;

	nemocanvas_commit_state(canvas, &sub->cached);
	nemobuffer_reference(&sub->buffer_reference, NULL);

	sub->has_cached_data = 0;
}

void nemosubcanvas_commit_to_cache(struct nemosubcanvas *sub)
{
	struct nemocanvas *canvas = sub->canvas;

	pixman_region32_translate(&sub->cached.damage, -canvas->pending.sx, -canvas->pending.sy);
	pixman_region32_union(&sub->cached.damage, &sub->cached.damage, &canvas->pending.damage);
	pixman_region32_clear(&canvas->pending.damage);

	if (canvas->pending.newly_attached) {
		sub->cached.newly_attached = 1;
		nemocanvas_state_set_buffer(&sub->cached, canvas->pending.buffer);
		nemobuffer_reference(&sub->buffer_reference, canvas->pending.buffer);
		nemopresentation_discard_feedback_list(&sub->cached.feedback_list);
	}

	sub->cached.sx += canvas->pending.sx;
	sub->cached.sy += canvas->pending.sy;

	canvas->pending.sx = 0;
	canvas->pending.sy = 0;
	canvas->pending.newly_attached = 0;

	sub->cached.buffer_viewport.changed |= canvas->pending.buffer_viewport.changed;
	sub->cached.buffer_viewport.buffer = canvas->pending.buffer_viewport.buffer;
	sub->cached.buffer_viewport.canvas = canvas->pending.buffer_viewport.canvas;

	nemocanvas_reset_pending_buffer(canvas);

	pixman_region32_copy(&sub->cached.opaque, &canvas->pending.opaque);
	pixman_region32_copy(&sub->cached.input, &canvas->pending.input);

	wl_list_insert_list(&sub->cached.frame_callback_list, &canvas->pending.frame_callback_list);
	wl_list_init(&canvas->pending.frame_callback_list);

	wl_list_insert_list(&sub->cached.feedback_list, &canvas->pending.feedback_list);
	wl_list_init(&canvas->pending.feedback_list);

	sub->has_cached_data = 1;
}

int nemosubcanvas_is_sync(struct nemosubcanvas *sub)
{
	while (sub) {
		if (sub->sync)
			return 1;

		if (sub->parent == NULL)
			return 0;

		sub = nemosubcanvas_get_subcanvas(sub->parent);
	}

	return 0;
}

void nemosubcanvas_commit(struct nemosubcanvas *sub)
{
	struct nemocanvas *canvas = sub->canvas;
	struct nemosubcanvas *tmp;

	if (nemosubcanvas_is_sync(sub)) {
		nemosubcanvas_commit_to_cache(sub);
	} else {
		if (sub->has_cached_data) {
			nemosubcanvas_commit_to_cache(sub);
			nemosubcanvas_commit_from_cache(sub);
		} else {
			nemocanvas_commit(canvas);
		}

		wl_list_for_each(tmp, &canvas->subcanvas_list, parent_link) {
			nemosubcanvas_commit_parent(tmp, 0);
		}
	}
}

void nemosubcanvas_commit_sync(struct nemosubcanvas *sub)
{
	struct nemocanvas *canvas = sub->canvas;
	struct nemosubcanvas *tmp;

	if (sub->has_cached_data)
		nemosubcanvas_commit_from_cache(sub);

	wl_list_for_each(tmp, &canvas->subcanvas_list, parent_link) {
		nemosubcanvas_commit_parent(tmp, 1);
	}
}

void nemosubcanvas_commit_parent(struct nemosubcanvas *sub, int sync)
{
	if (sync || sub->sync)
		nemosubcanvas_commit_sync(sub);
}
