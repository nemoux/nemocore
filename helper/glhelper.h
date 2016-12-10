#ifndef	__GL_HELPER_H__
#define	__GL_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

extern GLuint gl_compile_shader(GLenum type, int count, const char **sources);
extern GLuint gl_compile_program(const char *vertex_source, const char *fragment_source, GLuint *vertex_shader, GLuint *fragment_shader);

extern GLuint gl_create_texture(GLint filter, GLint wrap, GLuint width, GLuint height);
extern int gl_load_texture(GLuint texture, GLuint width, GLuint height, const char *filepath);

extern int gl_create_fbo(GLuint tex, GLuint width, GLuint height, GLuint *fbo, GLuint *dbo);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
