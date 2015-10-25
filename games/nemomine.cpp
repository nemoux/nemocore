#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <nemotool.h>
#include <showhelper.h>
#include <nemolog.h>
#include <nemomisc.h>

struct minecontext {
	struct nemotool *tool;

	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;
	struct showone *canvas;
	struct showone *inner, *outer, *solid;
	struct showone *ease;

	int columns;
	int rows;
	int bombs;
	int size;

	int width, height;
};

static void nemomine_dispatch_tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
	uint32_t id = nemotale_node_get_id(node);

	if (id == 1) {
		if (nemotale_dispatch_grab(tale, event->device, type, event) == 0) {
			struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);
			struct minecontext *context = (struct minecontext *)nemoshow_get_userdata(show);
			struct nemocanvas *canvas = NEMOSHOW_AT(show, canvas);

			if (nemotale_is_touch_down(tale, event, type)) {
				nemotale_event_update_node_taps(tale, node, event, type);

				if (nemotale_is_single_tap(tale, event, type)) {
					nemocanvas_move(canvas, event->taps[0]->serial);
				} else if (nemotale_is_double_taps(tale, event, type)) {
					nemocanvas_pick(canvas,
							event->taps[0]->serial,
							event->taps[1]->serial,
							(1 << NEMO_SURFACE_PICK_TYPE_ROTATE) | (1 << NEMO_SURFACE_PICK_TYPE_SCALE) | (1 << NEMO_SURFACE_PICK_TYPE_MOVE));

					nemotale_tap_set_state(event->taps[0], NEMOTALE_TAP_USED_STATE);
					nemotale_tap_set_state(event->taps[1], NEMOTALE_TAP_USED_STATE);
				}
			}
		}
	}
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "columns",		required_argument,	NULL,		'c' },
		{ "rows",				required_argument,	NULL,		'r' },
		{ "bombs",			required_argument,	NULL,		'b' },
		{ "size",				required_argument,	NULL,		's' },
		{ 0 }
	};
	struct minecontext *context;
	struct nemotool *tool;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct showone *blur;
	struct showone *ease;
	struct showone *one;
	int columns = 30;
	int rows = 16;
	int bombs = 99;
	int size = 10;
	int width, height;
	int opt;

	nemolog_set_file(2);

	while (opt = getopt_long(argc, argv, "c:r:b:s:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'c':
				columns = strtoul(optarg, NULL, 10);
				break;

			case 'r':
				rows = strtoul(optarg, NULL, 10);
				break;

			case 'b':
				bombs = strtoul(optarg, NULL, 10);
				break;

			case 's':
				size = strtoul(optarg, NULL, 10);
				break;

			default:
				break;
		}
	}

	context = (struct minecontext *)malloc(sizeof(struct minecontext));
	if (context == NULL)
		return -1;
	memset(context, 0, sizeof(struct minecontext));

	context->columns = columns;
	context->rows = rows;
	context->bombs = bombs;
	context->size = size;

	context->width = width = columns * size;
	context->height = height = rows * size;

	context->tool = tool = nemotool_create();
	if (tool == NULL)
		goto err1;
	nemotool_connect_wayland(tool, NULL);

	context->show = show = nemoshow_create_on_tale(tool, width, height, nemomine_dispatch_tale_event);
	if (show == NULL)
		goto err2;
	nemoshow_set_userdata(show, context);

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
	nemoshow_one_attach_one(scene, canvas);

	context->canvas = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
	nemoshow_canvas_set_event(canvas, 1);
	nemoshow_attach_one(show, canvas);
	nemoshow_one_attach_one(scene, canvas);

	nemoshow_set_scene(show, scene);
	nemoshow_set_size(show, width, height);

	context->ease = ease = nemoshow_ease_create();
	nemoshow_ease_set_type(ease, NEMOEASE_CUBIC_INOUT_TYPE);

	context->inner = blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_attach_one(show, blur);
	nemoshow_filter_set_blur(blur, "high", "inner", 5.0f);

	context->outer = blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_attach_one(show, blur);
	nemoshow_filter_set_blur(blur, "high", "outer", 5.0f);

	context->solid = blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_attach_one(show, blur);
	nemoshow_filter_set_blur(blur, "high", "solid", 15.0f);

	one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_attach_one(show, one);
	nemoshow_item_attach_one(canvas, one);
	nemoshow_item_set_x(one, 0.0f);
	nemoshow_item_set_y(one, 0.0f);
	nemoshow_item_set_width(one, 30.0f);
	nemoshow_item_set_height(one, 30.0f);
	nemoshow_item_set_filter(one, context->outer);
	nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
	nemoshow_item_set_stroke_color(one, 0x1e, 0xdc, 0xdc, 0xff);
	nemoshow_item_set_stroke_width(one, 3.0f);
	nemoshow_item_set_tsr(one);
	nemoshow_item_translate(one, width / 2.0f, height / 2.0f);
	nemoshow_item_load_svg(one, "/home/root/images/STE-SHELL/MAIN-ICON/System.svg");

	nemocanvas_dispatch_frame(NEMOSHOW_AT(show, canvas));

	nemotool_run(tool);

	nemoshow_destroy_on_tale(show);

err2:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

err1:
	free(context);

	return 0;
}
