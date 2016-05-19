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
#include <pdfhelper.h>
#include <pixmanhelper.h>
#include <talehelper.h>
#include <nemomisc.h>

struct pdfcontext {
	struct nemotool *tool;
	struct eglcanvas *canvas;

	struct nemotale *tale;
	struct talenode *node;

	struct nemopdf *pdf;
};

static uint32_t nemopdf_get_touch_location(int32_t sx, int32_t sy, int32_t sw, int32_t sh, int32_t x, int32_t y)
{
	int32_t w = sw - sx;
	int32_t h = sh - sy;
	int32_t px = x - sx;
	int32_t py = y - sy;
	uint32_t type = 0x0;

	if (px < w / 2)
		type |= (1 << 0);
	else
		type |= (1 << 1);

	if (py < h / 2)
		type |= (1 << 2);
	else
		type |= (1 << 3);

	return type;
}

static void nemopdf_dispatch_tale_event(struct nemotale *tale, struct talenode *node, struct taleevent *event)
{
	struct pdfcontext *context = (struct pdfcontext *)nemotale_get_userdata(tale);
	struct nemocanvas *canvas = NTEGL_CANVAS(context->canvas);
	struct nemopdf *pdf = context->pdf;
	uint32_t id = nemotale_node_get_id(node);

	if (id == 1) {
		if (nemotale_event_is_touch_down(tale, event) || nemotale_event_is_touch_up(tale, event)) {
			nemotale_event_update_taps_by_node(tale, node, event);

			if (nemotale_event_is_single_tap(tale, event)) {
				nemocanvas_move(canvas, nemotale_event_get_serial_on(event, 0));
			} else if (nemotale_event_is_many_taps(tale, event)) {
				uint32_t serial0, serial1;

				nemotale_event_get_distant_tapserials(tale, event, &serial0, &serial1);

				nemocanvas_pick(canvas,
						serial0, serial1,
						(1 << NEMO_SURFACE_PICK_TYPE_ROTATE) | (1 << NEMO_SURFACE_PICK_TYPE_SCALE) | (1 << NEMO_SURFACE_PICK_TYPE_MOVE));
			}
		}

		if (nemotale_event_is_single_click(tale, event)) {
			nemotale_event_update_taps_by_node(tale, node, event);

			if (nemotale_event_is_no_tap(tale, event)) {
				uint32_t location;

				location = nemopdf_get_touch_location(0, 0,
						nemotale_node_get_width(node),
						nemotale_node_get_height(node),
						(int32_t)event->x, (int32_t)event->y);

				if (location & (1 << 0)) {
					if (nemopdf_get_page_index(pdf) - 1 >= 0) {
						nemopdf_render_page(pdf,
								nemopdf_get_page_index(pdf) - 1,
								nemotale_node_get_pixman(node));
						nemotale_node_damage_all(node);
						nemotale_composite_egl(tale, NULL);
					}
				} else if (location & (1 << 1)) {
					if (nemopdf_get_page_index(pdf) + 1 < nemopdf_get_page_count(pdf)) {
						nemopdf_render_page(pdf,
								nemopdf_get_page_index(pdf) + 1,
								nemotale_node_get_pixman(node));
						nemotale_node_damage_all(node);
						nemotale_composite_egl(tale, NULL);
					}
				}
			}
		}
	}
}

static void nemopdf_dispatch_canvas_resize(struct nemocanvas *canvas, int32_t width, int32_t height)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct pdfcontext *context = (struct pdfcontext *)nemotale_get_userdata(tale);
	struct nemopdf *pdf = context->pdf;
	struct talenode *node = context->node;

	if (width == 0 || height == 0)
		return;

	if (width < nemotale_get_close_width(tale) || height < nemotale_get_close_height(tale)) {
		nemotool_exit(context->tool);
		return;
	}

	nemotool_resize_egl_canvas(context->canvas, width, height);
	nemotale_resize(tale, width, height);
	nemotale_node_resize_pixman(node, width, height);

	nemopdf_render_page(pdf,
			nemopdf_get_page_index(pdf),
			nemotale_node_get_pixman(node));
	nemotale_node_damage_all(node);
	nemotale_composite_egl(tale, NULL);
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ 0 }
	};

	struct pdfcontext *context;
	struct nemopdf *pdf;
	struct nemotool *tool;
	struct eglcontext *egl;
	struct eglcanvas *canvas;
	struct nemotale *tale;
	struct talenode *node;
	char *filepath = NULL;
	int32_t width, height;
	int opt;

	while (opt = getopt_long(argc, argv, "", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			default:
				break;
		}
	}

	if (optind < argc)
		filepath = strdup(argv[optind]);

	if (filepath == NULL)
		return 0;

	context = (struct pdfcontext *)malloc(sizeof(struct pdfcontext));
	if (context == NULL)
		return -1;

	context->pdf = pdf = nemopdf_create_doc(filepath);
	if (pdf == NULL)
		goto out1;

	width = nemopdf_get_page_width(pdf);
	height = nemopdf_get_page_height(pdf);

	context->tool = tool = nemotool_create();
	if (tool == NULL)
		goto out2;
	nemotool_connect_wayland(tool, NULL);

	egl = nemotool_create_egl(tool);

	context->canvas = canvas = nemotool_create_egl_canvas(egl, width, height);
	nemocanvas_set_nemosurface(NTEGL_CANVAS(canvas), NEMO_SHELL_SURFACE_TYPE_NORMAL);
	nemocanvas_set_fullscreen_type(NTEGL_CANVAS(canvas), (1 << NEMO_SURFACE_FULLSCREEN_TYPE_PICK) | (1 << NEMO_SURFACE_FULLSCREEN_TYPE_PITCH));
	nemocanvas_set_anchor(NTEGL_CANVAS(canvas), -0.5f, -0.5f);
	nemocanvas_set_dispatch_resize(NTEGL_CANVAS(canvas), nemopdf_dispatch_canvas_resize);
	nemocanvas_set_max_size(NTEGL_CANVAS(canvas), UINT32_MAX, UINT32_MAX);
	nemocanvas_set_state(NTEGL_CANVAS(canvas), "layer");
	nemocanvas_set_state(NTEGL_CANVAS(canvas), "opaque");
	nemocanvas_put_state(NTEGL_CANVAS(canvas), "keypad");

	context->tale = tale = nemotale_create_gl();
	nemotale_set_backend(tale,
			nemotale_create_egl(
				NTEGL_DISPLAY(egl),
				NTEGL_CONTEXT(egl),
				NTEGL_CONFIG(egl),
				(EGLNativeWindowType)NTEGL_WINDOW(canvas)));
	nemotale_resize(tale, width, height);

	nemotale_attach_canvas(tale, NTEGL_CANVAS(canvas), nemopdf_dispatch_tale_event);
	nemotale_set_userdata(tale, context);

	context->node = node = nemotale_node_create_pixman(width, height);
	nemotale_node_set_id(node, 1);
	nemotale_attach_node(tale, node);

	nemopdf_render_page(pdf,
			0,
			nemotale_node_get_pixman(node));
	nemotale_composite_egl(tale, NULL);

	nemotool_run(tool);

	nemotale_destroy_gl(tale);

	nemotool_destroy_egl_canvas(canvas);
	nemotool_destroy_egl(egl);

	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

out2:
	nemopdf_destroy_doc(pdf);

out1:
	free(context);

	return 0;
}
