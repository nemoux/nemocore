#ifndef	__FBO_HELPER_H__
#define	__FBO_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <GLES2/gl2.h>

extern int fbo_prepare_context(GLuint tex, int32_t width, int32_t height, GLuint *fbo, GLuint *dbo);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
