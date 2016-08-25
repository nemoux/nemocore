#ifndef	__NEMOSHOW_POLY_H__
#define	__NEMOSHOW_POLY_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <showone.h>

#include <nemolist.h>
#include <nemolistener.h>
#include <nemomatrix.h>

typedef enum {
	NEMOSHOW_POLY_X_VERTEX = 0,
	NEMOSHOW_POLY_Y_VERTEX = 1,
	NEMOSHOW_POLY_Z_VERTEX = 2,
	NEMOSHOW_POLY_TX_VERTEX = 0,
	NEMOSHOW_POLY_TY_VERTEX = 1,
	NEMOSHOW_POLY_R_DIFFUSE = 0,
	NEMOSHOW_POLY_G_DIFFUSE = 1,
	NEMOSHOW_POLY_B_DIFFUSE = 2,
	NEMOSHOW_POLY_A_DIFFUSE = 3,
	NEMOSHOW_POLY_X_NORMAL = 0,
	NEMOSHOW_POLY_Y_NORMAL = 1,
	NEMOSHOW_POLY_Z_NORMAL = 2,
} NemoShowPolyOffset;

typedef enum {
	NEMOSHOW_POLY_BLUE_COLOR = 0,
	NEMOSHOW_POLY_GREEN_COLOR = 1,
	NEMOSHOW_POLY_RED_COLOR = 2,
	NEMOSHOW_POLY_ALPHA_COLOR = 3
} NemoShowPolyColor;

typedef enum {
	NEMOSHOW_NONE_POLY = 0,
	NEMOSHOW_QUAD_POLY = 1,
	NEMOSHOW_CUBE_POLY = 2,
	NEMOSHOW_MESH_POLY = 3,
	NEMOSHOW_LAST_POLY
} NemoShowPolyType;

struct showpoly {
	struct showone base;

	float colors[4];

	float *vertices;
	float *texcoords;
	float *diffuses;
	float *normals;
	int elements;

	float bounds[6];

	GLuint varray;
	GLuint vvertex;
	GLuint vtexcoord;
	GLuint vdiffuse;
	GLuint vnormal;
	GLuint vindex;
	GLenum mode;

	int on_texcoords;
	int on_diffuses;
	int on_normals;
	int on_vbo;

	struct nemomatrix modelview;

	double tx, ty, tz;
	double sx, sy, sz;
	double rx, ry, rz;
};

#define NEMOSHOW_POLY(one)					((struct showpoly *)container_of(one, struct showpoly, base))
#define NEMOSHOW_POLY_AT(one, at)		(NEMOSHOW_POLY(one)->at)

#define NEMOSHOW_POLY_X_OFFSET(one, index)		(3 * index + NEMOSHOW_POLY_X_VERTEX)
#define NEMOSHOW_POLY_Y_OFFSET(one, index)		(3 * index + NEMOSHOW_POLY_Y_VERTEX)
#define NEMOSHOW_POLY_Z_OFFSET(one, index)		(3 * index + NEMOSHOW_POLY_Z_VERTEX)
#define NEMOSHOW_POLY_TX_OFFSET(one, index)		(2 * index + NEMOSHOW_POLY_TX_VERTEX)
#define NEMOSHOW_POLY_TY_OFFSET(one, index)		(2 * index + NEMOSHOW_POLY_TY_VERTEX)

extern struct showone *nemoshow_poly_create(int type);
extern void nemoshow_poly_destroy(struct showone *one);

extern void nemoshow_poly_attach_one(struct showone *parent, struct showone *one);
extern void nemoshow_poly_detach_one(struct showone *one);

extern int nemoshow_poly_update(struct showone *one);

extern void nemoshow_poly_set_canvas(struct showone *one, struct showone *canvas);

extern void nemoshow_poly_set_vertices(struct showone *one, float *vertices, int elements);

extern void nemoshow_poly_transform_vertices(struct showone *one, struct nemomatrix *matrix);

extern void nemoshow_poly_use_texcoords(struct showone *one, int on_texcoords);
extern void nemoshow_poly_use_normals(struct showone *one, int on_normals);
extern void nemoshow_poly_use_diffuses(struct showone *one, int on_diffuses);
extern void nemoshow_poly_use_vbo(struct showone *one, int on_vbo);

extern int nemoshow_poly_pick_plane(struct showone *cone, struct showone *pone, struct showone *one, double x, double y, float *tx, float *ty);
extern float nemoshow_poly_contain_point(struct showone *cone, struct showone *pone, struct showone *one, double x, double y);

extern int nemoshow_poly_load_obj(struct showone *one, const char *uri);

static inline void nemoshow_poly_set_x(struct showone *one, int index, float x)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);

	poly->vertices[3 * index + NEMOSHOW_POLY_X_VERTEX] = x;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

static inline void nemoshow_poly_set_y(struct showone *one, int index, float y)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);

	poly->vertices[3 * index + NEMOSHOW_POLY_Y_VERTEX] = y;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

static inline void nemoshow_poly_set_z(struct showone *one, int index, float z)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);

	poly->vertices[3 * index + NEMOSHOW_POLY_Z_VERTEX] = z;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

static inline void nemoshow_poly_set_vertex(struct showone *one, int index, float x, float y, float z)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);

	poly->vertices[3 * index + NEMOSHOW_POLY_X_VERTEX] = x;
	poly->vertices[3 * index + NEMOSHOW_POLY_Y_VERTEX] = y;
	poly->vertices[3 * index + NEMOSHOW_POLY_Z_VERTEX] = z;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

static inline void nemoshow_poly_set_tx(struct showone *one, int index, float tx)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);

	poly->texcoords[2 * index + NEMOSHOW_POLY_TX_VERTEX] = tx;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

static inline void nemoshow_poly_set_ty(struct showone *one, int index, float ty)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);

	poly->texcoords[2 * index + NEMOSHOW_POLY_TY_VERTEX] = ty;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

static inline void nemoshow_poly_set_texcoord(struct showone *one, int index, float tx, float ty)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);

	poly->texcoords[2 * index + NEMOSHOW_POLY_TX_VERTEX] = tx;
	poly->texcoords[2 * index + NEMOSHOW_POLY_TY_VERTEX] = ty;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

static inline void nemoshow_poly_set_diffuse(struct showone *one, int index, float r, float g, float b, float a)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);

	poly->diffuses[4 * index + NEMOSHOW_POLY_R_DIFFUSE] = r;
	poly->diffuses[4 * index + NEMOSHOW_POLY_G_DIFFUSE] = g;
	poly->diffuses[4 * index + NEMOSHOW_POLY_B_DIFFUSE] = b;
	poly->diffuses[4 * index + NEMOSHOW_POLY_A_DIFFUSE] = a;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

static inline void nemoshow_poly_set_normal(struct showone *one, int index, float x, float y, float z)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);

	poly->normals[3 * index + NEMOSHOW_POLY_X_NORMAL] = x;
	poly->normals[3 * index + NEMOSHOW_POLY_Y_NORMAL] = y;
	poly->normals[3 * index + NEMOSHOW_POLY_Z_NORMAL] = z;

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

static inline void nemoshow_poly_translate(struct showone *one, float tx, float ty, float tz)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);

	poly->tx = tx;
	poly->ty = ty;
	poly->tz = tz;

	nemoshow_one_set_state(one, NEMOSHOW_TRANSFORM_STATE);

	nemoshow_one_dirty(one, NEMOSHOW_MATRIX_DIRTY);
}

static inline void nemoshow_poly_scale(struct showone *one, float sx, float sy, float sz)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);

	poly->sx = sx;
	poly->sy = sy;
	poly->sz = sz;

	nemoshow_one_set_state(one, NEMOSHOW_TRANSFORM_STATE);

	nemoshow_one_dirty(one, NEMOSHOW_MATRIX_DIRTY);
}

static inline void nemoshow_poly_rotate(struct showone *one, float rx, float ry, float rz)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);

	poly->rx = rx;
	poly->ry = ry;
	poly->rz = rz;

	nemoshow_one_set_state(one, NEMOSHOW_TRANSFORM_STATE);

	nemoshow_one_dirty(one, NEMOSHOW_MATRIX_DIRTY);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
