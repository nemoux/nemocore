#ifndef	__EGL_HELPER_H__
#define	__EGL_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

extern int egl_prepare_context(EGLNativeDisplayType nativedisplay, EGLDisplay *egl_display, EGLContext *egl_context, EGLConfig *egl_config, int buffer_size, int use_alpha);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
