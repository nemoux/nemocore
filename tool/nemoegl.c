#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-egl.h>

#include <nemotool.h>
#include <nemocanvas.h>
#include <nemoegl.h>
#include <nemomisc.h>

struct eglcontext *nemoegl_create(struct nemotool *tool)
{
#ifdef NEMOUX_WITH_OPENGL_ES3
	static const EGLint opaque_attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 1,
		EGL_GREEN_SIZE, 1,
		EGL_BLUE_SIZE, 1,
		EGL_ALPHA_SIZE, 0,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
		EGL_SAMPLE_BUFFERS, 1,
		EGL_SAMPLES, 4,
		EGL_NONE
	};
	static const EGLint alpha_attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 1,
		EGL_GREEN_SIZE, 1,
		EGL_BLUE_SIZE, 1,
		EGL_ALPHA_SIZE, 1,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
		EGL_SAMPLE_BUFFERS, 1,
		EGL_SAMPLES, 4,
		EGL_NONE
	};
	static const EGLint client_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 3,
		EGL_NONE
	};
#else
	static const EGLint opaque_attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 1,
		EGL_GREEN_SIZE, 1,
		EGL_BLUE_SIZE, 1,
		EGL_ALPHA_SIZE, 0,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_SAMPLE_BUFFERS, 1,
		EGL_SAMPLES, 4,
		EGL_NONE
	};
	static const EGLint alpha_attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 1,
		EGL_GREEN_SIZE, 1,
		EGL_BLUE_SIZE, 1,
		EGL_ALPHA_SIZE, 1,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_SAMPLE_BUFFERS, 1,
		EGL_SAMPLES, 4,
		EGL_NONE
	};
	static const EGLint client_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
#endif

	struct eglcontext *egl;
	EGLConfig configs[16];
	EGLint major, minor;
	EGLint count, n, i, s;
	int buffer_size = 32;
	int use_alpha = 1;

	egl = (struct eglcontext *)malloc(sizeof(struct eglcontext));
	if (egl == NULL)
		return NULL;
	memset(egl, 0, sizeof(struct eglcontext));

	egl->tool = tool;

	egl->display = eglGetDisplay((EGLNativeDisplayType)tool->display);
	if (egl->display == EGL_NO_DISPLAY)
		goto err1;

	if (!eglInitialize(egl->display, &major, &minor))
		goto err1;

	if (!eglBindAPI(EGL_OPENGL_ES_API))
		goto err1;

	if (!eglGetConfigs(egl->display, NULL, 0, &count) || count < 1)
		goto err1;

	if (!eglChooseConfig(egl->display, use_alpha == 0 ? opaque_attribs : alpha_attribs, configs, count, &n))
		goto err1;

	for (i = 0; i < n; i++) {
		eglGetConfigAttrib(egl->display, configs[i], EGL_BUFFER_SIZE, &s);

		if (s == buffer_size) {
			egl->config = configs[i];
			break;
		}
	}

	egl->context = eglCreateContext(egl->display, egl->config, EGL_NO_CONTEXT, client_attribs);
	if (egl->context == NULL)
		goto err1;

	return egl;

err1:
	free(egl);

	return NULL;
}

void nemoegl_destroy(struct eglcontext *egl)
{
	eglDestroyContext(egl->display, egl->context);

	free(egl);
}

struct nemocanvas *nemoegl_create_canvas(struct eglcontext *egl, int32_t width, int32_t height)
{
	struct eglcanvas *ecanvas;
	struct nemocanvas *canvas;

	ecanvas = (struct eglcanvas *)malloc(sizeof(struct eglcanvas));
	if (ecanvas == NULL)
		return NULL;
	memset(ecanvas, 0, sizeof(struct eglcanvas));

	canvas = &ecanvas->base;

	if (nemocanvas_init(canvas, egl->tool) < 0)
		goto err1;

	ecanvas->window = wl_egl_window_create(
			canvas->surface,
			width, height);
	if (ecanvas->window == NULL)
		goto err2;

	return canvas;

err2:
	nemocanvas_exit(canvas);

err1:
	free(ecanvas);

	return NULL;
}

void nemoegl_destroy_canvas(struct nemocanvas *canvas)
{
	struct eglcanvas *ecanvas = NTEGL_CANVAS(canvas);

	wl_egl_window_destroy(ecanvas->window);

	nemocanvas_exit(canvas);

	free(ecanvas);
}

void nemoegl_resize_canvas(struct nemocanvas *canvas, int32_t width, int32_t height)
{
	struct eglcanvas *ecanvas = NTEGL_CANVAS(canvas);

	nemocanvas_set_size(canvas, width, height);

	wl_egl_window_resize(ecanvas->window, width, height, 0, 0);
}
