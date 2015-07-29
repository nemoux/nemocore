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
	NEMOSHOW_CANVAS_LAST_TYPE
} NemoShowCanvasType;

struct nemoshow;

struct showcanvas {
	struct showone base;

	char type[NEMOSHOW_CANVAS_TYPE_MAX];
	char src[NEMOSHOW_CANVAS_SRC_MAX];
	int32_t event;

	struct talenode *node;

	double width, height;

	struct showone **items;
	int nitems, sitems;

	void *cc;
};

#define	NEMOSHOW_CANVAS(one)		((struct showcanvas *)container_of(one, struct showcanvas, base))

extern struct showone *nemoshow_canvas_create(void);
extern void nemoshow_canvas_destroy(struct showone *one);

extern int nemoshow_canvas_arrange(struct nemoshow *show, struct showone *one);
extern int nemoshow_canvas_update(struct nemoshow *show, struct showone *one);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
