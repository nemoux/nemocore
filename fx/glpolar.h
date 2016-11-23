#ifndef	__NEMOFX_GL_POLAR_H__
#define	__NEMOFX_GL_POLAR_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

struct glpolar {
	GLuint texture;
	GLuint fbo, dbo;

	GLuint program;

	GLuint utexture;
	GLuint uwidth, uheight;
	GLuint ucolor;

	int32_t width, height;

	float color[4];
};

extern struct glpolar *nemofx_glpolar_create(int32_t width, int32_t height);
extern void nemofx_glpolar_destroy(struct glpolar *polar);

extern void nemofx_glpolar_set_color(struct glpolar *polar, float r, float g, float b, float a);

extern void nemofx_glpolar_resize(struct glpolar *polar, int32_t width, int32_t height);
extern void nemofx_glpolar_dispatch(struct glpolar *polar, GLuint texture);

static inline GLuint nemofx_glpolar_get_texture(struct glpolar *polar)
{
	return polar->texture;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
