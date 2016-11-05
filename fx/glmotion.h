#ifndef	__NEMOFX_GL_MOTION_H__
#define	__NEMOFX_GL_MOTION_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

struct glmotion {
	GLuint texture[2];
	GLuint fbo[2], dbo[2];

	GLuint program;

	GLuint utexture;
	GLuint uwidth, uheight;
	GLuint udirectx, udirecty;
	GLuint uvelocity;

	int32_t width, height;

	float velocity;
};

extern struct glmotion *nemofx_glmotion_create(int32_t width, int32_t height);
extern void nemofx_glmotion_destroy(struct glmotion *motion);

extern void nemofx_glmotion_set_velocity(struct glmotion *motion, float velocity);

extern void nemofx_glmotion_resize(struct glmotion *motion, int32_t width, int32_t height);
extern void nemofx_glmotion_dispatch(struct glmotion *motion, GLuint texture);
extern void nemofx_glmotion_clear(struct glmotion *motion);

static inline float nemofx_glmotion_get_velocity(struct glmotion *motion)
{
	return motion->velocity;
}

static inline GLuint nemofx_glmotion_get_texture(struct glmotion *motion)
{
	return motion->texture[0];
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
