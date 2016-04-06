#ifndef	__NEMO_VIEW_H__
#define	__NEMO_VIEW_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <pixman.h>

#include <compz.h>
#include <content.h>
#include <nemomatrix.h>
#include <nemometro.h>

typedef enum {
	NEMO_VIEW_TRANSFORM_NORMAL = 0,
	NEMO_VIEW_TRANSFORM_OVERLAY = 1,
	NEMO_VIEW_TRANSFORM_LAST
} NemoViewTransformType;

typedef enum {
	NEMO_VIEW_MAP_STATE = (1 << 0),
	NEMO_VIEW_CATCH_STATE = (1 << 1),
	NEMO_VIEW_PICK_STATE = (1 << 2),
	NEMO_VIEW_KEYPAD_STATE = (1 << 3),
	NEMO_VIEW_SOUND_STATE = (1 << 4),
	NEMO_VIEW_LAYER_STATE = (1 << 5),
	NEMO_VIEW_PUSH_STATE = (1 << 6),
	NEMO_VIEW_SCOPE_STATE = (1 << 7),
	NEMO_VIEW_LAST_STATE
} NemoViewState;

struct nemocompz;
struct nemolayer;
struct nemocontent;
struct nemocanvas;
struct nemoactor;

struct nemotransform {
	struct nemomatrix matrix;
	struct wl_list link;
};

struct nemoview {
	struct nemocompz *compz;
	struct nemocontent *content;

	struct nemocanvas *canvas;
	struct nemoactor *actor;

	uint32_t tag;

	struct wl_signal destroy_signal;

	struct wl_list link;
	struct wl_list layer_link;

	struct nemoview *parent;
	struct wl_listener parent_destroy_listener;
	struct wl_list children_list;
	struct wl_list children_link;

	struct nemolayer *layer;

	struct nemoscreen *screen;

	uint32_t state;

	uint32_t node_mask;
	uint32_t screen_mask;

	uint32_t psf_flags;

	pixman_region32_t clip;
	float alpha;

	pixman_region32_t *input;

	struct {
		uint32_t keyboard_count;
	} focus;

	struct {
		float px, py;
		int has_pivot;

		float ax, ay;
		int has_anchor;

		float fx, fy;

		float x, y;
		float r;
		float sx, sy;

		int32_t width, height;

		pixman_region32_t scope;

		struct wl_list transform_list;
	} geometry;

	struct {
		int enable;
		int dirty;
		int done;
		int type;

		float cosr, sinr;

		pixman_region32_t boundingbox;
		pixman_region32_t opaque;

		struct nemomatrix matrix, inverse;
	} transform;

	void *data;
};

extern struct nemoview *nemoview_create(struct nemocompz *compz, struct nemocontent *content);
extern void nemoview_destroy(struct nemoview *view);

extern void nemoview_correct_pivot(struct nemoview *view, float px, float py);
extern void nemoview_unmap(struct nemoview *view);

extern void nemoview_schedule_repaint(struct nemoview *view);

extern void nemoview_update_output(struct nemoview *view);
extern void nemoview_update_transform(struct nemoview *view);
extern void nemoview_update_transform_done(struct nemoview *view);
extern void nemoview_update_transform_children(struct nemoview *view);
extern void nemoview_update_transform_parent(struct nemoview *view);

extern void nemoview_set_parent(struct nemoview *view, struct nemoview *parent);
extern void nemoview_set_scope(struct nemoview *view, uint32_t x, uint32_t y, uint32_t width, uint32_t height);

extern void nemoview_accumulate_damage(struct nemoview *view, pixman_region32_t *opaque);

extern void nemoview_attach_layer(struct nemoview *view, struct nemolayer *layer);
extern void nemoview_detach_layer(struct nemoview *view);
extern void nemoview_above_layer(struct nemoview *view, struct nemoview *above);
extern void nemoview_below_layer(struct nemoview *view, struct nemoview *below);

extern int nemoview_get_trapezoids(struct nemoview *view, int32_t x, int32_t y, int32_t width, int32_t height, pixman_trapezoid_t *traps);

static inline void nemoview_transform_dirty(struct nemoview *view)
{
	struct nemoview *child;

	if (view->transform.dirty)
		return;

	view->transform.dirty = 1;

	wl_list_for_each(child, &view->children_list, children_link) {
		nemoview_transform_dirty(child);
	}
}

static inline void nemoview_transform_done(struct nemoview *view)
{
	view->transform.done = 1;
}

static inline void nemoview_damage_dirty(struct nemoview *view)
{
	pixman_region32_t damage;

	pixman_region32_init(&damage);
	pixman_region32_subtract(&damage, &view->transform.boundingbox, &view->clip);
	pixman_region32_union(&view->compz->damage, &view->compz->damage, &damage);
	pixman_region32_fini(&damage);
}

static inline void nemoview_damage_below(struct nemoview *view)
{
	pixman_region32_t damage;

	pixman_region32_init(&damage);
	pixman_region32_subtract(&damage, &view->transform.boundingbox, &view->clip);
	pixman_region32_union(&view->compz->damage, &view->compz->damage, &damage);
	pixman_region32_fini(&damage);

	nemoview_schedule_repaint(view);
}

static inline void nemoview_clip_dirty(struct nemoview *view)
{
	pixman_region32_clear(&view->clip);
}

static inline void nemoview_set_position(struct nemoview *view, float x, float y)
{
	if (view->geometry.x == x && view->geometry.y == y)
		return;

	view->geometry.x = x;
	view->geometry.y = y;
	nemoview_transform_dirty(view);
}

static inline void nemoview_set_rotation(struct nemoview *view, float r)
{
	if (view->geometry.r == r)
		return;

	view->transform.enable = 1;

	view->geometry.r = r;
	view->transform.cosr = cos(r);
	view->transform.sinr = sin(r);
	nemoview_transform_dirty(view);
}

static inline void nemoview_set_scale(struct nemoview *view, float sx, float sy)
{
	if (view->geometry.sx == sx && view->geometry.sy == sy)
		return;

	view->transform.enable = 1;

	view->geometry.sx = sx;
	view->geometry.sy = sy;
	nemoview_transform_dirty(view);
}

static inline void nemoview_set_pivot(struct nemoview *view, float px, float py)
{
	if (view->geometry.px == px && view->geometry.py == py)
		return;

	nemoview_correct_pivot(view, px, py);

	view->geometry.has_pivot = 1;
	nemoview_transform_dirty(view);
}

static inline void nemoview_put_pivot(struct nemoview *view)
{
	if (view->geometry.has_pivot == 0)
		return;

	nemoview_correct_pivot(view, 0.0f, 0.0f);

	view->geometry.has_pivot = 0;
	nemoview_transform_dirty(view);
}

static inline void nemoview_set_anchor(struct nemoview *view, float ax, float ay)
{
	if (view->geometry.ax == ax && view->geometry.ay == ay)
		return;

	view->geometry.ax = ax;
	view->geometry.ay = ay;
	view->geometry.has_anchor = 1;
	nemoview_transform_dirty(view);
}

static inline void nemoview_set_flag(struct nemoview *view, float fx, float fy)
{
	view->geometry.fx = fx;
	view->geometry.fy = fy;
}

static inline void nemoview_set_alpha(struct nemoview *view, float alpha)
{
	view->alpha = alpha;
	nemoview_damage_dirty(view);
}

static inline void nemoview_transform_to_global(struct nemoview *view, float sx, float sy, float *x, float *y)
{
	if (view->transform.dirty)
		nemoview_update_transform(view);

	if (view->transform.enable) {
		struct nemovector v = { { sx, sy, 0.0f, 1.0f } };

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

static inline void nemoview_transform_from_global(struct nemoview *view, float x, float y, float *sx, float *sy)
{
	if (view->transform.dirty)
		nemoview_update_transform(view);

	if (view->transform.enable) {
		struct nemovector v = { { x, y, 0.0f, 1.0f } };

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

static inline void nemoview_transform_to_global_nocheck(struct nemoview *view, float sx, float sy, float *x, float *y)
{
	if (view->transform.enable) {
		struct nemovector v = { { sx, sy, 0.0f, 1.0f } };

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

static inline void nemoview_transform_from_global_nocheck(struct nemoview *view, float x, float y, float *sx, float *sy)
{
	if (view->transform.enable) {
		struct nemovector v = { { x, y, 0.0f, 1.0f } };

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

static inline int nemoview_overlap_view(struct nemoview *view, struct nemoview *oview)
{
	pixman_box32_t *box0;
	pixman_box32_t *box1;
	float b0[8];
	float b1[8];

	if (view->transform.dirty)
		nemoview_update_transform(view);
	if (oview->transform.dirty)
		nemoview_update_transform(oview);

	box0 = pixman_region32_extents(&view->geometry.scope);
	box1 = pixman_region32_extents(&oview->geometry.scope);

	nemoview_transform_to_global_nocheck(view, box0->x1, box0->y1, &b0[2*0+0], &b0[2*0+1]);
	nemoview_transform_to_global_nocheck(view, box0->x1, box0->y2, &b0[2*1+0], &b0[2*1+1]);
	nemoview_transform_to_global_nocheck(view, box0->x2, box0->y2, &b0[2*2+0], &b0[2*2+1]);
	nemoview_transform_to_global_nocheck(view, box0->x2, box0->y1, &b0[2*3+0], &b0[2*3+1]);

	nemoview_transform_to_global_nocheck(oview, box1->x1, box1->y1, &b1[2*0+0], &b1[2*0+1]);
	nemoview_transform_to_global_nocheck(oview, box1->x1, box1->y2, &b1[2*1+0], &b1[2*1+1]);
	nemoview_transform_to_global_nocheck(oview, box1->x2, box1->y2, &b1[2*2+0], &b1[2*2+1]);
	nemoview_transform_to_global_nocheck(oview, box1->x2, box1->y1, &b1[2*3+0], &b1[2*3+1]);

	return nemometro_intersect_boxes(b0, b1);
}

static inline void nemoview_set_state(struct nemoview *view, uint32_t state)
{
	view->state |= state;
}

static inline void nemoview_put_state(struct nemoview *view, uint32_t state)
{
	view->state &= ~state;
}

static inline int nemoview_has_state(struct nemoview *view, uint32_t state)
{
	return view->state & state;
}

static inline void nemoview_set_tag(struct nemoview *view, uint32_t tag)
{
	view->tag = tag;
}

static inline uint32_t nemoview_get_tag(struct nemoview *view)
{
	return view->tag;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
