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
#include <polyhelper.h>

typedef enum {
	NEMOVIEW_TRANSFORM_NORMAL = 0,
	NEMOVIEW_TRANSFORM_OVERLAY = 1,
	NEMOVIEW_TRANSFORM_LAST
} NemoViewTransformType;

typedef enum {
	NEMOVIEW_MAP_STATE = (1 << 0),
	NEMOVIEW_PICK_STATE = (1 << 1),
	NEMOVIEW_KEYPAD_STATE = (1 << 2),
	NEMOVIEW_SOUND_STATE = (1 << 3),
	NEMOVIEW_LAYER_STATE = (1 << 4),
	NEMOVIEW_REGION_STATE = (1 << 5),
	NEMOVIEW_SCOPE_STATE = (1 << 6),
	NEMOVIEW_STAGE_STATE = (1 << 7),
	NEMOVIEW_OPAQUE_STATE = (1 << 8),
	NEMOVIEW_CANVAS_STATE = (1 << 9),
	NEMOVIEW_SMOOTH_STATE = (1 << 10),
	NEMOVIEW_EFFECT_STATE = (1 << 11),
	NEMOVIEW_CLOSE_STATE = (1 << 12),
	NEMOVIEW_LAST_STATE
} NemoViewState;

struct nemocompz;
struct nemolayer;
struct nemocontent;
struct nemocanvas;
struct nemoxkb;
struct nemoscope;

struct nemotransform {
	struct nemomatrix matrix;
	struct wl_list link;
};

struct nemoview {
	struct nemocompz *compz;

	struct nemocontent *content;

	struct nemocanvas *canvas;
	struct wl_listener canvas_destroy_listener;

	char *type;
	char uuid[38];
	uint32_t tag;

	struct wl_signal destroy_signal;

	struct wl_client *client;

	struct wl_list link;
	struct wl_list layer_link;

	struct nemoview *parent;
	struct wl_listener parent_destroy_listener;
	struct wl_list children_list;
	struct wl_list children_link;

	struct nemoview *focus;
	struct wl_listener focus_destroy_listener;

	struct nemolayer *layer;

	struct nemoscreen *screen;

	uint32_t state;

	uint32_t node_mask;
	uint32_t screen_mask;

	uint32_t psf_flags;

	pixman_region32_t clip;
	float alpha;

	struct nemoscope *scope;

	uint32_t keyboard_count;

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

		pixman_region32_t region;

		struct wl_list transform_list;
	} geometry;

	struct {
		int enable;
		int dirty;
		int notify;
		int type;

		float cosr, sinr;

		pixman_region32_t boundingbox;
		pixman_region32_t opaque;

		struct nemomatrix matrix, inverse;
	} transform;

	struct nemoxkb *xkb;

	int grabcount;

	uint32_t lasttouch;

	void *data;
};

extern struct nemoview *nemoview_create(struct nemocompz *compz);
extern void nemoview_destroy(struct nemoview *view);

extern void nemoview_correct_pivot(struct nemoview *view, float px, float py);
extern void nemoview_unmap(struct nemoview *view);

extern void nemoview_schedule_repaint(struct nemoview *view);

extern void nemoview_update_output(struct nemoview *view);
extern void nemoview_update_transform(struct nemoview *view);
extern void nemoview_update_transform_notify(struct nemoview *view);
extern void nemoview_update_transform_children(struct nemoview *view);
extern void nemoview_update_transform_parent(struct nemoview *view);

extern void nemoview_set_type(struct nemoview *view, const char *type);
extern void nemoview_set_uuid(struct nemoview *view, const char *uuid);

extern void nemoview_set_parent(struct nemoview *view, struct nemoview *parent);
extern void nemoview_set_focus(struct nemoview *view, struct nemoview *focus);

extern void nemoview_set_region(struct nemoview *view, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
extern void nemoview_put_region(struct nemoview *view);

extern void nemoview_set_scope(struct nemoview *view, const char *cmds);
extern void nemoview_put_scope(struct nemoview *view);

extern void nemoview_accumulate_damage(struct nemoview *view, pixman_region32_t *opaque);
extern void nemoview_merge_damage(struct nemoview *view, pixman_region32_t *damage);

extern void nemoview_attach_canvas(struct nemoview *view, struct nemocanvas *canvas);
extern void nemoview_detach_canvas(struct nemoview *view);

extern void nemoview_attach_layer(struct nemoview *view, struct nemolayer *layer);
extern void nemoview_detach_layer(struct nemoview *view);
extern void nemoview_above_layer(struct nemoview *view, struct nemoview *above);
extern void nemoview_below_layer(struct nemoview *view, struct nemoview *below);

extern int nemoview_get_trapezoids(struct nemoview *view, int32_t x, int32_t y, int32_t width, int32_t height, pixman_trapezoid_t *traps);

extern int nemoview_update_modifiers(struct nemoview *view);

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

static inline void nemoview_transform_notify(struct nemoview *view)
{
	view->transform.notify = 1;
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

static inline void nemoview_rotate_to_global(struct nemoview *view, float s, float *r)
{
	*r = s + view->geometry.r;
}

static inline void nemoview_rotate_from_global(struct nemoview *view, float r, float *s)
{
	*s = r - view->geometry.r;
}

static inline void nemoview_transform_to_global(struct nemoview *view, float sx, float sy, float *x, float *y)
{
	if (view->transform.dirty)
		nemoview_update_transform(view);

	if (view->transform.enable) {
		struct nemovector v = { { sx, sy, 0.0f, 1.0f } };

		nemomatrix_transform_vector(&view->transform.matrix, &v);

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

		nemomatrix_transform_vector(&view->transform.inverse, &v);

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

		nemomatrix_transform_vector(&view->transform.matrix, &v);

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

		nemomatrix_transform_vector(&view->transform.inverse, &v);

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

static inline void nemoview_transform_to_local(struct nemoview *view, struct nemoview *other, float x, float y, float *sx, float *sy)
{
	float tx, ty;

	nemoview_transform_to_global(view, x, y, &tx, &ty);
	nemoview_transform_from_global(other, tx, ty, sx, sy);
}

static inline void nemoview_transform_from_local(struct nemoview *view, struct nemoview *other, float x, float y, float *sx, float *sy)
{
	float tx, ty;

	nemoview_transform_to_global(other, x, y, &tx, &ty);
	nemoview_transform_from_global(view, tx, ty, sx, sy);
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

	box0 = pixman_region32_extents(&view->geometry.region);
	box1 = pixman_region32_extents(&oview->geometry.region);

	nemoview_transform_to_global_nocheck(view, box0->x1, box0->y1, &b0[2*0+0], &b0[2*0+1]);
	nemoview_transform_to_global_nocheck(view, box0->x1, box0->y2, &b0[2*1+0], &b0[2*1+1]);
	nemoview_transform_to_global_nocheck(view, box0->x2, box0->y2, &b0[2*2+0], &b0[2*2+1]);
	nemoview_transform_to_global_nocheck(view, box0->x2, box0->y1, &b0[2*3+0], &b0[2*3+1]);

	nemoview_transform_to_global_nocheck(oview, box1->x1, box1->y1, &b1[2*0+0], &b1[2*0+1]);
	nemoview_transform_to_global_nocheck(oview, box1->x1, box1->y2, &b1[2*1+0], &b1[2*1+1]);
	nemoview_transform_to_global_nocheck(oview, box1->x2, box1->y2, &b1[2*2+0], &b1[2*2+1]);
	nemoview_transform_to_global_nocheck(oview, box1->x2, box1->y1, &b1[2*3+0], &b1[2*3+1]);

	return poly_intersect_boxes(b0, b1);
}

static inline int nemoview_contain_point(struct nemoview *view, float x, float y)
{
	float sx, sy;

	nemoview_transform_from_global(view, x, y, &sx, &sy);

	if (pixman_region32_contains_point(&view->geometry.region, sx, sy, NULL))
		return 1;

	return 0;
}

static inline int nemoview_contain_view(struct nemoview *view, struct nemoview *oview)
{
	pixman_box32_t *box;
	float s[4][2];
	float tx, ty;
	int i;

	if (view->transform.dirty)
		nemoview_update_transform(view);
	if (oview->transform.dirty)
		nemoview_update_transform(oview);

	box = pixman_region32_extents(&oview->geometry.region);
	if (box->x1 >= box->x2 || box->y1 >= box->y2)
		return 0;

	s[0][0] = s[2][0] = box->x1 + 1.0f;
	s[0][1] = s[1][1] = box->y1 + 1.0f;
	s[1][0] = s[3][0] = box->x2 - 1.0f;
	s[2][1] = s[3][1] = box->y2 - 1.0f;

	for (i = 0; i < 4; i++) {
		nemoview_transform_to_global_nocheck(oview, s[i][0], s[i][1], &tx, &ty);

		if (nemoview_contain_point(view, tx, ty) == 0)
			return 0;
	}

	return 1;
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

static inline int nemoview_has_state_all(struct nemoview *view, uint32_t state)
{
	return (view->state & state) == state;
}

static inline int nemoview_has_type(struct nemoview *view, const char *type)
{
	return view->type != NULL && strcmp(view->type, type) == 0;
}

static inline int nemoview_grab(struct nemoview *view)
{
	return view->grabcount++;
}

static inline int nemoview_ungrab(struct nemoview *view)
{
	return --view->grabcount;
}

static inline int nemoview_has_grab(struct nemoview *view)
{
	return view->grabcount > 0;
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
