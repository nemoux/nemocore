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

#include <EGL/egl.h>
#include <EGL/eglext.h>

struct nemocook;

extern int nemocook_prepare_egl(struct nemocook *cook, EGLDisplay egl_display, EGLContext egl_context, EGLConfig egl_config, EGLNativeWindowType egl_window);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
