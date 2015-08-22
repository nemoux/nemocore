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
#define	NEMOSHOW_CANVAS_SRC_MAX			(64)

typedef enum {
	NEMOSHOW_CANVAS_NONE_TYPE = 0,
	NEMOSHOW_CANVAS_VECTOR_TYPE = 1,
	NEMOSHOW_CANVAS_SVG_TYPE = 2,
	NEMOSHOW_CANVAS_IMAGE_TYPE = 3,
	NEMOSHOW_CANVAS_REF_TYPE = 4,
	NEMOSHOW_CANVAS_USE_TYPE = 5,
	NEMOSHOW_CANVAS_SCENE_TYPE = 7,
	NEMOSHOW_CANVAS_OPENGL_TYPE = 8,
	NEMOSHOW_CANVAS_PIXMAN_TYPE = 9,
	NEMOSHOW_CANVAS_BACK_TYPE = 10,
	NEMOSHOW_CANVAS_LAST_TYPE
} NemoShowCanvasType;

struct nemoshow;
struct showmatrix;

typedef void (*nemoshow_canvas_dispatch_render_t)(struct nemoshow *show, struct showone *one);

struct showcanvas {
	struct showone base;

	char type[NEMOSHOW_CANVAS_TYPE_MAX];
	char src[NEMOSHOW_CANVAS_SRC_MAX];
	int32_t event;

	uint32_t fill;
	double fills[4];

	double alpha;

	struct showone *matrix;

	struct talenode *node;

	struct nemotale *tale;
	struct talefbo *fbo;

	double width, height;

	struct {
		double width, height;

		double sx, sy;
	} viewport;

	int needs_redraw;
	int needs_full_redraw;

	int needs_redraw_picker;
	int needs_full_redraw_picker;

	nemoshow_canvas_dispatch_render_t dispatch_render;

	void *cc;
};

#define NEMOSHOW_CANVAS(one)					((struct showcanvas *)container_of(one, struct showcanvas, base))
#define NEMOSHOW_CANVAS_AT(one, at)		(NEMOSHOW_CANVAS(one)->at)

extern struct showone *nemoshow_canvas_create(void);
extern void nemoshow_canvas_destroy(struct showone *one);

extern int nemoshow_canvas_arrange(struct nemoshow *show, struct showone *one);
extern int nemoshow_canvas_update(struct nemoshow *show, struct showone *one);

extern void nemoshow_canvas_render_vector(struct nemoshow *show, struct showone *one);
extern void nemoshow_canvas_render_picker(struct nemoshow *show, struct showone *one);
extern void nemoshow_canvas_render_back(struct nemoshow *show, struct showone *one);
extern void nemoshow_canvas_render_scene(struct nemoshow *show, struct showone *one);

extern int nemoshow_canvas_set_viewport(struct nemoshow *show, struct showone *one, double sx, double sy);

extern void nemoshow_canvas_damage_region(struct showone *one, int32_t x, int32_t y, int32_t width, int32_t height);
extern void nemoshow_canvas_damage_one(struct showone *one, struct showone *child);
extern void nemoshow_canvas_damage_all(struct showone *one);

extern int32_t nemoshow_canvas_get_pixel(struct showone *one, int x, int y);

static inline void nemoshow_canvas_set_dispatch_render(struct showone *one, nemoshow_canvas_dispatch_render_t dispatch_render)
{
	NEMOSHOW_CANVAS_AT(one, dispatch_render) = dispatch_render;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
