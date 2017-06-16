#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <assert.h>
#include <uuid/uuid.h>
#include <wayland-server.h>

#include <view.h>
#include <content.h>
#include <canvas.h>
#include <compz.h>
#include <screen.h>
#include <renderer.h>
#include <layer.h>
#include <seat.h>
#include <keyboard.h>
#include <keymap.h>
#include <scope.h>
#include <nemomisc.h>
#include <nemolog.h>

static struct nemocontent content_dummy;

void __attribute__((constructor(101))) nemoview_initialize(void)
{
	nemocontent_prepare(&content_dummy, NEMOCOMPZ_NODE_MAX);
}

void __attribute__((destructor(101))) nemoview_finalize(void)
{
	nemocontent_finish(&content_dummy);
}

struct nemoview *nemoview_create(struct nemocompz *compz)
{
	struct nemoview *view;
	uuid_t uuid;

	view = (struct nemoview *)malloc(sizeof(struct nemoview));
	if (view == NULL)
		return NULL;
	memset(view, 0, sizeof(struct nemoview));

	view->scope = nemoscope_create();
	if (view->scope == NULL)
		goto err1;

	view->compz = compz;
	view->content = &content_dummy;
	view->canvas = NULL;

	uuid_generate_time_safe(uuid);
	uuid_unparse_lower(uuid, view->uuid);
	uuid_clear(uuid);

	wl_signal_init(&view->destroy_signal);

	wl_list_init(&view->link);
	wl_list_init(&view->layer_link);

	wl_list_init(&view->children_link);
	wl_list_init(&view->children_list);

	wl_list_init(&view->canvas_destroy_listener.link);
	wl_list_init(&view->parent_destroy_listener.link);
	wl_list_init(&view->focus_destroy_listener.link);

	wl_list_init(&view->geometry.transform_list);

	pixman_region32_init(&view->clip);

	view->state = NEMOVIEW_PICK_STATE | NEMOVIEW_KEYPAD_STATE | NEMOVIEW_SOUND_STATE | NEMOVIEW_STAGE_STATE | NEMOVIEW_SMOOTH_STATE;

	view->psf_flags = 0x0;

	view->alpha = 1.0f;
	pixman_region32_init(&view->transform.opaque);

	view->transform.enable = 0;
	view->transform.dirty = 1;
	view->transform.notify = 1;

	view->geometry.r = 0.0f;
	view->geometry.sx = 1.0f;
	view->geometry.sy = 1.0f;
	view->geometry.px = 0.0f;
	view->geometry.py = 0.0f;
	view->geometry.has_pivot = 0;

	view->geometry.fx = 0.5f;
	view->geometry.fy = 0.5f;

	view->geometry.width = 0;
	view->geometry.height = 0;

	pixman_region32_init(&view->geometry.region);
	pixman_region32_init(&view->transform.boundingbox);

	return view;

err1:
	free(view);

	return NULL;
}

void nemoview_destroy(struct nemoview *view)
{
	wl_signal_emit(&view->destroy_signal, view);

	assert(wl_list_empty(&view->children_list));

	if (nemoview_has_state(view, NEMOVIEW_MAP_STATE) != 0)
		nemoview_unmap(view);

	if (view->xkb != NULL)
		nemoxkb_destroy(view->xkb);

	wl_list_remove(&view->link);
	wl_list_remove(&view->layer_link);
	wl_list_remove(&view->children_link);

	wl_list_remove(&view->canvas_destroy_listener.link);
	wl_list_remove(&view->parent_destroy_listener.link);
	wl_list_remove(&view->focus_destroy_listener.link);

	pixman_region32_fini(&view->clip);
	pixman_region32_fini(&view->geometry.region);
	pixman_region32_fini(&view->transform.boundingbox);

	nemoscope_destroy(view->scope);

	if (view->type != NULL)
		free(view->type);

	free(view);
}

void nemoview_correct_pivot(struct nemoview *view, float px, float py)
{
	if (view->geometry.px != px || view->geometry.py != py) {
		struct nemomatrix matrix;
		struct nemovector vector;
		float sx, sy, ex, ey;

		nemomatrix_init_identity(&matrix);

		if (view->geometry.r != 0.0f) {
			nemomatrix_translate(&matrix, -view->geometry.px, -view->geometry.py);
			nemomatrix_rotate(&matrix, view->transform.cosr, view->transform.sinr);
			nemomatrix_translate(&matrix, view->geometry.px, view->geometry.py);
		}

		if (view->geometry.sx != 1.0f || view->geometry.sy != 1.0f) {
			nemomatrix_translate(&matrix, -view->geometry.px, -view->geometry.py);
			nemomatrix_scale(&matrix, view->geometry.sx, view->geometry.sy);
			nemomatrix_translate(&matrix, view->geometry.px, view->geometry.py);
		}

		vector.f[0] = 0.0f;
		vector.f[1] = 0.0f;
		vector.f[2] = 0.0f;
		vector.f[3] = 1.0f;

		nemomatrix_transform_vector(&matrix, &vector);

		if (fabsf(vector.f[3]) < 1e-6) {
			sx = 0.0f;
			sy = 0.0f;
		} else {
			sx = vector.f[0] / vector.f[3];
			sy = vector.f[1] / vector.f[3];
		}

		view->geometry.px = px;
		view->geometry.py = py;

		nemomatrix_init_identity(&matrix);

		if (view->geometry.r != 0.0f) {
			nemomatrix_translate(&matrix, -view->geometry.px, -view->geometry.py);
			nemomatrix_rotate(&matrix, view->transform.cosr, view->transform.sinr);
			nemomatrix_translate(&matrix, view->geometry.px, view->geometry.py);
		}

		if (view->geometry.sx != 1.0f || view->geometry.sy != 1.0f) {
			nemomatrix_translate(&matrix, -view->geometry.px, -view->geometry.py);
			nemomatrix_scale(&matrix, view->geometry.sx, view->geometry.sy);
			nemomatrix_translate(&matrix, view->geometry.px, view->geometry.py);
		}

		vector.f[0] = 0.0f;
		vector.f[1] = 0.0f;
		vector.f[2] = 0.0f;
		vector.f[3] = 1.0f;

		nemomatrix_transform_vector(&matrix, &vector);

		if (fabsf(vector.f[3]) < 1e-6) {
			ex = 0.0f;
			ey = 0.0f;
		} else {
			ex = vector.f[0] / vector.f[3];
			ey = vector.f[1] / vector.f[3];
		}

		view->geometry.x = view->geometry.x - (ex - sx);
		view->geometry.y = view->geometry.y - (ey - sy);
	}
}

void nemoview_unmap(struct nemoview *view)
{
	if (nemoview_has_state(view, NEMOVIEW_MAP_STATE) == 0)
		return;

	nemoview_damage_below(view);

	view->node_mask = 0;
	view->screen_mask = 0;

	nemoview_put_state(view, NEMOVIEW_MAP_STATE);

	nemocontent_update_output(view->content);

	wl_list_remove(&view->layer_link);
	wl_list_init(&view->layer_link);
	wl_list_remove(&view->link);
	wl_list_init(&view->link);

	view->layer = NULL;

	view->compz->layer_notify = 1;

	nemoseat_put_focus(view->compz->seat, view);
}

void nemoview_schedule_repaint(struct nemoview *view)
{
	struct nemoscreen *screen;

	wl_list_for_each(screen, &view->compz->screen_list, link) {
		if (view->screen_mask & (1 << screen->id))
			nemoscreen_schedule_repaint(screen);
	}
}

static void nemoview_update_boundingbox(struct nemoview *view, int32_t x, int32_t y, int32_t width, int32_t height, pixman_region32_t *bbox)
{
	float minx = HUGE_VALF, miny = HUGE_VALF;
	float maxx = -HUGE_VALF, maxy = -HUGE_VALF;
	int32_t s[4][2] = {
		{ x, y },
		{ x, y + height },
		{ x + width, y },
		{ x + width, y + height }
	};
	float tx, ty;
	float sx, sy, ex, ey;
	int i;

	if (width == 0 || height == 0) {
		pixman_region32_init(bbox);
		return;
	}

	for (i = 0; i < 4; i++) {
		nemoview_transform_to_global(view, s[i][0], s[i][1], &tx, &ty);

		if (tx < minx)
			minx = tx;
		if (tx > maxx)
			maxx = tx;
		if (ty < miny)
			miny = ty;
		if (ty > maxy)
			maxy = ty;
	}

	sx = floorf(minx);
	sy = floorf(miny);
	ex = ceilf(maxx);
	ey = ceilf(maxy);

	pixman_region32_init_rect(bbox, sx, sy, ex - sx, ey - sy);
}

void nemoview_update_output(struct nemoview *view)
{
	struct nemocompz *compz = view->compz;
	struct nemoscreen *screen;
	pixman_region32_t region;
	uint32_t scrnmask, nodemask;

	nodemask = 0;
	scrnmask = 0;

	view->screen = NULL;

	pixman_region32_init(&region);

	wl_list_for_each(screen, &compz->screen_list, link) {
		pixman_region32_intersect(&region, &view->transform.boundingbox, &screen->region);

		if (pixman_region32_not_empty(&region)) {
			scrnmask |= (1 << screen->id);
			nodemask |= (1 << screen->node->id);

			view->screen = screen;
		}
	}

	pixman_region32_fini(&region);

	view->node_mask = nodemask;
	view->screen_mask = scrnmask;

	nemocontent_update_output(view->content);
}

static void nemoview_update_transform_disable(struct nemoview *view)
{
	view->geometry.x = roundf(view->geometry.x);
	view->geometry.y = roundf(view->geometry.y);

	nemomatrix_init_translate(&view->transform.matrix, view->geometry.x, view->geometry.y);
	nemomatrix_init_translate(&view->transform.inverse, -view->geometry.x, -view->geometry.y);

	pixman_region32_init_rect(&view->transform.boundingbox,
			view->geometry.x, view->geometry.y,
			view->content->width, view->content->height);

	if (view->alpha == 1.0f) {
		pixman_region32_copy(&view->transform.opaque, &view->content->opaque);
		pixman_region32_translate(&view->transform.opaque, view->geometry.x, view->geometry.y);
	}
}

static int nemoview_update_transform_enable(struct nemoview *view)
{
	struct nemoview *parent = view->parent;
	struct nemomatrix *matrix = &view->transform.matrix;
	struct nemomatrix *inverse = &view->transform.inverse;
	struct nemotransform *transform;

	nemomatrix_init_identity(matrix);

	if (view->geometry.r != 0.0f) {
		nemomatrix_translate(matrix, -view->geometry.px, -view->geometry.py);
		nemomatrix_rotate(matrix, view->transform.cosr, view->transform.sinr);
		nemomatrix_translate(matrix, view->geometry.px, view->geometry.py);
	}

	if (view->geometry.sx != 1.0f || view->geometry.sy != 1.0f) {
		nemomatrix_translate(matrix, -view->geometry.px, -view->geometry.py);
		nemomatrix_scale(matrix, view->geometry.sx, view->geometry.sy);
		nemomatrix_translate(matrix, view->geometry.px, view->geometry.py);
	}

	nemomatrix_translate(matrix, view->geometry.x, view->geometry.y);

	wl_list_for_each(transform, &view->geometry.transform_list, link) {
		nemomatrix_multiply(matrix, &transform->matrix);
	}

	if (parent != NULL)
		nemomatrix_multiply(matrix, &parent->transform.matrix);

	if (nemomatrix_invert(inverse, matrix) < 0) {
		nemolog_warning("VIEW", "failed to invert transform matrix\n");
		return -1;
	}

	nemoview_update_boundingbox(view, 0, 0, view->content->width, view->content->height, &view->transform.boundingbox);

	return 0;
}

void nemoview_update_transform(struct nemoview *view)
{
	if (!view->transform.dirty)
		return;

	view->transform.dirty = 0;

	if (view->parent != NULL)
		nemoview_update_transform(view->parent);

	nemoview_damage_below(view);

	pixman_region32_fini(&view->transform.boundingbox);
	pixman_region32_fini(&view->transform.opaque);
	pixman_region32_init(&view->transform.opaque);

	if (nemoview_has_state(view, NEMOVIEW_REGION_STATE) == 0)
		pixman_region32_init_rect(&view->geometry.region,
				0, 0, view->content->width, view->content->height);

	if (view->transform.enable == 0) {
		nemoview_update_transform_disable(view);
	} else if (nemoview_update_transform_enable(view) < 0) {
		nemoview_update_transform_disable(view);
	}

	nemoview_damage_below(view);

	nemoview_update_output(view);
}

void nemoview_update_transform_notify(struct nemoview *view)
{
	view->transform.notify = 0;

	view->compz->layer_notify = 1;

	if (nemoview_has_grab(view) == 0) {
		pixman_region32_t region;
		pixman_box32_t *extents;
		float p[4][2];
		float x0, y0, x1, y1;

		pixman_region32_init(&region);
		pixman_region32_intersect(&region, &view->compz->scope, &view->transform.boundingbox);

		extents = pixman_region32_extents(&region);

		nemoview_transform_from_global_nocheck(view, extents->x1, extents->y1, &p[0][0], &p[0][1]);
		nemoview_transform_from_global_nocheck(view, extents->x2, extents->y1, &p[1][0], &p[1][1]);
		nemoview_transform_from_global_nocheck(view, extents->x1, extents->y2, &p[2][0], &p[2][1]);
		nemoview_transform_from_global_nocheck(view, extents->x2, extents->y2, &p[3][0], &p[3][1]);

		pixman_region32_fini(&region);

		x0 = MIN(MIN(p[0][0], p[1][0]), MIN(p[2][0], p[3][0]));
		x1 = MAX(MAX(p[0][0], p[1][0]), MAX(p[2][0], p[3][0]));
		y0 = MIN(MIN(p[0][1], p[1][1]), MIN(p[2][1], p[3][1]));
		y1 = MAX(MAX(p[0][1], p[1][1]), MAX(p[2][1], p[3][1]));

		x0 = MAX(x0, 0.0f);
		x1 = MIN(x1, view->content->width);
		y0 = MAX(y0, 0.0f);
		y1 = MIN(y1, view->content->height);

		nemocontent_update_transform(view->content,
				nemocompz_contain_view(view->compz, view),
				x0, y0, x1 - x0, y1 - y0);
	} else {
		nemocontent_update_transform(view->content,
				nemocompz_contain_view(view->compz, view),
				0, 0, view->content->width, view->content->height);
	}
}

void nemoview_update_transform_children(struct nemoview *view)
{
	struct nemoview *child;

	wl_list_for_each(child, &view->children_list, children_link) {
		if (child->transform.type == NEMOVIEW_TRANSFORM_OVERLAY) {
			if (child->content->width != view->content->width || child->content->height != view->content->height) {
				child->geometry.sx = (double)view->content->width / (double)child->content->width;
				child->geometry.sy = (double)view->content->height / (double)child->content->height;
			} else {
				child->geometry.sx = 1.0f;
				child->geometry.sy = 1.0f;
			}

			nemoview_transform_dirty(child);
			nemoview_update_transform(child);
		}
	}
}

void nemoview_update_transform_parent(struct nemoview *view)
{
	struct nemoview *parent = view->parent;

	if (parent != NULL) {
		if (view->content->width != parent->content->width || view->content->height != parent->content->height) {
			view->geometry.sx = (double)parent->content->width / (double)view->content->width;
			view->geometry.sy = (double)parent->content->height / (double)view->content->height;
		} else {
			view->geometry.sx = 1.0f;
			view->geometry.sy = 1.0f;
		}

		nemoview_transform_dirty(view);
		nemoview_update_transform(view);
	}
}

void nemoview_set_type(struct nemoview *view, const char *type)
{
	if (view->type != NULL)
		free(view->type);

	view->type = strdup(type);
}

void nemoview_set_uuid(struct nemoview *view, const char *uuid)
{
	strcpy(view->uuid, uuid);
}

static void nemoview_handle_transform_parent_destroy(struct wl_listener *listener, void *data)
{
	struct nemoview *view = (struct nemoview *)container_of(listener, struct nemoview, parent_destroy_listener);

	nemoview_set_parent(view, NULL);
}

void nemoview_set_parent(struct nemoview *view, struct nemoview *parent)
{
	view->transform.enable = 1;

	if (view->parent != NULL) {
		wl_list_remove(&view->parent_destroy_listener.link);
		wl_list_init(&view->parent_destroy_listener.link);

		wl_list_remove(&view->children_link);
		wl_list_init(&view->children_link);
	}

	view->parent = parent;

	if (parent != NULL) {
		view->parent_destroy_listener.notify = nemoview_handle_transform_parent_destroy;
		wl_signal_add(&parent->destroy_signal, &view->parent_destroy_listener);

		wl_list_insert(&parent->children_list, &view->children_link);
	}

	nemoview_transform_dirty(view);
}

static void nemoview_handle_focus_destroy(struct wl_listener *listener, void *data)
{
	struct nemoview *view = (struct nemoview *)container_of(listener, struct nemoview, focus_destroy_listener);

	view->focus = NULL;

	wl_list_remove(&view->focus_destroy_listener.link);
	wl_list_init(&view->focus_destroy_listener.link);
}

void nemoview_set_focus(struct nemoview *view, struct nemoview *focus)
{
	if (view->focus != NULL) {
		wl_list_remove(&view->focus_destroy_listener.link);
		wl_list_init(&view->focus_destroy_listener.link);
	}

	if (focus != NULL) {
		view->focus_destroy_listener.notify = nemoview_handle_focus_destroy;
		wl_signal_add(&focus->destroy_signal, &view->focus_destroy_listener);
	}

	view->focus = focus;
}

void nemoview_set_region(struct nemoview *view, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	pixman_region32_union_rect(&view->geometry.region, &view->geometry.region, x, y, width, height);

	nemoview_set_state(view, NEMOVIEW_REGION_STATE);
}

void nemoview_put_region(struct nemoview *view)
{
	pixman_region32_clear(&view->geometry.region);

	nemoview_put_state(view, NEMOVIEW_REGION_STATE);
}

void nemoview_set_scope(struct nemoview *view, const char *cmds)
{
	nemoscope_add_cmd(view->scope, 1, cmds);

	nemoview_set_state(view, NEMOVIEW_SCOPE_STATE);
}

void nemoview_put_scope(struct nemoview *view)
{
	nemoscope_clear(view->scope);

	nemoview_put_state(view, NEMOVIEW_SCOPE_STATE);
}

void nemoview_accumulate_damage(struct nemoview *view, pixman_region32_t *opaque)
{
	pixman_region32_t damage;

	pixman_region32_init(&damage);

	if (view->transform.enable) {
		pixman_box32_t *extents;

		extents = pixman_region32_extents(&view->content->damage);
		nemoview_update_boundingbox(view,
				extents->x1, extents->y1,
				extents->x2 - extents->x1,
				extents->y2 - extents->y1,
				&damage);
	} else {
		pixman_region32_copy(&damage, &view->content->damage);
		pixman_region32_translate(&damage, view->geometry.x, view->geometry.y);
	}

	pixman_region32_subtract(&damage, &damage, opaque);
	pixman_region32_union(&view->compz->damage, &view->compz->damage, &damage);
	pixman_region32_fini(&damage);
	pixman_region32_copy(&view->clip, opaque);
	pixman_region32_union(opaque, opaque, &view->transform.opaque);
}

void nemoview_merge_damage(struct nemoview *view, pixman_region32_t *damage)
{
	if (view->transform.enable) {
		pixman_box32_t *extents;

		extents = pixman_region32_extents(&view->content->damage);
		nemoview_update_boundingbox(view,
				extents->x1, extents->y1,
				extents->x2 - extents->x1,
				extents->y2 - extents->y1,
				damage);
	} else {
		pixman_region32_copy(damage, &view->content->damage);
		pixman_region32_translate(damage, view->geometry.x, view->geometry.y);
	}
}

static void nemoview_handle_canvas_destroy(struct wl_listener *listener, void *data)
{
	struct nemoview *view = (struct nemoview *)container_of(listener, struct nemoview, canvas_destroy_listener);

	view->canvas = NULL;
	view->content = &content_dummy;

	wl_list_remove(&view->link);
	wl_list_init(&view->link);

	wl_list_remove(&view->canvas_destroy_listener.link);
	wl_list_init(&view->canvas_destroy_listener.link);
}

void nemoview_attach_canvas(struct nemoview *view, struct nemocanvas *canvas)
{
	wl_list_remove(&view->link);
	wl_list_init(&view->link);

	wl_list_remove(&view->canvas_destroy_listener.link);
	wl_list_init(&view->canvas_destroy_listener.link);

	if (canvas != NULL) {
		view->canvas = canvas;
		view->content = &canvas->base;

		wl_list_insert(&canvas->view_list, &view->link);

		view->canvas_destroy_listener.notify = nemoview_handle_canvas_destroy;
		wl_signal_add(&canvas->destroy_signal, &view->canvas_destroy_listener);
	} else {
		view->canvas = NULL;
		view->content = &content_dummy;
	}
}

void nemoview_detach_canvas(struct nemoview *view)
{
	view->canvas = NULL;
	view->content = &content_dummy;

	wl_list_remove(&view->link);
	wl_list_init(&view->link);

	wl_list_remove(&view->canvas_destroy_listener.link);
	wl_list_init(&view->canvas_destroy_listener.link);
}

void nemoview_attach_layer(struct nemoview *view, struct nemolayer *layer)
{
	struct wl_list *layer_link = &layer->view_list;

	if (layer == NULL)
		return;

	if (layer_link == &view->layer_link)
		return;

	nemoview_transform_dirty(view);
	wl_list_remove(&view->layer_link);
	wl_list_insert(layer_link, &view->layer_link);
	nemoview_clip_dirty(view);
	nemoview_damage_below(view);

	view->layer = layer;

	view->compz->layer_notify = 1;
}

void nemoview_detach_layer(struct nemoview *view)
{
	if (view->layer == NULL)
		return;

	nemoview_transform_dirty(view);
	wl_list_remove(&view->layer_link);
	wl_list_init(&view->layer_link);
	nemoview_damage_below(view);

	view->layer = NULL;

	view->compz->layer_notify = 1;
}

void nemoview_above_layer(struct nemoview *view, struct nemoview *above)
{
	struct wl_list *layer_link;

	if (view->parent != NULL)
		view = view->parent;

	if (above != NULL) {
		layer_link = above->layer_link.prev;
		view->layer = above->layer;
	} else {
		layer_link = &view->layer->view_list;
	}

	nemoview_transform_dirty(view);
	wl_list_remove(&view->layer_link);
	wl_list_insert(layer_link, &view->layer_link);
	nemoview_clip_dirty(view);
	nemoview_damage_below(view);

	view->compz->layer_notify = 1;
}

void nemoview_below_layer(struct nemoview *view, struct nemoview *below)
{
	struct wl_list *layer_link;

	if (view->parent != NULL)
		view = view->parent;

	if (below != NULL) {
		layer_link = below->layer_link.next;
		view->layer = below->layer;
	} else {
		layer_link = view->layer->view_list.prev;
	}

	nemoview_transform_dirty(view);
	wl_list_remove(&view->layer_link);
	wl_list_insert(layer_link, &view->layer_link);
	nemoview_clip_dirty(view);
	nemoview_damage_below(view);

	view->compz->layer_notify = 1;
}

int nemoview_get_trapezoids(struct nemoview *view, int32_t x, int32_t y, int32_t width, int32_t height, pixman_trapezoid_t *traps)
{
	float s[4][2] = {
		{ x, y },
		{ x, y + height },
		{ x + width, y },
		{ x + width, y + height }
	};
	float d[4][3] = { 0 };
	float tx0, tx1;
	int ntraps = 0;
	int i, j, k;

	for (i = 0; i < 4; i++) {
		nemoview_transform_to_global(view, s[i][0], s[i][1], &s[i][0], &s[i][1]);
	}

	d[0][0] = d[1][0] = d[2][0] = d[3][0] = HUGE_VALF;

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			if (d[j][0] > s[i][1]) {
				for (k = ntraps - 1; k >= j; k--) {
					d[k+1][0] = d[k][0];
					d[k+1][1] = d[k][1];
					d[k+1][2] = d[k][2];
				}

				d[j][0] = s[i][1];
				d[j][1] = s[i][0];
				d[j][2] = s[i][0];

				ntraps++;
				break;
			} else if (d[j][0] == s[i][1]) {
				if (d[j][1] > s[i][0]) {
					d[j][2] = d[j][1];
					d[j][1] = s[i][0];
				} else {
					d[j][2] = s[i][0];
				}
				break;
			}
		}
	}

	if (ntraps == 2) {
		traps[0].top = pixman_double_to_fixed(d[0][0]);
		traps[0].bottom = pixman_double_to_fixed(d[1][0]);
		traps[0].left.p1.x = pixman_double_to_fixed(d[0][1]);
		traps[0].left.p1.y = pixman_double_to_fixed(d[0][0]);
		traps[0].left.p2.x = pixman_double_to_fixed(d[1][1]);
		traps[0].left.p2.y = pixman_double_to_fixed(d[1][0]);
		traps[0].right.p1.x = pixman_double_to_fixed(d[0][2]);
		traps[0].right.p1.y = pixman_double_to_fixed(d[0][0]);
		traps[0].right.p2.x = pixman_double_to_fixed(d[1][2]);
		traps[0].right.p2.y = pixman_double_to_fixed(d[1][0]);
	} else if (ntraps == 3) {
		traps[0].top = pixman_double_to_fixed(d[0][0]);
		traps[0].bottom = pixman_double_to_fixed(d[1][0]);
		traps[0].left.p1.x = pixman_double_to_fixed(d[0][1]);
		traps[0].left.p1.y = pixman_double_to_fixed(d[0][0]);
		traps[0].left.p2.x = pixman_double_to_fixed(d[1][1]);
		traps[0].left.p2.y = pixman_double_to_fixed(d[1][0]);
		traps[0].right.p1.x = pixman_double_to_fixed(d[0][1]);
		traps[0].right.p1.y = pixman_double_to_fixed(d[0][0]);
		traps[0].right.p2.x = pixman_double_to_fixed(d[1][2]);
		traps[0].right.p2.y = pixman_double_to_fixed(d[1][0]);

		traps[1].top = pixman_double_to_fixed(d[1][0]);
		traps[1].bottom = pixman_double_to_fixed(d[2][0]);
		traps[1].left.p1.x = pixman_double_to_fixed(d[1][1]);
		traps[1].left.p1.y = pixman_double_to_fixed(d[1][0]);
		traps[1].left.p2.x = pixman_double_to_fixed(d[2][1]);
		traps[1].left.p2.y = pixman_double_to_fixed(d[2][0]);
		traps[1].right.p1.x = pixman_double_to_fixed(d[1][2]);
		traps[1].right.p1.y = pixman_double_to_fixed(d[1][0]);
		traps[1].right.p2.x = pixman_double_to_fixed(d[2][1]);
		traps[1].right.p2.y = pixman_double_to_fixed(d[2][0]);
	} else if (ntraps == 4) {
		if (d[1][1] < d[2][1]) {
			tx0 = (d[2][1] - d[0][1]) * ((d[1][0] - d[0][0]) / (d[2][0] - d[0][0])) + d[0][1];
			tx1 = (d[3][1] - d[1][1]) * ((d[2][0] - d[1][0]) / (d[3][0] - d[1][0])) + d[1][1];

			traps[0].top = pixman_double_to_fixed(d[0][0]);
			traps[0].bottom = pixman_double_to_fixed(d[1][0]);
			traps[0].left.p1.x = pixman_double_to_fixed(d[0][1]);
			traps[0].left.p1.y = pixman_double_to_fixed(d[0][0]);
			traps[0].left.p2.x = pixman_double_to_fixed(d[1][1]);
			traps[0].left.p2.y = pixman_double_to_fixed(d[1][0]);
			traps[0].right.p1.x = pixman_double_to_fixed(d[0][1]);
			traps[0].right.p1.y = pixman_double_to_fixed(d[0][0]);
			traps[0].right.p2.x = pixman_double_to_fixed(tx0);
			traps[0].right.p2.y = pixman_double_to_fixed(d[1][0]);

			traps[1].top = pixman_double_to_fixed(d[1][0]);
			traps[1].bottom = pixman_double_to_fixed(d[2][0]);
			traps[1].left.p1.x = pixman_double_to_fixed(d[1][1]);
			traps[1].left.p1.y = pixman_double_to_fixed(d[1][0]);
			traps[1].left.p2.x = pixman_double_to_fixed(tx1);
			traps[1].left.p2.y = pixman_double_to_fixed(d[2][0]);
			traps[1].right.p1.x = pixman_double_to_fixed(tx0);
			traps[1].right.p1.y = pixman_double_to_fixed(d[1][0]);
			traps[1].right.p2.x = pixman_double_to_fixed(d[2][1]);
			traps[1].right.p2.y = pixman_double_to_fixed(d[2][0]);

			traps[2].top = pixman_double_to_fixed(d[2][0]);
			traps[2].bottom = pixman_double_to_fixed(d[3][0]);
			traps[2].left.p1.x = pixman_double_to_fixed(tx1);
			traps[2].left.p1.y = pixman_double_to_fixed(d[2][0]);
			traps[2].left.p2.x = pixman_double_to_fixed(d[3][1]);
			traps[2].left.p2.y = pixman_double_to_fixed(d[3][0]);
			traps[2].right.p1.x = pixman_double_to_fixed(d[2][1]);
			traps[2].right.p1.y = pixman_double_to_fixed(d[2][0]);
			traps[2].right.p2.x = pixman_double_to_fixed(d[3][1]);
			traps[2].right.p2.y = pixman_double_to_fixed(d[3][0]);
		} else {
			tx0 = d[0][1] - (d[0][1] - d[2][1]) * ((d[1][0] - d[0][0]) / (d[2][0] - d[0][0]));
			tx1 = d[1][1] - (d[1][1] - d[3][1]) * ((d[2][0] - d[1][0]) / (d[3][0] - d[1][0]));

			traps[0].top = pixman_double_to_fixed(d[0][0]);
			traps[0].bottom = pixman_double_to_fixed(d[1][0]);
			traps[0].left.p1.x = pixman_double_to_fixed(d[0][1]);
			traps[0].left.p1.y = pixman_double_to_fixed(d[0][0]);
			traps[0].left.p2.x = pixman_double_to_fixed(tx0);
			traps[0].left.p2.y = pixman_double_to_fixed(d[1][0]);
			traps[0].right.p1.x = pixman_double_to_fixed(d[0][1]);
			traps[0].right.p1.y = pixman_double_to_fixed(d[0][0]);
			traps[0].right.p2.x = pixman_double_to_fixed(d[1][1]);
			traps[0].right.p2.y = pixman_double_to_fixed(d[1][0]);

			traps[1].top = pixman_double_to_fixed(d[1][0]);
			traps[1].bottom = pixman_double_to_fixed(d[2][0]);
			traps[1].left.p1.x = pixman_double_to_fixed(tx0);
			traps[1].left.p1.y = pixman_double_to_fixed(d[1][0]);
			traps[1].left.p2.x = pixman_double_to_fixed(d[2][1]);
			traps[1].left.p2.y = pixman_double_to_fixed(d[2][0]);
			traps[1].right.p1.x = pixman_double_to_fixed(d[1][1]);
			traps[1].right.p1.y = pixman_double_to_fixed(d[1][0]);
			traps[1].right.p2.x = pixman_double_to_fixed(tx1);
			traps[1].right.p2.y = pixman_double_to_fixed(d[2][0]);

			traps[2].top = pixman_double_to_fixed(d[2][0]);
			traps[2].bottom = pixman_double_to_fixed(d[3][0]);
			traps[2].left.p1.x = pixman_double_to_fixed(d[2][1]);
			traps[2].left.p1.y = pixman_double_to_fixed(d[2][0]);
			traps[2].left.p2.x = pixman_double_to_fixed(d[3][1]);
			traps[2].left.p2.y = pixman_double_to_fixed(d[3][0]);
			traps[2].right.p1.x = pixman_double_to_fixed(tx1);
			traps[2].right.p1.y = pixman_double_to_fixed(d[2][0]);
			traps[2].right.p2.x = pixman_double_to_fixed(d[3][1]);
			traps[2].right.p2.y = pixman_double_to_fixed(d[3][0]);
		}
	}

	return ntraps - 1;
}
