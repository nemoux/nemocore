#ifndef	__NEMOFX_GL_SWIRL_H__
#define	__NEMOFX_GL_SWIRL_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

struct glswirl {
	GLuint texture;
	GLuint fbo, dbo;

	GLuint program;

	GLint utexture;
	GLint uwidth, uheight;
	GLint uradius;
	GLint uangle;
	GLint ucenter;

	int32_t width, height;

	float radius;
	float angle;
	float center[2];
};

extern struct glswirl *nemofx_glswirl_create(int32_t width, int32_t height);
extern void nemofx_glswirl_destroy(struct glswirl *swirl);

extern void nemofx_glswirl_set_radius(struct glswirl *swirl, float radius);
extern void nemofx_glswirl_set_angle(struct glswirl *swirl, float angle);
extern void nemofx_glswirl_set_center(struct glswirl *swirl, float cx, float cy);

extern void nemofx_glswirl_resize(struct glswirl *swirl, int32_t width, int32_t height);
extern void nemofx_glswirl_dispatch(struct glswirl *swirl, uint32_t texture);

static inline float nemofx_glswirl_get_radius(struct glswirl *swirl)
{
	return swirl->radius;
}

static inline float nemofx_glswirl_get_angle(struct glswirl *swirl)
{
	return swirl->angle;
}

static inline float nemofx_glswirl_get_center_x(struct glswirl *swirl)
{
	return swirl->center[0];
}

static inline float nemofx_glswirl_get_center_y(struct glswirl *swirl)
{
	return swirl->center[1];
}

static inline uint32_t nemofx_glswirl_get_texture(struct glswirl *swirl)
{
	return swirl->texture;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
