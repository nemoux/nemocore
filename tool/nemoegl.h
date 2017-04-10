#ifndef	__NEMOTOOL_EGL_H__
#define	__NEMOTOOL_EGL_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <nemotool.h>
#include <nemocanvas.h>
#include <nemolist.h>
#include <nemolistener.h>

struct eglcontext {
	EGLDisplay display;
	EGLContext context;
	EGLConfig config;
};

#define NEMOTOOL_EGLCONTEXT(tool)			((tool)->eglcontext)

#define	NTEGL_DISPLAY(tool)		(NEMOTOOL_EGLCONTEXT(tool)->display)
#define	NTEGL_CONTEXT(tool)		(NEMOTOOL_EGLCONTEXT(tool)->context)
#define	NTEGL_CONFIG(tool)		(NEMOTOOL_EGLCONTEXT(tool)->config)

struct eglcanvas {
	struct nemocanvas base;

	struct eglcontext *context;

	struct wl_egl_window *window;
};

#define NTEGL_CANVAS(canvas)		((struct eglcanvas *)container_of(canvas, struct eglcanvas, base))
#define NTEGL_WINDOW(canvas)		((EGLNativeWindowType)(NTEGL_CANVAS(canvas)->window))

extern int nemotool_connect_egl(struct nemotool *tool, int alpha, int samples);
extern void nemotool_disconnect_egl(struct nemotool *tool);

extern struct nemocanvas *nemocanvas_egl_create(struct nemotool *tool, int32_t width, int32_t height);
extern void nemocanvas_egl_destroy(struct nemocanvas *canvas);

extern void nemocanvas_egl_resize(struct nemocanvas *canvas, int32_t width, int32_t height);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
