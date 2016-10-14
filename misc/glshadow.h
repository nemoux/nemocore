#ifndef	__GL_SHADOW_H__
#define	__GL_SHADOW_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#define GLSHADOW_POINTLIGHTS_MAX				(16)

struct glshadow {
	GLuint texture;
	GLuint fbo, dbo;

	GLuint occluder;
	GLuint ofbo, odbo;
	GLuint shadow;
	GLuint sfbo, sdbo;

	GLuint program0;
	GLuint utexture0;

	GLuint program1;
	GLuint utexture1;
	GLuint uprojection1;

	GLuint program2;
	GLuint utexture2;
	GLuint uwidth2;
	GLuint uheight2;

	GLuint program3;
	GLuint ushadow3;
	GLuint uprojection3;
	GLuint uwidth3;
	GLuint uheight3;
	GLuint ucolor3;
	GLuint usize3;

	int32_t width, height;
	int32_t lightscope;

	struct {
		float position[3];
		float color[3];
		float size;
	} pointlights[GLSHADOW_POINTLIGHTS_MAX];
};

extern struct glshadow *glshadow_create(int32_t width, int32_t height, int32_t lightscope);
extern void glshadow_destroy(struct glshadow *shadow);

extern void glshadow_set_pointlight_position(struct glshadow *shadow, int index, float x, float y);
extern void glshadow_set_pointlight_color(struct glshadow *shadow, int index, float r, float g, float b);
extern void glshadow_set_pointlight_size(struct glshadow *shadow, int index, float size);
extern void glshadow_clear_pointlights(struct glshadow *shadow);

extern void glshadow_resize(struct glshadow *shadow, int32_t width, int32_t height);
extern void glshadow_dispatch(struct glshadow *shadow, GLuint texture);

static inline int32_t glshadow_get_width(struct glshadow *shadow)
{
	return shadow->width;
}

static inline int32_t glshadow_get_height(struct glshadow *shadow)
{
	return shadow->height;
}

static inline GLuint glshadow_get_texture(struct glshadow *shadow)
{
	return shadow->texture;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
