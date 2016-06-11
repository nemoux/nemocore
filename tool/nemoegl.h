#ifndef	__NEMOTOOL_EGL_H__
#define	__NEMOTOOL_EGL_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

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

extern struct eglcontext *nemoegl_create(struct nemotool *tool);
extern void nemoegl_destroy(struct eglcontext *egl);

extern struct eglcanvas *nemoegl_create_canvas(struct eglcontext *egl, int32_t width, int32_t height);
extern void nemoegl_destroy_canvas(struct eglcanvas *canvas);

extern void nemoegl_resize_canvas(struct eglcanvas *canvas, int32_t width, int32_t height);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
