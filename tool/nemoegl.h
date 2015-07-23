#ifndef	__NEMOTOOL_EGL_H__
#define	__NEMOTOOL_EGL_H__

#include <GLES2/gl2.h>
#include <EGL/egl.h>

#include <nemotool.h>
#include <nemocanvas.h>
#include <nemolist.h>
#include <nemolistener.h>

struct eglcontext {
	struct nemotool *tool;

	EGLDisplay egl_display;
	EGLContext egl_context;
	EGLConfig egl_config;
};

#define	NTEGL_DISPLAY(egl)		(egl->egl_display)
#define	NTEGL_CONTEXT(egl)		(egl->egl_context)
#define	NTEGL_CONFIG(egl)			(egl->egl_config)

struct eglcanvas {
	struct eglcontext *egl;

	struct nemocanvas *canvas;

	struct wl_egl_window *egl_window;
};

#define	NTEGL_CANVAS(ec)		(ec->canvas)
#define	NTEGL_WINDOW(ec)		(ec->egl_window)

extern struct eglcontext *nemotool_create_egl(struct nemotool *tool);
extern void nemotool_destroy_egl(struct eglcontext *egl);

extern struct eglcanvas *nemotool_create_egl_canvas(struct eglcontext *egl, int32_t width, int32_t height);
extern void nemotool_destroy_egl_canvas(struct eglcanvas *canvas);

extern void nemotool_resize_egl_canvas(struct eglcanvas *canvas, int32_t width, int32_t height);

#endif
