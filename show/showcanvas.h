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
	NEMOSHOW_CANVAS_SCENE_TYPE = 6,
	NEMOSHOW_CANVAS_OPENGL_TYPE = 7,
	NEMOSHOW_CANVAS_PIXMAN_TYPE = 8,
	NEMOSHOW_CANVAS_BACK_TYPE = 9,
	NEMOSHOW_CANVAS_LAST_TYPE
} NemoShowCanvasType;

struct nemoshow;
struct showmatrix;

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

	void *cc;
};

#define NEMOSHOW_CANVAS(one)					((struct showcanvas *)container_of(one, struct showcanvas, base))
#define NEMOSHOW_CANVAS_AT(one, at)		(NEMOSHOW_CANVAS(one)->at)

extern struct showone *nemoshow_canvas_create(void);
extern void nemoshow_canvas_destroy(struct showone *one);

extern int nemoshow_canvas_arrange(struct nemoshow *show, struct showone *one);
extern int nemoshow_canvas_update(struct nemoshow *show, struct showone *one);

extern void nemoshow_canvas_render_vector(struct nemoshow *show, struct showone *one);
extern void nemoshow_canvas_render_back(struct nemoshow *show, struct showone *one);
extern void nemoshow_canvas_render_scene(struct nemoshow *show, struct showone *one);

static inline struct talenode *nemoshow_canvas_get_node(struct showone *one)
{
	return NEMOSHOW_CANVAS_AT(one, node);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
