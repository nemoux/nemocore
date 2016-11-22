#ifndef	__NEMOSHOW_CANVAS_H__
#define	__NEMOSHOW_CANVAS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemotale.h>
#include <talenode.h>
#include <talepixman.h>
#include <talegl.h>

#include <showone.h>

#define	NEMOSHOW_CANVAS_TYPE_MAX		(32)

typedef enum {
	NEMOSHOW_CANVAS_NONE_TYPE = 0,
	NEMOSHOW_CANVAS_VECTOR_TYPE = 1,
	NEMOSHOW_CANVAS_OPENGL_TYPE = 2,
	NEMOSHOW_CANVAS_PIXMAN_TYPE = 3,
	NEMOSHOW_CANVAS_BACK_TYPE = 4,
	NEMOSHOW_CANVAS_LAST_TYPE
} NemoShowCanvasType;

typedef enum {
	NEMOSHOW_CANVAS_REDRAW_STATE = (1 << 0),
	NEMOSHOW_CANVAS_REDRAW_FULL_STATE = (1 << 1),
	NEMOSHOW_CANVAS_POOLING_STATE = (1 << 2),
	NEMOSHOW_CANVAS_OPAQUE_STATE = (1 << 3)
} NemoShowCanvasState;

struct nemoshow;
struct showevent;

typedef int (*nemoshow_canvas_prepare_render_t)(struct nemoshow *show, struct showone *one);
typedef void (*nemoshow_canvas_finish_render_t)(struct nemoshow *show, struct showone *one);
typedef void (*nemoshow_canvas_dispatch_redraw_t)(struct nemoshow *show, struct showone *one);
typedef void (*nemoshow_canvas_dispatch_record_t)(struct nemoshow *show, struct showone *one);
typedef void (*nemoshow_canvas_dispatch_replay_t)(struct nemoshow *show, struct showone *one, int32_t x, int32_t y, int32_t width, int32_t height);
typedef void (*nemoshow_canvas_dispatch_resize_t)(struct nemoshow *show, struct showone *one, int32_t width, int32_t height);
typedef void (*nemoshow_canvas_dispatch_event_t)(struct nemoshow *show, struct showone *one, struct showevent *event);
typedef int (*nemoshow_canvas_contain_point_t)(struct nemoshow *show, struct showone *one, float x, float y);

struct showcanvas {
	struct showone base;

	uint32_t state;

	struct nemoshow *show;
	struct nemolist redraw_link;

	char type[NEMOSHOW_CANVAS_TYPE_MAX];

	uint32_t fill;
	double fills[4];
	double alpha;

	struct talenode *node;

	double width, height;

	struct {
		double width, height;
		double sx, sy;
		double rx, ry;
	} viewport;

	double tx, ty;
	double ro;
	double px, py;
	double sx, sy;

	nemoshow_canvas_prepare_render_t prepare_render;
	nemoshow_canvas_finish_render_t finish_render;
	nemoshow_canvas_dispatch_redraw_t dispatch_redraw;
	nemoshow_canvas_dispatch_record_t dispatch_record;
	nemoshow_canvas_dispatch_replay_t dispatch_replay;
	nemoshow_canvas_dispatch_resize_t dispatch_resize;
	nemoshow_canvas_dispatch_event_t dispatch_event;
	nemoshow_canvas_contain_point_t contain_point;

	void *cc;
};

#define NEMOSHOW_CANVAS(one)					((struct showcanvas *)container_of(one, struct showcanvas, base))
#define NEMOSHOW_CANVAS_AT(one, at)		(NEMOSHOW_CANVAS(one)->at)
#define NEMOSHOW_CANVAS_ONE(canvas)		(&(canvas)->base)

extern struct showone *nemoshow_canvas_create(void);
extern void nemoshow_canvas_destroy(struct showone *one);

extern int nemoshow_canvas_update(struct showone *one);

extern void nemoshow_canvas_attach_one(struct showone *parent, struct showone *one);
extern void nemoshow_canvas_detach_one(struct showone *one);
extern int nemoshow_canvas_above_one(struct showone *one, struct showone *above);
extern int nemoshow_canvas_below_one(struct showone *one, struct showone *below);

extern int nemoshow_canvas_set_type(struct showone *one, int type);
extern void nemoshow_canvas_set_alpha(struct showone *one, double alpha);
extern void nemoshow_canvas_set_opaque(struct showone *one, int opaque);
extern int nemoshow_canvas_use_pbo(struct showone *one, int use_pbo);

extern int nemoshow_canvas_prepare_vector(struct nemoshow *show, struct showone *one);
extern void nemoshow_canvas_finish_vector(struct nemoshow *show, struct showone *one);
extern void nemoshow_canvas_render_vector(struct nemoshow *show, struct showone *one);
extern void nemoshow_canvas_record_vector(struct nemoshow *show, struct showone *one);
extern void nemoshow_canvas_replay_vector(struct nemoshow *show, struct showone *one, int32_t x, int32_t y, int32_t width, int32_t height);

extern void nemoshow_canvas_render_back(struct nemoshow *show, struct showone *one);

extern int nemoshow_canvas_prepare_none(struct nemoshow *show, struct showone *one);
extern void nemoshow_canvas_finish_none(struct nemoshow *show, struct showone *one);
extern void nemoshow_canvas_render_none(struct nemoshow *show, struct showone *one);

extern int nemoshow_canvas_prepare_opengl(struct nemoshow *show, struct showone *one);
extern void nemoshow_canvas_finish_opengl(struct nemoshow *show, struct showone *one);

extern int nemoshow_canvas_render(struct nemoshow *show, struct showone *one);

extern int nemoshow_canvas_set_viewport(struct showone *one, int32_t width, int32_t height);
extern int nemoshow_canvas_set_smooth(struct showone *one, int has_smooth);

extern void nemoshow_canvas_damage(struct showone *one, int32_t x, int32_t y, int32_t width, int32_t height);
extern void nemoshow_canvas_damage_one(struct showone *one, struct showone *child);
extern void nemoshow_canvas_damage_filter(struct showone *one);
extern void nemoshow_canvas_damage_all(struct showone *one);
extern void nemoshow_canvas_damage_below(struct showone *one);

extern struct showone *nemoshow_canvas_pick_one(struct showone *one, float x, float y);

extern void nemoshow_canvas_set_one(struct showone *canvas, struct showone *one);
extern void nemoshow_canvas_put_one(struct showone *one);
extern void nemoshow_canvas_set_ones(struct showone *canvas, struct showone *one);
extern void nemoshow_canvas_put_ones(struct showone *one);

extern void nemoshow_canvas_transform_to_global(struct showone *one, float sx, float sy, float *x, float *y);
extern void nemoshow_canvas_transform_from_global(struct showone *one, float x, float y, float *sx, float *sy);
extern void nemoshow_canvas_transform_to_viewport(struct showone *one, float x, float y, float *sx, float *sy);
extern void nemoshow_canvas_transform_from_viewport(struct showone *one, float sx, float sy, float *x, float *y);

static inline void nemoshow_canvas_set_state(struct showcanvas *canvas, uint32_t state)
{
	canvas->state |= state;
}

static inline void nemoshow_canvas_put_state(struct showcanvas *canvas, uint32_t state)
{
	canvas->state &= ~state;
}

static inline int nemoshow_canvas_has_state(struct showcanvas *canvas, uint32_t state)
{
	return canvas->state & state;
}

static inline int nemoshow_canvas_has_state_all(struct showcanvas *canvas, uint32_t state)
{
	return (canvas->state & state) == state;
}

static inline int nemoshow_canvas_is_type(struct showcanvas *canvas, int type)
{
	return NEMOSHOW_CANVAS_ONE(canvas)->sub == type;
}

static inline void nemoshow_canvas_set_prepare_render(struct showone *one, nemoshow_canvas_prepare_render_t prepare_render)
{
	NEMOSHOW_CANVAS_AT(one, prepare_render) = prepare_render;
}

static inline void nemoshow_canvas_set_finish_render(struct showone *one, nemoshow_canvas_finish_render_t finish_render)
{
	NEMOSHOW_CANVAS_AT(one, finish_render) = finish_render;
}

static inline void nemoshow_canvas_set_dispatch_redraw(struct showone *one, nemoshow_canvas_dispatch_redraw_t dispatch_redraw)
{
	NEMOSHOW_CANVAS_AT(one, dispatch_redraw) = dispatch_redraw;
}

static inline void nemoshow_canvas_set_dispatch_record(struct showone *one, nemoshow_canvas_dispatch_record_t dispatch_record)
{
	NEMOSHOW_CANVAS_AT(one, dispatch_record) = dispatch_record;
}

static inline void nemoshow_canvas_set_dispatch_replay(struct showone *one, nemoshow_canvas_dispatch_replay_t dispatch_replay)
{
	NEMOSHOW_CANVAS_AT(one, dispatch_replay) = dispatch_replay;
}

static inline void nemoshow_canvas_set_dispatch_resize(struct showone *one, nemoshow_canvas_dispatch_resize_t dispatch_resize)
{
	NEMOSHOW_CANVAS_AT(one, dispatch_resize) = dispatch_resize;
}

static inline void nemoshow_canvas_set_dispatch_event(struct showone *one, nemoshow_canvas_dispatch_event_t dispatch_event)
{
	NEMOSHOW_CANVAS_AT(one, dispatch_event) = dispatch_event;
}

static inline void nemoshow_canvas_set_contain_point(struct showone *one, nemoshow_canvas_contain_point_t contain_point)
{
	NEMOSHOW_CANVAS_AT(one, contain_point) = contain_point;
}

static inline struct talenode *nemoshow_canvas_get_node(struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	return canvas->node;
}

static inline pixman_image_t *nemoshow_canvas_get_pixman(struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	return nemotale_node_get_pixman(canvas->node);
}

static inline GLuint nemoshow_canvas_get_texture(struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	return nemotale_node_get_texture(canvas->node);
}

static inline GLuint nemoshow_canvas_get_effective_texture(struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	return nemotale_node_get_effective_texture(canvas->node);
}

static inline void *nemoshow_canvas_map(struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	return nemotale_node_map(canvas->node);
}

static inline int nemoshow_canvas_unmap(struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	return nemotale_node_unmap(canvas->node);
}

static inline int nemoshow_canvas_set_texture(struct showone *one, GLuint texture)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	return nemotale_node_set_texture(canvas->node, texture);
}

static inline double nemoshow_canvas_get_viewport_sx(struct showone *one)
{
	return NEMOSHOW_CANVAS_AT(one, viewport.sx);
}

static inline double nemoshow_canvas_get_viewport_sy(struct showone *one)
{
	return NEMOSHOW_CANVAS_AT(one, viewport.sy);
}

static inline int32_t nemoshow_canvas_get_viewport_width(struct showone *one)
{
	return NEMOSHOW_CANVAS_AT(one, viewport.width);
}

static inline int32_t nemoshow_canvas_get_viewport_height(struct showone *one)
{
	return NEMOSHOW_CANVAS_AT(one, viewport.height);
}

static inline void nemoshow_canvas_set_size(struct showone *one, int32_t width, int32_t height)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	canvas->width = width;
	canvas->height = height;

	nemoshow_one_set_state(one, NEMOSHOW_SIZE_STATE);

	nemoshow_one_dirty(one, NEMOSHOW_SIZE_DIRTY);
}

static inline void nemoshow_canvas_set_width(struct showone *one, double width)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	canvas->width = width;

	nemoshow_one_set_state(one, NEMOSHOW_SIZE_STATE);

	nemoshow_one_dirty(one, NEMOSHOW_SIZE_DIRTY);
}

static inline void nemoshow_canvas_set_height(struct showone *one, double height)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	canvas->height = height;

	nemoshow_one_set_state(one, NEMOSHOW_SIZE_STATE);

	nemoshow_one_dirty(one, NEMOSHOW_SIZE_DIRTY);
}

static inline double nemoshow_canvas_get_width(struct showone *one)
{
	return NEMOSHOW_CANVAS_AT(one, width);
}

static inline double nemoshow_canvas_get_height(struct showone *one)
{
	return NEMOSHOW_CANVAS_AT(one, height);
}

static inline double nemoshow_canvas_get_aspect_ratio(struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	return canvas->height / canvas->width;
}

static inline void nemoshow_canvas_set_fill_color(struct showone *one, double r, double g, double b, double a)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	canvas->fills[2] = r;
	canvas->fills[1] = g;
	canvas->fills[0] = b;
	canvas->fills[3] = a;

	canvas->fill = 1;

	nemoshow_one_dirty(one, NEMOSHOW_STYLE_DIRTY | NEMOSHOW_REDRAW_DIRTY);
}

static inline uint32_t nemoshow_canvas_pick_tag(struct showone *one, float x, float y)
{
	return nemoshow_one_get_tag(
			nemoshow_canvas_pick_one(one, x, y));
}

static inline void nemoshow_canvas_translate(struct showone *one, double tx, double ty)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	canvas->tx = tx;
	canvas->ty = ty;

	nemoshow_one_dirty(one, NEMOSHOW_MATRIX_DIRTY);
}

static inline void nemoshow_canvas_rotate(struct showone *one, double ro)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	canvas->ro = ro;

	nemoshow_one_dirty(one, NEMOSHOW_MATRIX_DIRTY);
}

static inline void nemoshow_canvas_pivot(struct showone *one, double px, double py)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	canvas->px = px;
	canvas->py = py;

	nemoshow_one_dirty(one, NEMOSHOW_MATRIX_DIRTY);
}

static inline void nemoshow_canvas_scale(struct showone *one, double sx, double sy)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	canvas->sx = sx;
	canvas->sy = sy;

	nemoshow_one_dirty(one, NEMOSHOW_MATRIX_DIRTY);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
