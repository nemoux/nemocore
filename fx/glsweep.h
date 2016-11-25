#ifndef	__NEMOFX_GL_SWEEP_H__
#define	__NEMOFX_GL_SWEEP_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

typedef enum {
	NEMOFX_GLSWEEP_HORIZONTAL_TYPE = 0,
	NEMOFX_GLSWEEP_VERTICAL_TYPE = 1,
	NEMOFX_GLSWEEP_CIRCLE_TYPE = 2,
	NEMOFX_GLSWEEP_FAN_TYPE = 3,
	NEMOFX_GLSWEEP_MASK_TYPE = 4,
	NEMOFX_GLSWEEP_LAST_TYPE
} NemoFXGLSweepType;

struct glsweep {
	GLuint texture;
	GLuint fbo, dbo;

	GLuint snapshot;
	int is_reference;

	GLuint mask;

	GLuint program;

	GLuint utexture;
	GLuint uwidth, uheight;
	GLuint usnapshot;
	GLuint umask;
	GLuint utiming;
	GLuint upoint;

	int type;

	int32_t width, height;

	float t;
	float r;
	float point[2];
};

extern struct glsweep *nemofx_glsweep_create(int32_t width, int32_t height);
extern void nemofx_glsweep_destroy(struct glsweep *sweep);

extern void nemofx_glsweep_ref_snapshot(struct glsweep *sweep, GLuint texture, int32_t width, int32_t height);
extern void nemofx_glsweep_set_snapshot(struct glsweep *sweep, GLuint texture, int32_t width, int32_t height);
extern void nemofx_glsweep_put_snapshot(struct glsweep *sweep);

extern void nemofx_glsweep_set_timing(struct glsweep *sweep, float t);
extern void nemofx_glsweep_set_type(struct glsweep *sweep, int type);
extern void nemofx_glsweep_set_point(struct glsweep *sweep, float x, float y);
extern void nemofx_glsweep_set_mask(struct glsweep *sweep, GLuint mask);

extern void nemofx_glsweep_resize(struct glsweep *sweep, int32_t width, int32_t height);
extern void nemofx_glsweep_dispatch(struct glsweep *sweep, GLuint texture);

static inline GLuint nemofx_glsweep_get_texture(struct glsweep *sweep)
{
	return sweep->texture;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
