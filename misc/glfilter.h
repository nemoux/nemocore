#ifndef	__GL_FILTER_H__
#define	__GL_FILTER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

struct glfilter {
	GLuint texture;
	GLuint fbo, dbo;

	GLuint program;

	GLuint utexture;
	GLuint uwidth;
	GLuint uheight;
	GLuint utime;

	int32_t width, height;
};

extern struct glfilter *glfilter_create(int32_t width, int32_t height, const char *shaderpath);
extern void glfilter_destroy(struct glfilter *filter);

extern void glfilter_resize(struct glfilter *filter, int32_t width, int32_t height);
extern void glfilter_dispatch(struct glfilter *filter, GLuint texture);

static inline int32_t glfilter_get_width(struct glfilter *filter)
{
	return filter->width;
}

static inline int32_t glfilter_get_height(struct glfilter *filter)
{
	return filter->height;
}

static inline GLuint glfilter_get_texture(struct glfilter *filter)
{
	return filter->texture;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
