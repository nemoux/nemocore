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

typedef enum {
	NEMOMINE_BOMB_STATE = (1 << 0),
	NEMOMINE_CHECK_STATE = (1 << 1),
	NEMOMINE_SAFE_STATE = (1 << 2),
} NemoMineState;

struct minecontext {
	struct nemotool *tool;

	struct nemotale *tale;

	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;
	struct showone *canvas;
	struct showone *inner, *outer, *solid;
	struct showone *ease;

	struct showone *box;
	struct showone *flag;

	struct showone **boxes;
	struct showone **ones;
	uint32_t *mines;

	int columns;
	int rows;
	int bombs;
	int size;

	int safes;

	int width, height;
};

static int nemomine_count_neighbors(struct minecontext *context, int index)
{
	static int neighbors[8] = {
		-context->columns - 1,
		-context->columns - 0,
		-context->columns + 1,
		-1,
		+1,
		context->columns - 1,
		context->columns - 0,
		context->columns + 1
	};
	int count = 0;
	int i;

	for (i = 0; i < 8; i++) {
		if (neighbors[i] + index < 0 || neighbors[i] + index >= context->columns * context->rows)
			continue;

		if (context->mines[neighbors[i] + index] & NEMOMINE_BOMB_STATE)
			count++;
	}

	return count;
}

static void nemomine_prepare_mines(struct minecontext *context)
{
	int index;
	int i;

	memset(context->mines, 0, sizeof(uint32_t) * context->columns * context->rows);

	for (i = 0; i < context->bombs; i++) {
retry:
		index = random_get_int(0, context->columns * context->rows);

		if (context->mines[index] & NEMOMINE_BOMB_STATE)
			goto retry;

		context->mines[index] = NEMOMINE_BOMB_STATE;
	}
}

static void nemomine_finish_mines(struct minecontext *context)
{
	int i;

	for (i = 0; i < context->columns * context->rows; i++) {
		if (context->ones[i] != NULL) {
			nemoshow_one_destroy(context->ones[i]);

			context->ones[i] = NULL;
		}
	}
}

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
				}
			}

			if (nemotale_is_single_click(tale, event, type)) {
				struct showtransition *trans;
				struct showone *sequence;
				struct showone *frame;
				struct showone *set0;
				struct showone *pick;
				struct showone *one;
				uint32_t tag;
				int neighbors;

				pick = nemoshow_canvas_pick_one(context->canvas, event->x, event->y);
				if (pick != NULL) {
					tag = nemoshow_one_get_tag(pick) - 1;

					if (!(context->mines[tag] & NEMOMINE_SAFE_STATE)) {
						if (context->mines[tag] & NEMOMINE_CHECK_STATE) {
							if (context->mines[tag] & NEMOMINE_BOMB_STATE) {
								nemomine_finish_mines(context);
								nemomine_prepare_mines(context);

								nemocanvas_dispatch_frame(canvas);
							} else {
								neighbors = nemomine_count_neighbors(context, tag);

								frame = nemoshow_sequence_create_frame();
								nemoshow_sequence_set_timing(frame, 1.0f);

								set0 = nemoshow_sequence_create_set();
								nemoshow_sequence_set_source(set0, context->ones[tag]);
								nemoshow_sequence_set_cattr(set0, "fill", 0xff, 0x8c, 0x32, 0xff, NEMOSHOW_STYLE_DIRTY);
								nemoshow_one_attach_one(frame, set0);

								sequence = nemoshow_sequence_create_easy(show, frame, NULL);

								trans = nemoshow_transition_create(context->ease, 500, 0);
								nemoshow_transition_check_one(trans, context->ones[tag]);
								nemoshow_transition_attach_sequence(trans, sequence);
								nemoshow_attach_transition(show, trans);

								nemocanvas_dispatch_frame(canvas);

								context->mines[tag] |= NEMOMINE_SAFE_STATE;
							}
						} else {
							context->ones[tag] = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
							nemoshow_attach_one(context->show, one);
							nemoshow_item_attach_one(context->canvas, one);
							nemoshow_one_set_tag(one, tag + 1);
							nemoshow_item_set_x(one, 0.0f);
							nemoshow_item_set_y(one, 0.0f);
							nemoshow_item_set_width(one, context->size);
							nemoshow_item_set_height(one, context->size);
							nemoshow_item_set_filter(one, context->inner);
							nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
							nemoshow_item_set_alpha(one, 0.0f);
							nemoshow_item_set_tsr(one);
							nemoshow_item_translate(one, (tag % context->columns) * context->size, (tag / context->columns) * context->size);
							nemoshow_item_path_append(one, context->flag);

							frame = nemoshow_sequence_create_frame();
							nemoshow_sequence_set_timing(frame, 1.0f);

							set0 = nemoshow_sequence_create_set();
							nemoshow_sequence_set_source(set0, one);
							nemoshow_sequence_set_dattr(set0, "alpha", 1.0f, NEMOSHOW_STYLE_DIRTY);
							nemoshow_one_attach_one(frame, set0);

							sequence = nemoshow_sequence_create_easy(show, frame, NULL);

							trans = nemoshow_transition_create(context->ease, 500, 0);
							nemoshow_transition_check_one(trans, one);
							nemoshow_transition_attach_sequence(trans, sequence);
							nemoshow_attach_transition(show, trans);

							nemocanvas_dispatch_frame(canvas);

							context->mines[tag] |= NEMOMINE_CHECK_STATE;
						}
					}
				}
			}
		}
	}
}

static void nemomine_prepare_boxes(struct minecontext *context)
{
	struct showone *one;
	int index;
	int i, j;

	for (i = 0; i < context->rows; i++) {
		for (j = 0; j < context->columns; j++) {
			index = i * context->columns + j;

			context->boxes[index] = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
			nemoshow_attach_one(context->show, one);
			nemoshow_item_attach_one(context->canvas, one);
			nemoshow_one_set_tag(one, index + 1);
			nemoshow_item_set_x(one, 0.0f);
			nemoshow_item_set_y(one, 0.0f);
			nemoshow_item_set_width(one, context->size);
			nemoshow_item_set_height(one, context->size);
			nemoshow_item_set_filter(one, context->inner);
			nemoshow_item_set_stroke_color(one, 0x1e, 0xdc, 0xdc, 0xff);
			nemoshow_item_set_stroke_width(one, 1.0f);
			nemoshow_item_set_tsr(one);
			nemoshow_item_translate(one, j * context->size, i * context->size);
			nemoshow_item_path_append(one, context->box);
		}
	}
}

static void nemomine_finish_boxes(struct minecontext *context)
{
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
	struct nemotale *tale;
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

	context->tale = tale = NEMOSHOW_AT(show, tale);

	nemocanvas_set_min_size(NEMOSHOW_AT(show, canvas),
			width * 1.0f, height * 1.0f);
	nemocanvas_set_max_size(NEMOSHOW_AT(show, canvas),
			width * 5.0f, height * 5.0f);

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
	nemoshow_filter_set_blur(blur, "high", "inner", 3.0f);

	context->outer = blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_attach_one(show, blur);
	nemoshow_filter_set_blur(blur, "high", "outer", 3.0f);

	context->solid = blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_attach_one(show, blur);
	nemoshow_filter_set_blur(blur, "high", "solid", 5.0f);

	context->box = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_attach_one(show, one);
	nemoshow_item_set_x(one, 0.0f);
	nemoshow_item_set_y(one, 0.0f);
	nemoshow_item_set_width(one, size);
	nemoshow_item_set_height(one, size);
	nemoshow_item_load_svg(one, "/home/root/images/NEMO-MINE/Box.svg");

	context->flag = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_attach_one(show, one);
	nemoshow_item_set_x(one, 0.0f);
	nemoshow_item_set_y(one, 0.0f);
	nemoshow_item_set_width(one, size);
	nemoshow_item_set_height(one, size);
	nemoshow_item_load_svg(one, "/home/root/images/STE-SHELL/MAIN-ICON/System.svg");

	context->boxes = (struct showone **)malloc(sizeof(struct showone *) * context->columns * context->rows);
	if (context->boxes == NULL)
		goto err3;
	memset(context->boxes, 0, sizeof(struct showone *) * context->columns * context->rows);

	context->ones = (struct showone **)malloc(sizeof(struct showone *) * context->columns * context->rows);
	if (context->ones == NULL)
		goto err3;
	memset(context->ones, 0, sizeof(struct showone *) * context->columns * context->rows);

	context->mines = (uint32_t *)malloc(sizeof(uint32_t) * context->columns * context->rows);
	if (context->mines == NULL)
		goto err3;
	memset(context->mines, 0, sizeof(uint32_t) * context->columns * context->rows);

	nemomine_prepare_boxes(context);
	nemomine_prepare_mines(context);

	nemocanvas_dispatch_frame(NEMOSHOW_AT(show, canvas));

	nemotool_run(tool);

	nemomine_finish_boxes(context);

err3:
	nemoshow_destroy_on_tale(show);

err2:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

err1:
	free(context);

	return 0;
}
