#ifndef	__NEMO_VIEW_H__
#define	__NEMO_VIEW_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <pixman.h>

#include <nemomatrix.h>

typedef enum {
	NEMO_VIEW_TRANSFORM_NORMAL = 0,
	NEMO_VIEW_TRANSFORM_OVERLAY = 1,
	NEMO_VIEW_TRANSFORM_LAST
} NemoViewTransformType;

typedef enum {
	NEMO_VIEW_INPUT_NORMAL = 0,
	NEMO_VIEW_INPUT_TOUCH = 1,
	NEMO_VIEW_INPUT_LAST
} NemoViewInputType;

typedef enum {
	NEMO_VIEW_TOP_AREA = 0,
	NEMO_VIEW_BOTTOM_AREA = 1,
	NEMO_VIEW_LEFT_AREA = 2,
	NEMO_VIEW_RIGHT_AREA = 3,
	NEMO_VIEW_LEFTTOP_AREA = 4,
	NEMO_VIEW_RIGHTTOP_AREA = 5,
	NEMO_VIEW_LEFTBOTTOM_AREA = 6,
	NEMO_VIEW_RIGHTBOTTOM_AREA = 7,
	NEMO_VIEW_CENTER_AREA,
	NEMO_VIEW_LAST_AREA = NEMO_VIEW_CENTER_AREA
} NemoViewArea;

typedef enum {
	NEMO_VIEW_CATCHABLE_STATE = (1 << 0),
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

	struct {
		uint32_t keyboard_count;
	} focus;

	struct {
		float px, py;
		int has_pivot;

		float ax, ay;
		int has_anchor;

		float x, y;
		float r;
		float sx, sy;

		int32_t width, height;

		struct wl_list transform_list;
	} geometry;

	struct {
		int enable;
		int dirty;
		int type;

		float cosr, sinr;

		pixman_region32_t boundingbox;
		pixman_region32_t opaque;

		struct nemomatrix matrix, inverse;
	} transform;

	struct {
		int type;
	} input;

	void *data;
};

extern struct nemoview *nemoview_create(struct nemocompz *compz, struct nemocontent *content);
extern void nemoview_destroy(struct nemoview *view);

extern void nemoview_set_position(struct nemoview *view, float x, float y);
extern void nemoview_set_rotation(struct nemoview *view, float r);
extern void nemoview_set_scale(struct nemoview *view, float sx, float sy);
extern void nemoview_set_pivot(struct nemoview *view, float px, float py);
extern void nemoview_set_anchor(struct nemoview *view, float ax, float ay);
extern void nemoview_correct_pivot(struct nemoview *view, float px, float py);
extern void nemoview_geometry_dirty(struct nemoview *view);

extern void nemoview_damage_dirty(struct nemoview *view);
extern void nemoview_clip_dirty(struct nemoview *view);
extern void nemoview_damage_below(struct nemoview *view);
extern int nemoview_is_mapped(struct nemoview *view);
extern void nemoview_unmap(struct nemoview *view);
extern void nemoview_schedule_repaint(struct nemoview *view);

extern void nemoview_transform_to_global(struct nemoview *view, float sx, float sy, float *x, float *y);
extern void nemoview_transform_from_global(struct nemoview *view, float x, float y, float *sx, float *sy);
extern int nemoview_get_point_area(struct nemoview *view, float x, float y);
extern void nemoview_update_transform(struct nemoview *view);
extern void nemoview_update_transform_children(struct nemoview *view);
extern void nemoview_update_transform_parent(struct nemoview *view);

extern void nemoview_set_parent(struct nemoview *view, struct nemoview *parent);

extern void nemoview_accumulate_damage(struct nemoview *view, pixman_region32_t *opaque);

extern void nemoview_attach_layer(struct nemoview *view, struct nemolayer *layer);
extern void nemoview_detach_layer(struct nemoview *view);
extern void nemoview_update_layer(struct nemoview *view);
extern void nemoview_above_layer(struct nemoview *view, struct nemoview *above);

extern int nemoview_get_trapezoids(struct nemoview *view, int32_t x, int32_t y, int32_t width, int32_t height, pixman_trapezoid_t *traps);

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

static inline int nemoview_support_touch_only(struct nemoview *view)
{
	return view->input.type == NEMO_VIEW_INPUT_TOUCH;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
