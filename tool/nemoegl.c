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

struct eglcanvas *nemoegl_create_canvas(struct eglcontext *egl, int32_t width, int32_t height)
{
	struct eglcanvas *canvas;

	canvas = (struct eglcanvas *)malloc(sizeof(struct eglcanvas));
	if (canvas == NULL)
		return NULL;
	memset(canvas, 0, sizeof(struct eglcanvas));

	canvas->canvas = nemocanvas_create(egl->tool);
	if (canvas->canvas == NULL)
		goto err1;

	canvas->window = wl_egl_window_create(
			canvas->canvas->surface,
			width, height);
	if (canvas->window == NULL)
		goto err2;

	return canvas;

err2:
	nemocanvas_destroy(canvas->canvas);

err1:
	free(canvas);

	return NULL;
}

void nemoegl_destroy_canvas(struct eglcanvas *canvas)
{
	wl_egl_window_destroy(canvas->window);

	nemocanvas_destroy(canvas->canvas);

	free(canvas);
}

void nemoegl_resize_canvas(struct eglcanvas *canvas, int32_t width, int32_t height)
{
	nemocanvas_set_size(canvas->canvas, width, height);

	wl_egl_window_resize(canvas->window, width, height, 0, 0);
}
