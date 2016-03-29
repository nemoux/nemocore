#ifndef __NEMOPLAY_SHADER_H__
#define __NEMOPLAY_SHADER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

struct playshader {
	GLuint texture;
	GLuint fbo, dbo;

	GLuint program;

	GLuint utexy;
	GLuint utexu;
	GLuint utexv;

	GLuint texy;
	GLuint texu;
	GLuint texv;

	int32_t width, height;
};

extern struct playshader *nemoplay_shader_create(void);
extern void nemoplay_shader_destroy(struct playshader *shader);

extern int nemoplay_shader_set_texture(struct playshader *shader, GLuint texture, int32_t width, int32_t height);

extern int nemoplay_shader_dispatch(struct playshader *shader, uint8_t *y, uint8_t *u, uint8_t *v);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
