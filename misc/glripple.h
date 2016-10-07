#ifndef __GL_RIPPLE_H__
#define __GL_RIPPLE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

struct glripple {
	GLuint texture;
	GLuint fbo, dbo;

	GLuint program;

	GLuint varray;
	GLuint vvertex;
	GLuint vtexcoord;
	GLuint vindex;

	GLuint utexture;

	int32_t width, height;
	int32_t rows, columns;
	int32_t elements;
};

extern struct glripple *glripple_create(int32_t width, int32_t height);
extern void glripple_destroy(struct glripple *ripple);

extern void glripple_layout(struct glripple *ripple, int32_t rows, int32_t columns);
extern void glripple_resize(struct glripple *ripple, int32_t width, int32_t height);
extern void glripple_dispatch(struct glripple *ripple, GLuint texture);

static inline int32_t glripple_get_width(struct glripple *ripple)
{
	return ripple->width;
}

static inline int32_t glripple_get_height(struct glripple *ripple)
{
	return ripple->height;
}

static inline int32_t glripple_get_rows(struct glripple *ripple)
{
	return ripple->rows;
}

static inline int32_t glripple_get_columns(struct glripple *ripple)
{
	return ripple->columns;
}

static inline GLuint glripple_get_texture(struct glripple *ripple)
{
	return ripple->texture;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
