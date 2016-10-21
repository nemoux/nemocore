#ifndef	__GL_SHADER_H__
#define	__GL_SHADER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

extern GLuint glshader_compile(GLenum type, int count, const char **sources);
extern GLuint glshader_compile_program(const char *vertex_source, const char *fragment_source, GLuint *vertex_shader, GLuint *fragment_shader);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
