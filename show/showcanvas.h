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
#include <showpipe.h>

#define	NEMOSHOW_CANVAS_TYPE_MAX		(32)

typedef enum {
	NEMOSHOW_CANVAS_NONE_TYPE = 0,
	NEMOSHOW_CANVAS_VECTOR_TYPE = 1,
	NEMOSHOW_CANVAS_OPENGL_TYPE = 2,
	NEMOSHOW_CANVAS_PIXMAN_TYPE = 3,
	NEMOSHOW_CANVAS_BACK_TYPE = 4,
	NEMOSHOW_CANVAS_PIPELINE_TYPE = 5,
	NEMOSHOW_CANVAS_LAST_TYPE
} NemoShowCanvasType;

struct nemoshow;

typedef void (*nemoshow_canvas_dispatch_redraw_t)(struct nemoshow *show, struct showone *one);
typedef void (*nemoshow_canvas_dispatch_resize_t)(struct nemoshow *show, struct showone *one, int32_t width, int32_t height);
typedef void (*nemoshow_canvas_dispatch_event_t)(struct nemoshow *show, struct showone *one, void *event);

struct showcanvas {
	struct showone base;

	struct nemoshow *show;

	char type[NEMOSHOW_CANVAS_TYPE_MAX];

	uint32_t fill;
	double fills[4];

	double alpha;

	struct talenode *node;

	GLuint fbo, dbo;

	double width, height;

	struct {
		int dirty;

		double width, height;

		double sx, sy;
	} viewport;

	double tx, ty;
	double ro;
	double px, py;
	double sx, sy;

	int needs_resize;
	int needs_redraw;
	int needs_full_redraw;

	nemoshow_canvas_dispatch_redraw_t dispatch_redraw;
	nemoshow_canvas_dispatch_resize_t dispatch_resize;
	nemoshow_canvas_dispatch_event_t dispatch_event;

	void *cc;
};

#define NEMOSHOW_CANVAS(one)					((struct showcanvas *)container_of(one, struct showcanvas, base))
#define NEMOSHOW_CANVAS_AT(one, at)		(NEMOSHOW_CANVAS(one)->at)

extern struct showone *nemoshow_canvas_create(void);
extern void nemoshow_canvas_destroy(struct showone *one);

extern int nemoshow_canvas_arrange(struct showone *one);
extern int nemoshow_canvas_update(struct showone *one);

extern int nemoshow_canvas_set_type(struct showone *one, int type);
extern int nemoshow_canvas_resize(struct showone *one);
extern void nemoshow_canvas_set_event(struct showone *one, uint32_t event);
extern uint32_t nemoshow_canvas_get_event(struct showone *one);
extern void nemoshow_canvas_set_alpha(struct showone *one, double alpha);
extern int nemoshow_canvas_set_filter(struct showone *one, const char *shader);
extern int nemoshow_canvas_load_filter(struct showone *one, const char *shaderpath);

extern int nemoshow_canvas_attach_pixman(struct showone *one, void *data, int32_t width, int32_t height);
extern void nemoshow_canvas_detach_pixman(struct showone *one);
extern int nemoshow_canvas_resize_pixman(struct showone *one, int32_t width, int32_t height);

extern void nemoshow_canvas_render_vector(struct nemoshow *show, struct showone *one);
extern void nemoshow_canvas_render_back(struct nemoshow *show, struct showone *one);

extern void nemoshow_canvas_flush_now(struct nemoshow *show, struct showone *one);

extern int nemoshow_canvas_set_viewport(struct nemoshow *show, struct showone *one, double sx, double sy);

extern void nemoshow_canvas_damage_one(struct showone *one, struct showone *child);
extern void nemoshow_canvas_damage_all(struct showone *one);
extern void nemoshow_canvas_damage_filter(struct showone *one);
extern void nemoshow_canvas_dirty_all(struct showone *one, uint32_t dirty);

extern struct showone *nemoshow_canvas_pick_one(struct showone *one, float x, float y);

extern void nemoshow_canvas_attach_one(struct showone *canvas, struct showone *one);
extern void nemoshow_canvas_detach_one(struct showone *one);
extern void nemoshow_canvas_attach_ones(struct showone *canvas, struct showone *one);
extern void nemoshow_canvas_detach_ones(struct showone *one);

static inline void nemoshow_canvas_set_dispatch_redraw(struct showone *one, nemoshow_canvas_dispatch_redraw_t dispatch_redraw)
{
	NEMOSHOW_CANVAS_AT(one, dispatch_redraw) = dispatch_redraw;
}

static inline void nemoshow_canvas_set_dispatch_resize(struct showone *one, nemoshow_canvas_dispatch_resize_t dispatch_resize)
{
	NEMOSHOW_CANVAS_AT(one, dispatch_resize) = dispatch_resize;
}

static inline void nemoshow_canvas_set_dispatch_event(struct showone *one, nemoshow_canvas_dispatch_event_t dispatch_event)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	if (dispatch_event == NULL) {
		nemotale_node_set_pick_type(canvas->node, NEMOTALE_PICK_NO_TYPE);
	} else {
		nemotale_node_set_pick_type(canvas->node, NEMOTALE_PICK_DEFAULT_TYPE);

		if (nemotale_node_get_id(canvas->node) == 0)
			nemotale_node_set_id(canvas->node, 1);
	}

	NEMOSHOW_CANVAS_AT(one, dispatch_event) = dispatch_event;
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

static inline void nemoshow_canvas_set_width(struct showone *one, double width)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	canvas->width = width;
	canvas->needs_resize = 1;
}

static inline void nemoshow_canvas_set_height(struct showone *one, double height)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	canvas->height = height;
	canvas->needs_resize = 1;
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
}

static inline uint32_t nemoshow_canvas_pick_tag(struct showone *one, float x, float y)
{
	return nemoshow_one_get_tag(
			nemoshow_canvas_pick_one(one, x, y));
}

static inline void nemoshow_canvas_translate(struct showone *one, double tx, double ty)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_translate(canvas->node, tx, ty);

	canvas->tx = tx;
	canvas->ty = ty;
}

static inline void nemoshow_canvas_rotate(struct showone *one, double ro)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_rotate(canvas->node, ro * M_PI / 180.0f);

	canvas->ro = ro;
}

static inline void nemoshow_canvas_pivot(struct showone *one, double px, double py)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_pivot(canvas->node, px, py);

	canvas->px = px;
	canvas->py = py;
}

static inline void nemoshow_canvas_scale(struct showone *one, double sx, double sy)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_scale(canvas->node, sx, sy);

	canvas->sx = sx;
	canvas->sy = sy;
}

static inline void nemoshow_canvas_transform_to_global(struct showone *one, float sx, float sy, float *x, float *y)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_transform_to_global(canvas->node, sx, sy, x, y);
}

static inline void nemoshow_canvas_transform_from_global(struct showone *one, float x, float y, float *sx, float *sy)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_transform_from_global(canvas->node, x, y, sx, sy);
}

static inline void nemoshow_canvas_redraw_one(struct nemoshow *show, struct showone *one)
{
	if (one->sub == NEMOSHOW_CANVAS_VECTOR_TYPE) {
		nemoshow_canvas_render_vector(show, one);
	} else if (one->sub == NEMOSHOW_CANVAS_PIPELINE_TYPE) {
		nemoshow_canvas_render_pipeline(show, one);
	} else if (one->sub == NEMOSHOW_CANVAS_BACK_TYPE) {
		nemoshow_canvas_render_back(show, one);
	}
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
