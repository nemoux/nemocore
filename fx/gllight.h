#ifndef	__NEMOFX_GL_LIGHT_H__
#define	__NEMOFX_GL_LIGHT_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#define GLLIGHT_POINTLIGHTS_MAX				(16)

struct gllight {
	GLuint texture;
	GLuint fbo, dbo;

	GLuint program0;
	GLint udiffuse0;
	GLint uambient0;

	GLuint program1;
	GLint udiffuse1;
	GLint uposition1;
	GLint ucolor1;
	GLint usize1;
	GLint uscope1;
	GLint utime1;

	int32_t width, height;

	struct {
		float color[3];
	} ambientlight;

	struct {
		float position[3];
		float color[3];
		float size;
		float scope;
	} pointlights[GLLIGHT_POINTLIGHTS_MAX];
};

extern struct gllight *nemofx_gllight_create(int32_t width, int32_t height);
extern void nemofx_gllight_destroy(struct gllight *light);

extern void nemofx_gllight_set_ambientlight_color(struct gllight *light, float r, float g, float b);

extern void nemofx_gllight_set_pointlight_position(struct gllight *light, int index, float x, float y);
extern void nemofx_gllight_set_pointlight_color(struct gllight *light, int index, float r, float g, float b);
extern void nemofx_gllight_set_pointlight_size(struct gllight *light, int index, float size);
extern void nemofx_gllight_set_pointlight_scope(struct gllight *light, int index, float scope);
extern void nemofx_gllight_clear_pointlights(struct gllight *light);

extern void nemofx_gllight_resize(struct gllight *light, int32_t width, int32_t height);
extern void nemofx_gllight_dispatch(struct gllight *light, GLuint texture);

static inline int32_t nemofx_gllight_get_width(struct gllight *light)
{
	return light->width;
}

static inline int32_t nemofx_gllight_get_height(struct gllight *light)
{
	return light->height;
}

static inline GLuint nemofx_gllight_get_texture(struct gllight *light)
{
	return light->texture;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
