#ifndef __NEMOPLAY_SHADER_H__
#define __NEMOPLAY_SHADER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

struct playshader {
	GLuint texture;
	GLuint fbo, dbo;
	int32_t viewport_width, viewport_height;

	GLuint shaders[2];
	GLuint program;

	GLuint utexy;
	GLuint utexu;
	GLuint utexv;

	GLuint texy;
	GLuint texu;
	GLuint texv;
	int32_t texture_width, texture_height;
	int32_t texture_linesize;
};

extern struct playshader *nemoplay_shader_create(void);
extern void nemoplay_shader_destroy(struct playshader *shader);

extern int nemoplay_shader_set_texture(struct playshader *shader, int32_t width, int32_t height);
extern int nemoplay_shader_set_texture_linesize(struct playshader *shader, int32_t linesize);
extern int nemoplay_shader_set_viewport(struct playshader *shader, GLuint texture, int32_t width, int32_t height);

extern int nemoplay_shader_prepare(struct playshader *shader, const char *vertex_source, const char *fragment_source);
extern void nemoplay_shader_finish(struct playshader *shader);
extern int nemoplay_shader_update(struct playshader *shader, uint8_t *y, uint8_t *u, uint8_t *v);
extern int nemoplay_shader_clear(struct playshader *shader);
extern int nemoplay_shader_dispatch(struct playshader *shader);

static const char NEMOPLAY_TO_RGBA_VERTEX_SHADER[] =
"attribute vec2 position;\n"
"attribute vec2 texcoord;\n"
"varying vec2 vtexcoord;\n"
"void main()\n"
"{\n"
"  gl_Position = vec4(position, 0.0, 1.0);\n"
"  vtexcoord = texcoord;\n"
"}\n";

static const char NEMOPLAY_TO_RGBA_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D texy;\n"
"uniform sampler2D texu;\n"
"uniform sampler2D texv;\n"
"void main()\n"
"{\n"
"  float y = texture2D(texy, vtexcoord).r;\n"
"  float u = texture2D(texu, vtexcoord).r - 0.5;\n"
"  float v = texture2D(texv, vtexcoord).r - 0.5;\n"
"  float r = y + 1.402 * v;\n"
"  float g = y - 0.344 * u - 0.714 * v;\n"
"  float b = y + 1.772 * u;\n"
"  gl_FragColor = vec4(r, g, b, 1.0);\n"
"}\n";

static inline int nemoplay_shader_get_texture_width(struct playshader *shader)
{
	return shader->texture_width;
}

static inline int nemoplay_shader_get_texture_height(struct playshader *shader)
{
	return shader->texture_height;
}

static inline int nemoplay_shader_get_texture_linesize(struct playshader *shader)
{
	return shader->texture_linesize;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
