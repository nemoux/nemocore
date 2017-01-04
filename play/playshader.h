#ifndef __NEMOPLAY_SHADER_H__
#define __NEMOPLAY_SHADER_H__

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
	NEMOPLAY_SHADER_NORMAL_POLYGON = 0,
	NEMOPLAY_SHADER_FLIP_POLYGON = 1,
	NEMOPLAY_SHADER_ROTATE_POLYGON = 2,
	NEMOPLAY_SHADER_FLIP_ROTATE_POLYGON = 3,
	NEMOPLAY_SHADER_LAST_POLYGON
} NemoPlayShaderPolygon;

struct playone;
struct playshader;

typedef int (*nemoplay_shader_resize_t)(struct playshader *shader, int32_t width, int32_t height);
typedef int (*nemoplay_shader_update_t)(struct playshader *shader, struct playone *one);
typedef int (*nemoplay_shader_dispatch_t)(struct playshader *shader);

struct playshader {
	GLuint texture;
	GLuint fbo, dbo;
	int32_t viewport_width, viewport_height;

	int format;
	int polygon;

	int use_pbo;

	GLuint shaders[2];
	GLuint program;

	GLuint utex;
	GLuint utexy;
	GLuint utexu;
	GLuint utexv;

	GLuint tex;
	GLuint texy;
	GLuint texu;
	GLuint texv;

	GLuint pbo;
	GLuint pboy;
	GLuint pbou;
	GLuint pbov;

	int32_t texture_width, texture_height;

	nemoplay_shader_resize_t resize;
	nemoplay_shader_update_t update;
	nemoplay_shader_dispatch_t dispatch;
};

extern struct playshader *nemoplay_shader_create(void);
extern void nemoplay_shader_destroy(struct playshader *shader);

extern int nemoplay_shader_set_format(struct playshader *shader, int format);
extern int nemoplay_shader_set_viewport(struct playshader *shader, uint32_t texture, int32_t width, int32_t height);
extern int nemoplay_shader_set_polygon(struct playshader *shader, int polygon);
extern int nemoplay_shader_set_pbo(struct playshader *shader, int use_pbo);

static inline uint32_t nemoplay_shader_get_viewport(struct playshader *shader)
{
	return shader->texture;
}

static inline int32_t nemoplay_shader_get_viewport_width(struct playshader *shader)
{
	return shader->viewport_width;
}

static inline int32_t nemoplay_shader_get_viewport_height(struct playshader *shader)
{
	return shader->viewport_height;
}

static inline int32_t nemoplay_shader_get_texture_width(struct playshader *shader)
{
	return shader->texture_width;
}

static inline int32_t nemoplay_shader_get_texture_height(struct playshader *shader)
{
	return shader->texture_height;
}

static inline int nemoplay_shader_resize(struct playshader *shader, int32_t width, int32_t height)
{
	shader->texture_width = width;
	shader->texture_height = height;

	return shader->resize(shader, width, height);
}

static inline int nemoplay_shader_update(struct playshader *shader, struct playone *one)
{
	return shader->update(shader, one);
}

static inline int nemoplay_shader_dispatch(struct playshader *shader)
{
	return shader->dispatch(shader);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
