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

struct playone;
struct playshader;

typedef int (*nemoplay_shader_prepare_t)(struct playshader *shader, int32_t width, int32_t height, int use_pbo);
typedef int (*nemoplay_shader_update_t)(struct playshader *shader, struct playone *one);
typedef int (*nemoplay_shader_dispatch_t)(struct playshader *shader);

struct playshader {
	GLuint texture;
	GLuint fbo, dbo;
	int32_t viewport_width, viewport_height;

	int format;

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
	int flip;

	nemoplay_shader_prepare_t prepare;
	nemoplay_shader_update_t update;
	nemoplay_shader_dispatch_t dispatch;
};

extern struct playshader *nemoplay_shader_create(void);
extern void nemoplay_shader_destroy(struct playshader *shader);

extern int nemoplay_shader_set_format(struct playshader *shader, int format);
extern int nemoplay_shader_set_viewport(struct playshader *shader, uint32_t texture, int32_t width, int32_t height);
extern int nemoplay_shader_set_flip(struct playshader *shader, int flip);

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

static inline int nemoplay_shader_prepare(struct playshader *shader, int32_t width, int32_t height, int use_pbo)
{
	shader->texture_width = width;
	shader->texture_height = height;

	return shader->prepare(shader, width, height, use_pbo);
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
