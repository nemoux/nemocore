#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemotool.h>
#include <nemocanvas.h>
#include <nemoegl.h>
#include <nemoshow.h>
#include <talehelper.h>
#include <pixmanhelper.h>
#include <nemomisc.h>

struct showcontext {
	struct eglcanvas *canvas;

	struct nemotale *tale;
	struct nemoshow *show;
	struct showone *scene;
};

static void nemoshow_dispatch_tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
	struct showcontext *context = (struct showcontext *)nemotale_get_userdata(tale);
	struct nemocanvas *canvas = NTEGL_CANVAS(context->canvas);
	uint32_t id = nemotale_node_get_id(node);

	if (id == 1) {
		if (type & NEMOTALE_DOWN_EVENT) {
			struct taletap *taps[16];
			int ntaps;

			ntaps = nemotale_get_node_taps(tale, node, taps, type);
			if (ntaps == 1) {
				nemocanvas_move(canvas, taps[0]->serial);
			} else if (ntaps == 2) {
				nemocanvas_pick(canvas,
						taps[0]->serial,
						taps[1]->serial,
						(1 << NEMO_SURFACE_PICK_TYPE_ROTATE) | (1 << NEMO_SURFACE_PICK_TYPE_SCALE));
			}
		} else if (nemotale_is_single_click(tale, event, type) != 0) {
			struct taletap *taps[16];
			int ntaps;

			ntaps = nemotale_get_node_taps(tale, node, taps, type);
			if (ntaps == 1) {
				struct nemoshow *show = context->show;
				struct showtransition *trans;

				trans = nemoshow_transition_create(
						nemoshow_search_one(show, "ease0"),
						800, 100);
				nemoshow_transition_attach_sequence(trans,
						nemoshow_search_one(show, "hour-hand-sequence"));
				nemoshow_attach_transition(show, trans);

				nemocanvas_dispatch_frame(canvas);
			} else if (ntaps == 2) {
			} else if (ntaps == 3) {
				struct nemotool *tool = nemocanvas_get_tool(canvas);

				nemotool_exit(tool);
			}
		}
	}
}

static void nemoshow_dispatch_canvas_resize(struct nemocanvas *canvas, int32_t width, int32_t height)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct showcontext *context = (struct showcontext *)nemotale_get_userdata(tale);

	nemotool_resize_egl_canvas(context->canvas, width, height);

	nemoshow_set_size(context->show, width, height);

	nemoshow_render_one(context->show);

	nemotale_composite_egl_full(tale);
}

static void nemoshow_dispatch_canvas_frame(struct nemocanvas *canvas, uint64_t secs, uint32_t nsecs)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct showcontext *context = (struct showcontext *)nemotale_get_userdata(tale);
	struct nemoshow *show = context->show;

	if (secs == 0 && nsecs == 0) {
		nemocanvas_feedback(canvas);
	} else if (nemoshow_has_transition(show) != 0) {
		nemoshow_dispatch_transition(show, secs * 1000 + nsecs / 1000000);

		nemocanvas_feedback(canvas);

		nemoshow_render_one(show);
	}

	nemotale_composite_egl(tale, NULL);
}

int main(int argc, char *argv[])
{
	struct showcontext *context;
	struct nemotool *tool;
	struct eglcontext *egl;
	struct eglcanvas *canvas;
	struct nemotale *tale;
	struct nemoshow *show;
	struct showone *scene;
	int32_t width = 500, height = 500;

	context = (struct showcontext *)malloc(sizeof(struct showcontext));
	if (context == NULL)
		return -1;
	memset(context, 0, sizeof(struct showcontext));

	tool = nemotool_create();
	if (tool == NULL)
		return -1;
	nemotool_connect_wayland(tool, NULL);

	egl = nemotool_create_egl(tool);

	context->canvas = canvas = nemotool_create_egl_canvas(egl, width, height);
	nemocanvas_set_nemosurface(NTEGL_CANVAS(canvas), NEMO_SHELL_SURFACE_TYPE_NORMAL);
	nemocanvas_set_dispatch_resize(NTEGL_CANVAS(canvas), nemoshow_dispatch_canvas_resize);
	nemocanvas_set_dispatch_frame(NTEGL_CANVAS(canvas), nemoshow_dispatch_canvas_frame);

	context->tale = tale = nemotale_create_gl();
	nemotale_set_backend(tale,
			nemotale_create_egl(
				NTEGL_DISPLAY(egl),
				NTEGL_CONTEXT(egl),
				NTEGL_CONFIG(egl),
				(EGLNativeWindowType)NTEGL_WINDOW(canvas)));
	nemotale_attach_canvas(tale, NTEGL_CANVAS(canvas), nemoshow_dispatch_tale_event);
	nemotale_set_userdata(tale, context);

	nemoshow_initialize();

	context->show = show = nemoshow_create();
	nemoshow_set_tale(show, tale);
	nemoshow_set_userdata(show, context);

	nemoshow_load_xml(show, argv[1]);
	nemoshow_arrange_one(show);

	context->scene = scene = nemoshow_search_one(show, "scene0");
	nemoshow_set_scene(show, scene);

	nemoshow_set_camera(show,
			nemoshow_search_one(show, "camera0"));

	nemoshow_set_size(show, width, height);

	nemoshow_render_one(show);

	nemocanvas_dispatch_frame(NTEGL_CANVAS(canvas));

	nemotool_run(tool);

	nemoshow_destroy(show);

	nemoshow_finalize();

	nemotale_destroy_gl(tale);

	nemotool_destroy_egl_canvas(canvas);
	nemotool_destroy_egl(egl);

	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

	free(context);

	return 0;
}
