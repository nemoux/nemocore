#ifndef __NEMOCOOK_EGL_H__
#define __NEMOCOOK_EGL_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

struct nemocook;

extern int nemocook_prepare_fbo(struct nemocook *cook, GLuint texture, GLuint width, GLuint height);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
