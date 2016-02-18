#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <skiaconfig.hpp>

#include <nemotool.h>
#include <nemocanvas.h>
#include <nemoegl.h>
#include <showhelper.h>
#include <showitem.hpp>
#include <nemomisc.h>

#define NEMOIMAGE_SIZE_MAX			(1024)

struct imagecontext {
	struct nemotool *tool;

	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;
	struct showone *canvas;
	struct showone *image;
};

static void nemoimage_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, void *event)
{
	if (nemoshow_event_is_down(show, event) || nemoshow_event_is_up(show, event)) {
		nemoshow_event_update_taps(show, canvas, event);

		if (nemoshow_event_is_single_tap(show, event)) {
			nemoshow_view_move(show, nemoshow_event_get_serial_on(event, 0));
		} else if (nemoshow_event_is_many_taps(show, event)) {
			nemoshow_view_pick_distant(show, event, NEMOSHOW_VIEW_PICK_ALL_TYPE);
		}
	}
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "file",		required_argument,	NULL,		'f' },
		{ 0 }
	};

	struct imagecontext *context;
	struct nemotool *tool;
	struct nemotale *tale;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct showone *one;
	char *filepath = NULL;
	int32_t width, height;
	int opt;

	SkBitmap *bitmap;
	bool r;

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

	context->tool = tool = nemotool_create();
	if (tool == NULL)
		goto out1;
	nemotool_connect_wayland(tool, NULL);

	bitmap = new SkBitmap;

	r = SkImageDecoder::DecodeFile(filepath, bitmap);
	if (r == false)
		goto out2;

	width = bitmap->width();
	height = bitmap->height();

	if (width > NEMOIMAGE_SIZE_MAX) {
		height = height * ((double)NEMOIMAGE_SIZE_MAX / (double)width);
		width = NEMOIMAGE_SIZE_MAX;
	}
	if (height > NEMOIMAGE_SIZE_MAX) {
		width = width * ((double)NEMOIMAGE_SIZE_MAX / (double)height);
		height = NEMOIMAGE_SIZE_MAX;
	}

	context->show = show = nemoshow_create_view(tool, width, height);
	if (show == NULL)
		goto out2;
	nemoshow_set_userdata(show, context);

	nemoshow_view_set_input(show, "touch");

	context->scene = scene = nemoshow_scene_create();
	nemoshow_scene_set_width(scene, width);
	nemoshow_scene_set_height(scene, height);
	nemoshow_attach_one(show, scene);

	context->back = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_BACK_TYPE);
	nemoshow_canvas_set_fill_color(canvas, 0.0f, 0.0f, 0.0f, 0.0f);
	nemoshow_canvas_set_alpha(canvas, 0.0f);
	nemoshow_attach_one(show, canvas);
	nemoshow_one_attach(scene, canvas);

	context->canvas = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
	nemoshow_canvas_set_dispatch_event(canvas, nemoimage_dispatch_canvas_event);
	nemoshow_attach_one(show, canvas);
	nemoshow_one_attach(scene, canvas);

	context->image = one = nemoshow_item_create(NEMOSHOW_IMAGE_ITEM);
	nemoshow_attach_one(show, one);
	nemoshow_one_attach(canvas, one);
	nemoshow_item_set_x(one, 0.0f);
	nemoshow_item_set_y(one, 0.0f);
	nemoshow_item_set_width(one, width);
	nemoshow_item_set_height(one, height);
	nemoshow_item_set_bitmap(one, bitmap);

	nemoshow_dispatch_frame(show);

	nemotool_run(tool);

	nemoshow_destroy_view(show);

out2:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

out1:
	free(context);

	return 0;
}
