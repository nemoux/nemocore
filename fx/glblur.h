#ifndef	__NEMOFX_GL_BLUR_H__
#define	__NEMOFX_GL_BLUR_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

struct glblur {
	GLuint texture[2];
	GLuint fbo[2], dbo[2];

	GLuint program;

	GLint utexture;
	GLint uwidth, uheight;
	GLint udirectx, udirecty;
	GLint uradius;

	int32_t width, height;

	int32_t rx, ry;
};

extern struct glblur *nemofx_glblur_create(int32_t width, int32_t height);
extern void nemofx_glblur_destroy(struct glblur *blur);

extern void nemofx_glblur_set_radius(struct glblur *blur, int32_t rx, int32_t ry);

extern void nemofx_glblur_resize(struct glblur *blur, int32_t width, int32_t height);
extern void nemofx_glblur_dispatch(struct glblur *blur, uint32_t texture);

static inline int32_t nemofx_glblur_get_width(struct glblur *blur)
{
	return blur->width;
}

static inline int32_t nemofx_glblur_get_height(struct glblur *blur)
{
	return blur->height;
}

static inline uint32_t nemofx_glblur_get_texture(struct glblur *blur)
{
	return blur->texture[1];
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
