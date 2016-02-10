#ifndef	__NEMOSHOW_POLY_H__
#define	__NEMOSHOW_POLY_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <showone.h>

#include <nemolist.h>
#include <nemolistener.h>

typedef enum {
	NEMOSHOW_POLY_X_VERTEX = 0,
	NEMOSHOW_POLY_Y_VERTEX = 1,
	NEMOSHOW_POLY_Z_VERTEX = 2,
	NEMOSHOW_POLY_TX_VERTEX = 3,
	NEMOSHOW_POLY_TY_VERTEX = 4,
} NemoShowPolyVertex;

typedef enum {
	NEMOSHOW_POLY_BLUE_COLOR = 0,
	NEMOSHOW_POLY_GREEN_COLOR = 1,
	NEMOSHOW_POLY_RED_COLOR = 2,
	NEMOSHOW_POLY_ALPHA_COLOR = 3
} NemoShowPolyColor;

typedef enum {
	NEMOSHOW_NONE_POLY = 0,
	NEMOSHOW_QUAD_POLY = 1,
	NEMOSHOW_QUAD_TEX_POLY = 2,
	NEMOSHOW_LAST_POLY
} NemoShowPolyType;

struct showpoly {
	struct showone base;

	float colors[4];

	float *vertices;
	int nvertices;
	int stride;

	GLuint varray;
	GLuint vbuffer;
	GLuint vindex;
	GLenum mode;
	int elements;

	int has_vbo;
};

#define NEMOSHOW_POLY(one)					((struct showpoly *)container_of(one, struct showpoly, base))
#define NEMOSHOW_POLY_AT(one, at)		(NEMOSHOW_POLY(one)->at)

#define NEMOSHOW_POLY_X_OFFSET(one, index)	(NEMOSHOW_POLY(one)->stride * index + NEMOSHOW_POLY_X_VERTEX)
#define NEMOSHOW_POLY_Y_OFFSET(one, index)	(NEMOSHOW_POLY(one)->stride * index + NEMOSHOW_POLY_Y_VERTEX)
#define NEMOSHOW_POLY_Z_OFFSET(one, index)	(NEMOSHOW_POLY(one)->stride * index + NEMOSHOW_POLY_Z_VERTEX)
#define NEMOSHOW_POLY_TX_OFFSET(one, index)	(NEMOSHOW_POLY(one)->stride * index + NEMOSHOW_POLY_TX_VERTEX)
#define NEMOSHOW_POLY_TY_OFFSET(one, index)	(NEMOSHOW_POLY(one)->stride * index + NEMOSHOW_POLY_TY_VERTEX)

extern struct showone *nemoshow_poly_create(int type);
extern void nemoshow_poly_destroy(struct showone *one);

extern void nemoshow_poly_attach_one(struct showone *parent, struct showone *one);
extern void nemoshow_poly_detach_one(struct showone *parent, struct showone *one);

extern int nemoshow_poly_arrange(struct showone *one);
extern int nemoshow_poly_update(struct showone *one);

extern void nemoshow_poly_set_canvas(struct showone *one, struct showone *canvas);
extern void nemoshow_poly_set_vbo(struct showone *one, int has_vbo);

static inline void nemoshow_poly_set_x(struct showone *one, int index, float x)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);

	poly->vertices[poly->stride * index + NEMOSHOW_POLY_X_VERTEX] = x;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

static inline void nemoshow_poly_set_y(struct showone *one, int index, float y)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);

	poly->vertices[poly->stride * index + NEMOSHOW_POLY_Y_VERTEX] = y;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

static inline void nemoshow_poly_set_z(struct showone *one, int index, float z)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);

	poly->vertices[poly->stride * index + NEMOSHOW_POLY_Z_VERTEX] = z;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

static inline void nemoshow_poly_set_vertex(struct showone *one, int index, float x, float y, float z)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);

	poly->vertices[poly->stride * index + NEMOSHOW_POLY_X_VERTEX] = x;
	poly->vertices[poly->stride * index + NEMOSHOW_POLY_Y_VERTEX] = y;
	poly->vertices[poly->stride * index + NEMOSHOW_POLY_Z_VERTEX] = z;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

static inline void nemoshow_poly_set_tx(struct showone *one, int index, float tx)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);

	poly->vertices[poly->stride * index + NEMOSHOW_POLY_TX_VERTEX] = tx;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

static inline void nemoshow_poly_set_ty(struct showone *one, int index, float ty)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);

	poly->vertices[poly->stride * index + NEMOSHOW_POLY_TY_VERTEX] = ty;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

static inline void nemoshow_poly_set_texcoord(struct showone *one, int index, float tx, float ty)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);

	poly->vertices[poly->stride * index + NEMOSHOW_POLY_TX_VERTEX] = tx;
	poly->vertices[poly->stride * index + NEMOSHOW_POLY_TY_VERTEX] = ty;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

static inline void nemoshow_poly_set_color(struct showone *one, float r, float g, float b, float a)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);

	poly->colors[NEMOSHOW_POLY_RED_COLOR] = r;
	poly->colors[NEMOSHOW_POLY_GREEN_COLOR] = g;
	poly->colors[NEMOSHOW_POLY_BLUE_COLOR] = b;
	poly->colors[NEMOSHOW_POLY_ALPHA_COLOR] = a;

	nemoshow_one_dirty(one, NEMOSHOW_STYLE_DIRTY);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
