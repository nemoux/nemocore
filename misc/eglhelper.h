#ifndef	__EGL_HELPER_H__
#define	__EGL_HELPER_H__

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <EGL/egl.h>

extern int egl_choose_config(EGLDisplay egl_display, const EGLint *attribs, const EGLint *visualid, EGLConfig *config);
extern int egl_prepare_context(EGLNativeDisplayType nativedisplay, EGLDisplay *egl_display, EGLContext *egl_context, EGLConfig *egl_config, int use_alpha, const EGLint *visualid);

#endif
