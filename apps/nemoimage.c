#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <nemotool.h>
#include <nemocanvas.h>
#include <nemoegl.h>
#include <pixmanhelper.h>
#include <talehelper.h>
#include <nemomisc.h>

#define NEMOIMAGE_SIZE_MAX			(1024)

struct imagecontext {
	struct nemotool *tool;
	struct eglcanvas *canvas;

	struct nemotale *tale;
	struct talenode *node;

	int32_t screen_width, screen_height;
};

static void nemoimage_dispatch_tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
	struct imagecontext *context = (struct imagecontext *)nemotale_get_userdata(tale);
	struct nemocanvas *canvas = NTEGL_CANVAS(context->canvas);
	uint32_t id = nemotale_node_get_id(node);

	if (id == 1) {
		if (nemotale_is_touch_down(tale, event, type)) {
			nemotale_event_update_node_taps(tale, node, event, type);

			if (event->tapcount == 1) {
				nemocanvas_move(canvas, event->taps[0]->serial);
			} else if (event->tapcount == 2 && nemotale_tap_is_moving(tale, event->taps[0]) == 0) {
				nemocanvas_pick(canvas,
						event->taps[0]->serial,
						event->taps[1]->serial,
						(1 << NEMO_SURFACE_PICK_TYPE_ROTATE) | (1 << NEMO_SURFACE_PICK_TYPE_SCALE) | (1 << NEMO_SURFACE_PICK_TYPE_MOVE));
			}
		} else if (nemotale_is_touch_motion(tale, event, type)) {
			nemotale_event_update_node_taps(tale, node, event, type);

			if (event->tapcount == 1) {
			} else if (event->tapcount == 2) {
			}
		}
	}
}

static void nemoimage_dispatch_canvas_resize(struct nemocanvas *canvas, int32_t width, int32_t height)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct imagecontext *context = (struct imagecontext *)nemotale_get_userdata(tale);
	struct talenode *node = context->node;

	if (width == 0 || height == 0)
		return;

	if (width < nemotale_get_minimum_width(tale) || height < nemotale_get_minimum_height(tale)) {
		nemotool_exit(context->tool);
		return;
	}

	nemotool_resize_egl_canvas(context->canvas, width, height);
	nemotale_set_viewport(tale, width, height);

	nemotale_composite_egl(tale, NULL);
}

static void nemoimage_dispatch_canvas_screen(struct nemocanvas *canvas, int32_t x, int32_t y, int32_t width, int32_t height, int32_t mmwidth, int32_t mmheight, int left)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct imagecontext *context = (struct imagecontext *)nemotale_get_userdata(tale);

	context->screen_width = width;
	context->screen_height = height;
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "file",		required_argument,	NULL,		'f' },
		{ 0 }
	};

	struct imagecontext *context;
	struct nemotool *tool;
	struct eglcontext *egl;
	struct eglcanvas *canvas;
	struct nemotale *tale;
	struct talenode *node;
	pixman_image_t *piximg;
	char *filepath = NULL;
	int32_t width, height;
	int opt;

	while (opt = getopt_long(argc, argv, "f:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'f':
				filepath = strdup(optarg);
				break;

			default:
				break;
		}
	}

	if (filepath == NULL)
		return 0;

	context = (struct imagecontext *)malloc(sizeof(struct imagecontext));
	if (context == NULL)
		return -1;

	piximg = pixman_load_png_file(filepath);
	if (piximg == NULL) {
		piximg = pixman_load_jpeg_file(filepath);
		if (piximg == NULL) {
			goto out1;
		}
	}

	width = pixman_image_get_width(piximg);
	height = pixman_image_get_height(piximg);

	context->tool = tool = nemotool_create();
	if (tool == NULL)
		goto out2;
	nemotool_connect_wayland(tool, NULL);

	egl = nemotool_create_egl(tool);

	context->canvas = canvas = nemotool_create_egl_canvas(egl, width, height);
	nemocanvas_set_nemosurface(NTEGL_CANVAS(canvas), NEMO_SHELL_SURFACE_TYPE_NORMAL);
	nemocanvas_set_anchor(NTEGL_CANVAS(canvas), -0.5f, -0.5f);
	nemocanvas_set_dispatch_resize(NTEGL_CANVAS(canvas), nemoimage_dispatch_canvas_resize);
	nemocanvas_set_dispatch_screen(NTEGL_CANVAS(canvas), nemoimage_dispatch_canvas_screen);

	context->tale = tale = nemotale_create_gl();
	nemotale_set_backend(tale,
			nemotale_create_egl(
				NTEGL_DISPLAY(egl),
				NTEGL_CONTEXT(egl),
				NTEGL_CONFIG(egl),
				(EGLNativeWindowType)NTEGL_WINDOW(canvas)));
	nemotale_resize(tale, width, height);

	nemotale_attach_canvas(tale, NTEGL_CANVAS(canvas), nemoimage_dispatch_tale_event);
	nemotale_set_userdata(tale, context);

	context->node = node = nemotale_node_create_pixman(width, height);
	nemotale_node_set_id(node, 1);
	nemotale_attach_node(tale, node);

	pixman_image_composite32(PIXMAN_OP_SRC,
			piximg,
			NULL,
			nemotale_node_get_pixman(node),
			0, 0, 0, 0, 0, 0,
			width, height);

	if (width > NEMOIMAGE_SIZE_MAX) {
		nemotool_resize_egl_canvas(context->canvas, NEMOIMAGE_SIZE_MAX, NEMOIMAGE_SIZE_MAX * ((double)height / (double)width));

		nemotale_set_viewport(tale, NEMOIMAGE_SIZE_MAX, NEMOIMAGE_SIZE_MAX * ((double)height / (double)width));
	}

	nemotale_composite_egl(tale, NULL);

	nemotool_run(tool);

	nemotale_destroy_gl(tale);

	nemotool_destroy_egl_canvas(canvas);
	nemotool_destroy_egl(egl);

	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

out2:
	pixman_image_unref(piximg);

out1:
	free(context);

	return 0;
}
