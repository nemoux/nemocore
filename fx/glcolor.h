#ifndef	__NEMOFX_GL_COLOR_H__
#define	__NEMOFX_GL_COLOR_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

struct glcolor {
	GLuint texture;
	GLuint fbo, dbo;

	GLuint program;

	GLint ucolor;

	int32_t width, height;

	float color[4];
};

extern struct glcolor *nemofx_glcolor_create(int32_t width, int32_t height);
extern void nemofx_glcolor_destroy(struct glcolor *color);

extern void nemofx_glcolor_set_color(struct glcolor *color, float r, float g, float b, float a);

extern void nemofx_glcolor_resize(struct glcolor *color, int32_t width, int32_t height);
extern void nemofx_glcolor_dispatch(struct glcolor *color);

static inline GLuint nemofx_glcolor_get_texture(struct glcolor *color)
{
	return color->texture;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
