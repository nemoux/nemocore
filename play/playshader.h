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
	int use_pbo;

	int flip;

	int32_t texture_width, texture_height;
};

extern struct playshader *nemoplay_shader_create(void);
extern void nemoplay_shader_destroy(struct playshader *shader);

extern int nemoplay_shader_use_pbo(struct playshader *shader);

extern int nemoplay_shader_set_format(struct playshader *shader, int format);
extern int nemoplay_shader_set_texture(struct playshader *shader, int32_t width, int32_t height);
extern int nemoplay_shader_set_viewport(struct playshader *shader, uint32_t texture, int32_t width, int32_t height);
extern int nemoplay_shader_set_flip(struct playshader *shader, int flip);

extern int nemoplay_shader_update(struct playshader *shader, struct playone *one);
extern int nemoplay_shader_dispatch(struct playshader *shader);

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

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
