#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <assert.h>
#include <wayland-server.h>

#include <view.h>
#include <content.h>
#include <compz.h>
#include <screen.h>
#include <renderer.h>
#include <layer.h>
#include <seat.h>
#include <nemomisc.h>
#include <nemolog.h>

struct nemoview *nemoview_create(struct nemocompz *compz, struct nemocontent *content)
{
	struct nemoview *view;

	view = (struct nemoview *)malloc(sizeof(struct nemoview));
	if (view == NULL)
		return NULL;
	memset(view, 0, sizeof(struct nemoview));

	view->compz = compz;
	view->content = content;
	view->canvas = NULL;
	view->actor = NULL;

	wl_signal_init(&view->destroy_signal);

	wl_list_init(&view->link);
	wl_list_init(&view->layer_link);

	wl_list_init(&view->children_link);
	wl_list_init(&view->children_list);

	wl_list_init(&view->geometry.transform_list);

	pixman_region32_init(&view->clip);

	view->state = NEMO_VIEW_CATCHABLE_STATE;

	view->psf_flags = 0x0;

	view->alpha = 1.0f;
	pixman_region32_init(&view->transform.opaque);

	view->transform.enable = 0;
	view->transform.dirty = 1;

	view->geometry.r = 0.0f;
	view->geometry.sx = 1.0f;
	view->geometry.sy = 1.0f;
	view->geometry.px = 0.0f;
	view->geometry.py = 0.0f;
	view->geometry.has_pivot = 0;

	view->geometry.width = 0;
	view->geometry.height = 0;

	pixman_region32_init(&view->transform.boundingbox);

	return view;
}

void nemoview_destroy(struct nemoview *view)
{
	wl_signal_emit(&view->destroy_signal, view);

	assert(wl_list_empty(&view->children_list));

	if (nemoview_is_mapped(view)) {
		nemoview_unmap(view);
	}

	wl_list_remove(&view->link);

	nemoview_detach_layer(view);

	pixman_region32_fini(&view->clip);
	pixman_region32_fini(&view->transform.boundingbox);

	nemoview_set_parent(view, NULL);

	free(view);
}

void nemoview_set_position(struct nemoview *view, float x, float y)
{
	if (view->geometry.x == x && view->geometry.y == y)
		return;

	view->geometry.x = x;
	view->geometry.y = y;
	nemoview_geometry_dirty(view);
}

void nemoview_set_rotation(struct nemoview *view, float r)
{
	if (view->geometry.r == r)
		return;

	view->transform.enable = 1;

	view->geometry.r = r;
	view->transform.cosr = cos(r);
	view->transform.sinr = sin(r);
	nemoview_geometry_dirty(view);
}

void nemoview_set_scale(struct nemoview *view, float sx, float sy)
{
	if (view->geometry.sx == sx && view->geometry.sy == sy)
		return;

	view->transform.enable = 1;

	view->geometry.sx = sx;
	view->geometry.sy = sy;
	nemoview_geometry_dirty(view);
}

void nemoview_set_pivot(struct nemoview *view, float px, float py)
{
	if (view->geometry.px == px && view->geometry.py == py)
		return;

	nemoview_correct_pivot(view, px, py);

	view->geometry.has_pivot = 1;
	nemoview_geometry_dirty(view);
}

void nemoview_put_pivot(struct nemoview *view)
{
	if (view->geometry.has_pivot == 0)
		return;
	
	nemoview_correct_pivot(view, 0.5f, 0.5f);
	
	view->geometry.has_pivot = 0;
	nemoview_geometry_dirty(view);
}

void nemoview_set_anchor(struct nemoview *view, float ax, float ay)
{
	if (view->geometry.ax == ax && view->geometry.ay == ay)
		return;

	view->geometry.ax = ax;
	view->geometry.ay = ay;
	view->geometry.has_anchor = 1;
	nemoview_geometry_dirty(view);
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

		nemomatrix_transform(&matrix, &vector);

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

		nemomatrix_transform(&matrix, &vector);

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

void nemoview_geometry_dirty(struct nemoview *view)
{
	struct nemoview *child;

	if (view->transform.dirty)
		return;

	view->transform.dirty = 1;

	wl_list_for_each(child, &view->children_list, children_link) {
		nemoview_geometry_dirty(child);
	}
}

void nemoview_damage_dirty(struct nemoview *view)
{
	pixman_region32_union_rect(&view->content->damage, &view->content->damage, 0, 0, view->content->width, view->content->height);
}

void nemoview_clip_dirty(struct nemoview *view)
{
	pixman_region32_clear(&view->clip);
}

void nemoview_damage_below(struct nemoview *view)
{
	pixman_region32_t damage;

	pixman_region32_init(&damage);
	pixman_region32_subtract(&damage, &view->transform.boundingbox, &view->clip);
	pixman_region32_union(&view->compz->damage, &view->compz->damage, &damage);
	pixman_region32_fini(&damage);

	nemoview_schedule_repaint(view);
}

int nemoview_is_mapped(struct nemoview *view)
{
	return view->screen_mask != 0;
}

void nemoview_unmap(struct nemoview *view)
{
	if (!nemoview_is_mapped(view))
		return;

	nemoview_damage_below(view);

	view->node_mask = 0;
	view->screen_mask = 0;

	nemocontent_update_output(view->content, 0, 0);

	wl_list_remove(&view->layer_link);
	wl_list_init(&view->layer_link);
	wl_list_remove(&view->link);
	wl_list_init(&view->link);

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

void nemoview_transform_to_global(struct nemoview *view, float sx, float sy, float *x, float *y)
{
	if (view->transform.enable) {
		struct nemovector v = { sx, sy, 0.0f, 1.0f };

		nemomatrix_transform(&view->transform.matrix, &v);

		if (fabsf(v.f[3]) < 1e-6) {
			*x = 0.0f;
			*y = 0.0f;
			return;
		}

		*x = v.f[0] / v.f[3];
		*y = v.f[1] / v.f[3];
	} else {
		*x = sx + view->geometry.x;
		*y = sy + view->geometry.y;
	}
}

void nemoview_transform_from_global(struct nemoview *view, float x, float y, float *sx, float *sy)
{
	if (view->transform.enable) {
		struct nemovector v = { x, y, 0.0f, 1.0f };

		nemomatrix_transform(&view->transform.inverse, &v);

		if (fabsf(v.f[3]) < 1e-6) {
			*sx = 0.0f;
			*sy = 0.0f;
			return;
		}

		*sx = v.f[0] / v.f[3];
		*sy = v.f[1] / v.f[3];
	} else {
		*sx = x - view->geometry.x;
		*sy = y - view->geometry.y;
	}
}

int nemoview_get_point_area(struct nemoview *view, float x, float y)
{
	float sx, sy;
	float dx, dy;
	float mx = view->content->width / 2.0f;
	float my = view->content->height / 2.0f;
	float ex = view->content->width / 6.0f;
	float ey = view->content->height / 6.0f;
	int area = NEMO_VIEW_CENTER_AREA;
	int i;

	nemoview_transform_from_global(view, x, y, &sx, &sy);

	dx = sx > mx ? view->content->width - sx : sx;
	dy = sy > my ? view->content->height - sy : sy;

	if (dx < ex && dy < ey) {
		if (sx < mx && sy < my)
			area = NEMO_VIEW_LEFTTOP_AREA;
		else if (sx > mx && sy < my)
			area = NEMO_VIEW_RIGHTTOP_AREA;
		else if (sx < mx && sy > my)
			area = NEMO_VIEW_LEFTBOTTOM_AREA;
		else if (sx > mx && sy > my)
			area = NEMO_VIEW_RIGHTBOTTOM_AREA;
	} else if (dx < dy && dx < ex) {
		if (sx < mx)
			area = NEMO_VIEW_LEFT_AREA;
		else
			area = NEMO_VIEW_RIGHT_AREA;
	} else if (dy < dx && dy < ey) {
		if (sy < my)
			area = NEMO_VIEW_TOP_AREA;
		else
			area = NEMO_VIEW_BOTTOM_AREA;
	}

	return area;
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

static void nemoview_update_output(struct nemoview *view)
{
	struct nemocompz *compz = view->compz;
	struct nemoscreen *screen;
	pixman_region32_t region;
	uint32_t scrnmask, nodemask;

	pixman_region32_init(&region);

	nodemask = 0;
	scrnmask = 0;

	view->screen = NULL;

	wl_list_for_each(screen, &compz->screen_list, link) {
		pixman_region32_intersect(&region, &view->transform.boundingbox, &screen->region);

		if (pixman_region32_not_empty(&region)) {
			scrnmask |= (1 << screen->id);
			nodemask |= (1 << screen->node->id);

			view->screen = screen;
		}
	}

	view->node_mask = nodemask;
	view->screen_mask = scrnmask;

	nemocontent_update_output(view->content, nodemask, scrnmask);

	pixman_region32_fini(&region);
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

	if (view->transform.enable == 0) {
		nemoview_update_transform_disable(view);
	} else if (nemoview_update_transform_enable(view) < 0) {
		nemoview_update_transform_disable(view);
	}

	nemoview_damage_below(view);

	nemoview_update_output(view);
}

void nemoview_update_transform_children(struct nemoview *view)
{
	struct nemoview *child;

	wl_list_for_each(child, &view->children_list, children_link) {
		if (child->transform.type == NEMO_VIEW_TRANSFORM_OVERLAY) {
			if (child->content->width != view->content->width || child->content->height != view->content->height) {
				child->geometry.sx = (double)view->content->width / (double)child->content->width;
				child->geometry.sy = (double)view->content->height / (double)child->content->height;
			} else {
				child->geometry.sx = 1.0f;
				child->geometry.sy = 1.0f;
			}

			nemoview_geometry_dirty(child);
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

		nemoview_geometry_dirty(view);
		nemoview_update_transform(view);
	}
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
		wl_list_remove(&view->children_link);
	}

	view->parent = parent;

	if (parent != NULL) {
		view->parent_destroy_listener.notify = nemoview_handle_transform_parent_destroy;
		wl_signal_add(&parent->destroy_signal, &view->parent_destroy_listener);
		wl_list_insert(&parent->children_list, &view->children_link);
	}

	nemoview_geometry_dirty(view);
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

void nemoview_attach_layer(struct nemoview *view, struct nemolayer *layer)
{
	struct wl_list *layer_link = &layer->view_list;

	if (layer_link == &view->layer_link)
		return;

	nemoview_geometry_dirty(view);
	wl_list_remove(&view->layer_link);
	wl_list_insert(layer_link, &view->layer_link);
	nemoview_clip_dirty(view);
	nemoview_damage_below(view);

	view->layer = layer;
}

void nemoview_detach_layer(struct nemoview *view)
{
	if (view->layer == NULL)
		return;

	nemoview_geometry_dirty(view);
	wl_list_remove(&view->layer_link);
	wl_list_init(&view->layer_link);

	view->layer = NULL;
}

void nemoview_update_layer(struct nemoview *view)
{
	struct wl_list *layer_link;

	if (view->parent != NULL)
		view = view->parent;

	if (view->layer == NULL)
		return;

	layer_link = &view->layer->view_list;

	nemoview_geometry_dirty(view);
	wl_list_remove(&view->layer_link);
	wl_list_insert(layer_link, &view->layer_link);
	nemoview_clip_dirty(view);
	nemoview_damage_below(view);
}

void nemoview_above_layer(struct nemoview *view, struct nemoview *above)
{
	struct wl_list *layer_link;

	assert(view != NULL && above != NULL);

	if (view->layer != NULL) {
		nemoview_geometry_dirty(view);
		wl_list_remove(&view->layer_link);
		wl_list_init(&view->layer_link);

		view->layer = NULL;
	}

	layer_link = above->layer_link.prev;

	wl_list_insert(layer_link, &view->layer_link);
	nemoview_clip_dirty(view);
	nemoview_damage_below(view);

	view->layer = above->layer;
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
