#ifndef	__NEMOSHOW_PIPE_H__
#define	__NEMOSHOW_PIPE_H__

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
	NEMOSHOW_NONE_PIPE = 0,
	NEMOSHOW_SIMPLE_PIPE = 1,
	NEMOSHOW_TEXTURE_PIPE = 2,
	NEMOSHOW_LIGHTING_DIFFUSE_PIPE = 3,
	NEMOSHOW_LIGHTING_TEXTURE_PIPE = 4,
	NEMOSHOW_LIGHTING_DIFFUSE_TEXTURE_PIPE = 5,
	NEMOSHOW_LAST_PIPE
} NemoShowPipeType;

struct showpipe {
	struct showone base;

	GLuint program;
	GLuint vshader;
	GLuint fshader;

	GLuint uprojection;
	GLuint umodelview;
	GLuint ucolor;
	GLuint utex0;
	GLuint ulight;

	int vertex;
	int texcoord;
	int diffuse;
	int normal;

	float lights[4];

	struct nemomatrix projection;

	double tx, ty, tz;
	double sx, sy, sz;
	double rx, ry, rz;

	double ratio;
};

#define NEMOSHOW_PIPE(one)					((struct showpipe *)container_of(one, struct showpipe, base))
#define NEMOSHOW_PIPE_AT(one, at)		(NEMOSHOW_PIPE(one)->at)

extern struct showone *nemoshow_pipe_create(int type);
extern void nemoshow_pipe_destroy(struct showone *one);

extern void nemoshow_pipe_attach_one(struct showone *parent, struct showone *one);
extern void nemoshow_pipe_detach_one(struct showone *one);

extern int nemoshow_pipe_update(struct showone *one);

static inline void nemoshow_pipe_set_light(struct showone *one, float x, float y, float z, float a)
{
	struct showpipe *pipe = NEMOSHOW_PIPE(one);

	pipe->lights[0] = x;
	pipe->lights[1] = y;
	pipe->lights[2] = z;
	pipe->lights[3] = a;

	nemoshow_one_dirty(one, NEMOSHOW_REDRAW_DIRTY);
}

static inline void nemoshow_pipe_set_translate(struct showone *one, float tx, float ty, float tz)
{
	struct showpipe *pipe = NEMOSHOW_PIPE(one);

	pipe->tx = tx;
	pipe->ty = ty;
	pipe->tz = tz;

	nemoshow_one_dirty(one, NEMOSHOW_MATRIX_DIRTY);
}

static inline void nemoshow_pipe_set_scale(struct showone *one, float sx, float sy, float sz)
{
	struct showpipe *pipe = NEMOSHOW_PIPE(one);

	pipe->sx = sx;
	pipe->sy = sy;
	pipe->sz = sz;

	nemoshow_one_dirty(one, NEMOSHOW_MATRIX_DIRTY);
}

static inline void nemoshow_pipe_set_rotate(struct showone *one, float rx, float ry, float rz)
{
	struct showpipe *pipe = NEMOSHOW_PIPE(one);

	pipe->rx = rx;
	pipe->ry = ry;
	pipe->rz = rz;

	nemoshow_one_dirty(one, NEMOSHOW_MATRIX_DIRTY);
}

static inline void nemoshow_pipe_set_aspect_ratio(struct showone *one, float ratio)
{
	struct showpipe *pipe = NEMOSHOW_PIPE(one);

	pipe->ratio = ratio;

	nemoshow_one_dirty(one, NEMOSHOW_MATRIX_DIRTY);
}

static inline int nemoshow_pipe_get_vertex(struct showone *one)
{
	return NEMOSHOW_PIPE_AT(one, vertex);
}

static inline int nemoshow_pipe_get_texcoord(struct showone *one)
{
	return NEMOSHOW_PIPE_AT(one, texcoord);
}

static inline int nemoshow_pipe_get_diffuse(struct showone *one)
{
	return NEMOSHOW_PIPE_AT(one, diffuse);
}

static inline int nemoshow_pipe_get_normal(struct showone *one)
{
	return NEMOSHOW_PIPE_AT(one, normal);
}

extern int nemoshow_pipe_dispatch_one(struct showone *canvas, struct showone *one);

extern void nemoshow_canvas_render_pipeline(struct nemoshow *show, struct showone *one);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
