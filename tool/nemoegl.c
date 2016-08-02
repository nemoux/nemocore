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
#include <eglhelper.h>
#include <nemomisc.h>

struct eglcontext *nemoegl_create(struct nemotool *tool)
{
	struct eglcontext *egl;

	egl = (struct eglcontext *)malloc(sizeof(struct eglcontext));
	if (egl == NULL)
		return NULL;
	memset(egl, 0, sizeof(struct eglcontext));

	egl->tool = tool;

	egl_prepare_context(
			(EGLNativeDisplayType)tool->display,
			&egl->egl_display,
			&egl->egl_context,
			&egl->egl_config,
			32, 1);

	return egl;
}

void nemoegl_destroy(struct eglcontext *egl)
{
	eglDestroyContext(egl->egl_display, egl->egl_context);

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

	canvas->egl_window = wl_egl_window_create(
			canvas->canvas->surface,
			width, height);
	if (canvas->egl_window == NULL)
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
	wl_egl_window_destroy(canvas->egl_window);

	nemocanvas_destroy(canvas->canvas);

	free(canvas);
}

void nemoegl_resize_canvas(struct eglcanvas *canvas, int32_t width, int32_t height)
{
	nemocanvas_set_size(canvas->canvas, width, height);

	wl_egl_window_resize(canvas->egl_window, width, height, 0, 0);
}
