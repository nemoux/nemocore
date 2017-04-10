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

int nemotool_connect_egl(struct nemotool *tool, int alpha, int samples)
{
	static EGLint attribs[] = {
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
	static EGLint client_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 3,
		EGL_NONE
	};

	struct eglcontext *econtext;
	EGLConfig configs[16];
	EGLint major, minor;
	EGLint count, n, i, s;
	int buffer_size = 32;

	tool->eglcontext = econtext = (struct eglcontext *)malloc(sizeof(struct eglcontext));
	if (econtext == NULL)
		return -1;
	memset(econtext, 0, sizeof(struct eglcontext));

	econtext->display = eglGetDisplay((EGLNativeDisplayType)tool->display);
	if (econtext->display == EGL_NO_DISPLAY)
		goto err1;

	if (!eglInitialize(econtext->display, &major, &minor))
		goto err1;

	if (!eglBindAPI(EGL_OPENGL_ES_API))
		goto err1;

	if (!eglGetConfigs(econtext->display, NULL, 0, &count) || count < 1)
		goto err1;

	attribs[9] = alpha;
	attribs[15] = samples;

	if (!eglChooseConfig(econtext->display, attribs, configs, count, &n))
		goto err1;

	for (i = 0; i < n; i++) {
		eglGetConfigAttrib(econtext->display, configs[i], EGL_BUFFER_SIZE, &s);

		if (s == buffer_size) {
			econtext->config = configs[i];
			break;
		}
	}

	econtext->context = eglCreateContext(econtext->display, econtext->config, EGL_NO_CONTEXT, client_attribs);
	if (econtext->context == NULL)
		goto err1;

	return 0;

err1:
	free(econtext);

	return -1;
}

void nemotool_disconnect_egl(struct nemotool *tool)
{
	struct eglcontext *econtext = (struct eglcontext *)NEMOTOOL_EGLCONTEXT(tool);

	eglDestroyContext(econtext->display, econtext->context);

	free(econtext);

	tool->eglcontext = NULL;
}

struct nemocanvas *nemocanvas_egl_create(struct nemotool *tool, int32_t width, int32_t height)
{
	struct eglcontext *econtext = (struct eglcontext *)NEMOTOOL_EGLCONTEXT(tool);
	struct eglcanvas *ecanvas;
	struct nemocanvas *canvas;

	ecanvas = (struct eglcanvas *)malloc(sizeof(struct eglcanvas));
	if (ecanvas == NULL)
		return NULL;
	memset(ecanvas, 0, sizeof(struct eglcanvas));

	canvas = &ecanvas->base;

	if (nemocanvas_init(canvas, tool) < 0)
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

void nemocanvas_egl_destroy(struct nemocanvas *canvas)
{
	struct eglcanvas *ecanvas = NTEGL_CANVAS(canvas);

	wl_egl_window_destroy(ecanvas->window);

	nemocanvas_exit(canvas);

	free(ecanvas);
}

void nemocanvas_egl_resize(struct nemocanvas *canvas, int32_t width, int32_t height)
{
	struct eglcanvas *ecanvas = NTEGL_CANVAS(canvas);

	nemocanvas_set_size(canvas, width, height);

	wl_egl_window_resize(ecanvas->window, width, height, 0, 0);
}
