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
	struct nemotool *tool;

	EGLDisplay display;
	EGLContext context;
	EGLConfig config;
};

#define	NTEGL_DISPLAY(egl)		(egl->display)
#define	NTEGL_CONTEXT(egl)		(egl->context)
#define	NTEGL_CONFIG(egl)			(egl->config)

struct eglcanvas {
	struct eglcontext *egl;

	struct nemocanvas *canvas;

	struct wl_egl_window *window;
};

#define	NTEGL_CANVAS(ec)		(ec->canvas)
#define	NTEGL_WINDOW(ec)		(ec->window)

extern struct eglcontext *nemoegl_create(struct nemotool *tool);
extern void nemoegl_destroy(struct eglcontext *egl);

extern struct eglcanvas *nemoegl_create_canvas(struct eglcontext *egl, int32_t width, int32_t height);
extern void nemoegl_destroy_canvas(struct eglcanvas *canvas);

extern void nemoegl_resize_canvas(struct eglcanvas *canvas, int32_t width, int32_t height);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
