#ifndef	__GL_LIGHT_H__
#define	__GL_LIGHT_H__

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
	GLuint udiffuse0;
	GLuint uambient0;

	GLuint program1;
	GLuint udiffuse1;
	GLuint uposition1;
	GLuint ucolor1;
	GLuint usize1;
	GLuint uscope1;
	GLuint utime1;

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

extern struct gllight *gllight_create(int32_t width, int32_t height);
extern void gllight_destroy(struct gllight *light);

extern void gllight_set_ambientlight_color(struct gllight *light, float r, float g, float b);

extern void gllight_set_pointlight_position(struct gllight *light, int index, float x, float y);
extern void gllight_set_pointlight_color(struct gllight *light, int index, float r, float g, float b);
extern void gllight_set_pointlight_size(struct gllight *light, int index, float size);
extern void gllight_set_pointlight_scope(struct gllight *light, int index, float scope);

extern void gllight_resize(struct gllight *light, int32_t width, int32_t height);
extern void gllight_dispatch(struct gllight *light, GLuint texture);

static inline int32_t gllight_get_width(struct gllight *light)
{
	return light->width;
}

static inline int32_t gllight_get_height(struct gllight *light)
{
	return light->height;
}

static inline GLuint gllight_get_texture(struct gllight *light)
{
	return light->texture;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
