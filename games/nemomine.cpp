#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <showhelper.h>
#include <nemolog.h>
#include <nemomisc.h>

typedef enum {
	NEMOMINE_NONE_STATE = 0,
	NEMOMINE_CHECK_STATE = 1,
	NEMOMINE_CONFIRM_STATE = 2,
	NEMOMINE_LAST_STATE
} NemoMineState;

struct mineone {
	struct showone *box;
	struct showone *one;

	int is_bomb;

	int state;
};

struct minecontext {
	struct nemotool *tool;

	struct nemotimer *timer;

	struct nemotale *tale;

	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;
	struct showone *canvas;
	struct showone *inner, *outer, *solid;
	struct showone *ease;

	struct showone *numbers[10];
	struct showone *bomb;
	struct showone *flag;

	struct showone *font;

	struct showone *exit;
	struct showone *pin;
	struct showone *reset;
	struct showone *time;

	struct mineone *ones;

	int is_pinning;
	int is_playing;

	int columns;
	int rows;
	int bombs;
	int size;

	int found;

	uint32_t secs;

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
	struct mineone *mone;
	int count = 0;
	int i;

	for (i = 0; i < 8; i++) {
		if (neighbors[i] + index < 0 || neighbors[i] + index >= context->columns * context->rows)
			continue;

		mone = &context->ones[neighbors[i] + index];

		if (mone->is_bomb != 0)
			count++;
	}

	return count;
}

static void nemomine_prepare_mines(struct minecontext *context)
{
	struct mineone *mone;
	int index;
	int i;

	for (i = 0; i < context->columns * context->rows; i++) {
		mone = &context->ones[i];

		mone->is_bomb = 0;

		mone->state = NEMOMINE_NONE_STATE;

		nemoshow_item_set_fill_color(mone->box, 0x1e, 0xdc, 0xdc, 0x20);
		nemoshow_item_set_stroke_color(mone->box, 0x1e, 0xdc, 0xdc, 0x20);
		nemoshow_item_set_stroke_width(mone->box, 0.0f);

		nemoshow_item_set_alpha(mone->one, 0.0f);
		nemoshow_item_set_fill_color(mone->one, 0x1e, 0xdc, 0xdc, 0xff);
		nemoshow_item_path_clear(mone->one);

		nemoshow_one_dirty(mone->box, NEMOSHOW_STYLE_DIRTY);
		nemoshow_one_dirty(mone->one, NEMOSHOW_PATH_DIRTY);
	}

	for (i = 0; i < context->bombs; i++) {
retry:
		index = random_get_int(0, context->columns * context->rows);

		mone = &context->ones[index];

		if (mone->is_bomb != 0)
			goto retry;

		mone->is_bomb = 1;
	}

	nemoshow_item_set_fill_color(context->reset, 0x1e, 0xdc, 0xdc, 0xff);
	nemoshow_one_dirty(context->reset, NEMOSHOW_STYLE_DIRTY);

	nemotimer_set_timeout(context->timer, 1000);

	context->secs = 0;
	context->found = 0;

	context->is_playing = 1;
}

static void nemomine_finish_mines(struct minecontext *context)
{
	struct mineone *mone;
	int i;

	for (i = 0; i < context->columns * context->rows; i++) {
		mone = &context->ones[i];

		if (mone->is_bomb == 0)
			continue;

		nemoshow_item_set_alpha(mone->one, 1.0f);
		nemoshow_item_set_fill_color(mone->one, 0xed, 0x1c, 0x24, 0xff);
		nemoshow_item_path_clear(mone->one);
		nemoshow_item_path_append(mone->one, context->bomb);

		nemoshow_one_dirty(mone->one, NEMOSHOW_STYLE_DIRTY);
		nemoshow_one_dirty(mone->one, NEMOSHOW_PATH_DIRTY);
	}

	nemoshow_item_set_fill_color(context->reset, 0xed, 0x1c, 0x24, 0xff);
	nemoshow_one_dirty(context->reset, NEMOSHOW_STYLE_DIRTY);

	nemotimer_set_timeout(context->timer, 0);

	context->is_playing = 0;
}

static void nemomine_rotate_pin(struct minecontext *context, double ro)
{
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *frame;
	struct showone *set0;

	frame = nemoshow_sequence_create_frame();
	nemoshow_sequence_set_timing(frame, 1.0f);

	set0 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set0, context->pin);
	nemoshow_sequence_set_dattr(set0, "ro", ro, NEMOSHOW_MATRIX_DIRTY);
	nemoshow_one_attach_one(frame, set0);

	sequence = nemoshow_sequence_create_easy(context->show, frame, NULL);

	trans = nemoshow_transition_create(context->ease, 500, 0);
	nemoshow_transition_check_one(trans, context->pin);
	nemoshow_transition_attach_sequence(trans, sequence);
	nemoshow_attach_transition(context->show, trans);
}

static void nemomine_check_mine(struct minecontext *context, uint32_t tag)
{
	struct mineone *mone = &context->ones[tag];
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *frame;
	struct showone *set0;

	mone->state = NEMOMINE_CHECK_STATE;

	nemoshow_item_path_append(mone->one, context->flag);

	frame = nemoshow_sequence_create_frame();
	nemoshow_sequence_set_timing(frame, 1.0f);

	set0 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set0, mone->one);
	nemoshow_sequence_set_cattr(set0, "fill", 0xbb, 0xe5, 0xa9, 0xff, NEMOSHOW_STYLE_DIRTY);
	nemoshow_sequence_set_dattr(set0, "alpha", 1.0f, NEMOSHOW_STYLE_DIRTY);
	nemoshow_one_attach_one(frame, set0);

	sequence = nemoshow_sequence_create_easy(context->show, frame, NULL);

	trans = nemoshow_transition_create(context->ease, 500, 0);
	nemoshow_transition_check_one(trans, mone->one);
	nemoshow_transition_attach_sequence(trans, sequence);
	nemoshow_attach_transition(context->show, trans);
}

static void nemomine_uncheck_mine(struct minecontext *context, uint32_t tag)
{
	struct mineone *mone = &context->ones[tag];
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *frame;
	struct showone *set0;

	mone->state = NEMOMINE_NONE_STATE;

	nemoshow_item_path_clear(mone->one);

	frame = nemoshow_sequence_create_frame();
	nemoshow_sequence_set_timing(frame, 1.0f);

	set0 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set0, mone->one);
	nemoshow_sequence_set_dattr(set0, "alpha", 0.0f, NEMOSHOW_STYLE_DIRTY);
	nemoshow_one_attach_one(frame, set0);

	sequence = nemoshow_sequence_create_easy(context->show, frame, NULL);

	trans = nemoshow_transition_create(context->ease, 500, 0);
	nemoshow_transition_check_one(trans, mone->one);
	nemoshow_transition_attach_sequence(trans, sequence);
	nemoshow_attach_transition(context->show, trans);
}

static void nemomine_confirm_mine(struct minecontext *context, uint32_t tag)
{
	static int neighbors[4] = {
		-context->columns,
		-1,
		+1,
		context->columns,
	};
	struct mineone *mone = &context->ones[tag];
	struct mineone *none;
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *frame;
	struct showone *set0, *set1;
	int nneighbors;
	int i;

	mone->state = NEMOMINE_CONFIRM_STATE;

	nneighbors = nemomine_count_neighbors(context, tag);
	if (nneighbors == 0) {
		for (i = 0; i < 4; i++) {
			if (neighbors[i] + tag < 0 || neighbors[i] + tag >= context->columns * context->rows)
				continue;

			none = &context->ones[neighbors[i] + tag];
			if (none->is_bomb != 0 || none->state == NEMOMINE_CONFIRM_STATE)
				continue;

			nemomine_confirm_mine(context, neighbors[i] + tag);
		}

		frame = nemoshow_sequence_create_frame();
		nemoshow_sequence_set_timing(frame, 1.0f);

		set0 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set0, mone->box);
		nemoshow_sequence_set_cattr(set0, "fill", 0x0, 0x0, 0x0, 0x0, NEMOSHOW_STYLE_DIRTY);
		nemoshow_sequence_set_dattr(set0, "stroke-width", 1.0f, NEMOSHOW_SHAPE_DIRTY | NEMOSHOW_STYLE_DIRTY);
		nemoshow_one_attach_one(frame, set0);

		set1 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set1, mone->one);
		nemoshow_sequence_set_dattr(set1, "alpha", 0.0f, NEMOSHOW_STYLE_DIRTY);
		nemoshow_one_attach_one(frame, set1);

		sequence = nemoshow_sequence_create_easy(context->show, frame, NULL);

		trans = nemoshow_transition_create(context->ease, 500, 0);
		nemoshow_transition_check_one(trans, mone->box);
		nemoshow_transition_check_one(trans, mone->one);
		nemoshow_transition_attach_sequence(trans, sequence);
		nemoshow_attach_transition(context->show, trans);
	} else {
		nemoshow_item_path_clear(mone->one);
		nemoshow_item_path_append(mone->one, context->numbers[nneighbors]);

		frame = nemoshow_sequence_create_frame();
		nemoshow_sequence_set_timing(frame, 1.0f);

		set0 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set0, mone->one);
		nemoshow_sequence_set_cattr(set0, "fill", 0xff, 0x8c, 0x32, 0xff, NEMOSHOW_STYLE_DIRTY);
		nemoshow_sequence_set_dattr(set0, "alpha", 1.0f, NEMOSHOW_STYLE_DIRTY);
		nemoshow_one_attach_one(frame, set0);

		set1 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set1, mone->box);
		nemoshow_sequence_set_cattr(set1, "fill", 0x0, 0x0, 0x0, 0x0, NEMOSHOW_STYLE_DIRTY);
		nemoshow_sequence_set_dattr(set1, "stroke-width", 1.0f, NEMOSHOW_SHAPE_DIRTY | NEMOSHOW_STYLE_DIRTY);
		nemoshow_one_attach_one(frame, set1);

		sequence = nemoshow_sequence_create_easy(context->show, frame, NULL);

		trans = nemoshow_transition_create(context->ease, 500, 0);
		nemoshow_transition_check_one(trans, mone->one);
		nemoshow_transition_attach_sequence(trans, sequence);
		nemoshow_attach_transition(context->show, trans);
	}
}

static void nemomine_dispatch_tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
	uint32_t id = nemotale_node_get_id(node);

	if (id == 1) {
		struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);
		struct minecontext *context = (struct minecontext *)nemoshow_get_userdata(show);
		struct nemocanvas *canvas = NEMOSHOW_AT(show, canvas);

		if (context->is_pinning != 0) {
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
				struct showone *pick;
				struct showone *one;
				uint32_t tag;

				pick = nemoshow_canvas_pick_one(context->canvas, event->x, event->y);
				if (pick != NULL) {
					tag = nemoshow_one_get_tag(pick) - 1;

					if (tag == 10000) {
						nemotool_exit(context->tool);
					} else if (tag == 10001) {
						context->is_pinning = !context->is_pinning;

						nemomine_rotate_pin(context, 0.0f);

						nemocanvas_dispatch_frame(canvas);
					} else if (tag == 10002) {
						nemomine_prepare_mines(context);

						nemocanvas_dispatch_frame(canvas);
					}
				}
			}
		} else {
			struct showone *pick;
			uint32_t tag;

			pick = nemoshow_canvas_pick_one(context->canvas, event->x, event->y);
			if (pick != NULL) {
				tag = nemoshow_one_get_tag(pick) - 1;

				if (nemotale_is_single_click(tale, event, type)) {
					if (tag < 10000 && context->is_playing != 0) {
						struct mineone *mone = &context->ones[tag];

						if (mone->state != NEMOMINE_CONFIRM_STATE) {
							if (mone->is_bomb != 0) {
								nemomine_finish_mines(context);

								nemocanvas_dispatch_frame(canvas);
							} else {
								nemomine_confirm_mine(context, tag);

								nemocanvas_dispatch_frame(canvas);
							}
						}
					} else if (tag == 10000) {
						nemotool_exit(context->tool);
					} else if (tag == 10001) {
						context->is_pinning = !context->is_pinning;

						nemomine_rotate_pin(context, 45.0f);

						nemocanvas_dispatch_frame(canvas);
					} else if (tag == 10002) {
						nemomine_prepare_mines(context);

						nemocanvas_dispatch_frame(canvas);
					}
				}

				if (nemotale_is_long_press(tale, event, type)) {
					struct taletap *tap = nemotale_touch_get_tap(tale, event->device);

					if (tag < 10000 && context->is_playing != 0) {
						struct mineone *mone = &context->ones[tag];

						if (mone->state == NEMOMINE_NONE_STATE) {
							nemomine_check_mine(context, tag);

							nemocanvas_dispatch_frame(canvas);
						} else if (mone->state == NEMOMINE_CHECK_STATE) {
							nemomine_uncheck_mine(context, tag);

							nemocanvas_dispatch_frame(canvas);
						}
					}

					nemotale_tap_set_state(tap, NEMOTALE_TAP_USED_STATE);
				}
			}
		}
	}
}

static void nemomine_dispatch_timer(struct nemotimer *timer, void *data)
{
	struct minecontext *context = (struct minecontext *)data;
	struct nemocanvas *canvas = NEMOSHOW_AT(context->show, canvas);
	char text[64];

	nemotimer_set_timeout(timer, 1000);

	snprintf(text, sizeof(text), "%d", ++context->secs);

	nemoshow_item_set_text(context->time, text);

	nemocanvas_dispatch_frame(canvas);
}

static void nemomine_prepare_boxes(struct minecontext *context)
{
	struct mineone *mone;
	struct showone *one;
	int index;
	int i, j;

	for (i = 0; i < context->rows; i++) {
		for (j = 0; j < context->columns; j++) {
			index = i * context->columns + j;

			mone = &context->ones[index];

			mone->box = one = nemoshow_item_create(NEMOSHOW_RRECT_ITEM);
			nemoshow_attach_one(context->show, one);
			nemoshow_item_attach_one(context->canvas, one);
			nemoshow_one_set_tag(one, index + 1);
			nemoshow_item_set_x(one, 1.0f);
			nemoshow_item_set_y(one, 1.0f);
			nemoshow_item_set_rx(one, 2.0f);
			nemoshow_item_set_ry(one, 2.0f);
			nemoshow_item_set_width(one, context->size - 2.0f);
			nemoshow_item_set_height(one, context->size - 2.0f);
			nemoshow_item_set_filter(one, context->inner);
			nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0x40);
			nemoshow_item_set_stroke_color(one, 0x1e, 0xdc, 0xdc, 0x40);
			nemoshow_item_set_stroke_width(one, 0.0f);
			nemoshow_item_set_tsr(one);
			nemoshow_item_translate(one, j * context->size, i * context->size + context->size);

			mone->one = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
			nemoshow_attach_one(context->show, one);
			nemoshow_item_attach_one(context->canvas, one);
			nemoshow_one_set_tag(one, index + 1);
			nemoshow_item_set_x(one, 2.0f);
			nemoshow_item_set_y(one, 2.0f);
			nemoshow_item_set_width(one, context->size - 4.0f);
			nemoshow_item_set_height(one, context->size - 4.0f);
			nemoshow_item_set_filter(one, context->inner);
			nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
			nemoshow_item_set_tsr(one);
			nemoshow_item_translate(one, j * context->size, i * context->size + context->size);
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
	struct nemotimer *timer;
	struct nemotale *tale;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct showone *blur;
	struct showone *ease;
	struct showone *font;
	struct showone *one;
	char name[128];
	int columns = 30;
	int rows = 16;
	int bombs = 99;
	int size = 16;
	int width, height;
	int opt;
	int i;

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
	context->height = height = rows * size + size;

	context->is_pinning = 1;

	context->tool = tool = nemotool_create();
	if (tool == NULL)
		goto err1;
	nemotool_connect_wayland(tool, NULL);

	context->show = show = nemoshow_create_on_tale(tool, width, height, nemomine_dispatch_tale_event);
	if (show == NULL)
		goto err2;
	nemoshow_set_userdata(show, context);

	context->tale = tale = NEMOSHOW_AT(show, tale);
	nemotale_set_single_click_gesture(tale, 150, 50);
	nemotale_set_long_press_gesture(tale, 300, 50);

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

	for (i = 0; i < 10; i++) {
		snprintf(name, sizeof(name), "/home/root/images/Game/Mine/Mine-%d.svg", i);

		context->numbers[i] = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
		nemoshow_attach_one(show, one);
		nemoshow_item_set_x(one, 2.0f);
		nemoshow_item_set_y(one, 2.0f);
		nemoshow_item_set_width(one, size - 4.0f);
		nemoshow_item_set_height(one, size - 4.0f);
		nemoshow_item_load_svg(one, name);
	}

	context->bomb = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_attach_one(show, one);
	nemoshow_item_set_x(one, 2.0f);
	nemoshow_item_set_y(one, 2.0f);
	nemoshow_item_set_width(one, size - 4.0f);
	nemoshow_item_set_height(one, size - 4.0f);
	nemoshow_item_load_svg(one, "/home/root/images/Game/Mine/Mine-bomb.svg");

	context->flag = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_attach_one(show, one);
	nemoshow_item_set_x(one, 2.0f);
	nemoshow_item_set_y(one, 2.0f);
	nemoshow_item_set_width(one, size - 4.0f);
	nemoshow_item_set_height(one, size - 4.0f);
	nemoshow_item_load_svg(one, "/home/root/images/Game/Mine/Mine-finder.svg");

	context->exit = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_attach_one(show, one);
	nemoshow_item_attach_one(canvas, one);
	nemoshow_one_set_tag(one, 10000 + 1);
	nemoshow_item_set_x(one, 0.0f);
	nemoshow_item_set_y(one, 0.0f);
	nemoshow_item_set_width(one, size);
	nemoshow_item_set_height(one, size);
	nemoshow_item_set_filter(one, context->inner);
	nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
	nemoshow_item_set_tsr(one);
	nemoshow_item_translate(one, 0.0f, 0.0f);
	nemoshow_item_load_svg(one, "/home/root/images/Game/Mine/Mine-Quit.svg");

	context->pin = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_attach_one(show, one);
	nemoshow_item_attach_one(canvas, one);
	nemoshow_one_set_tag(one, 10001 + 1);
	nemoshow_item_set_x(one, 0.0f);
	nemoshow_item_set_y(one, 0.0f);
	nemoshow_item_set_width(one, size);
	nemoshow_item_set_height(one, size);
	nemoshow_item_set_filter(one, context->inner);
	nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
	nemoshow_item_set_tsr(one);
	nemoshow_item_pivot(one, size / 2.0f, size / 2.0f);
	nemoshow_item_translate(one, (context->columns - 1) * context->size, 0.0f);
	nemoshow_item_rotate(one, 45.0f);
	nemoshow_item_load_svg(one, "/home/root/images/Game/Mine/Mine-pin.svg");

	context->reset = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_attach_one(show, one);
	nemoshow_item_attach_one(canvas, one);
	nemoshow_one_set_tag(one, 10002 + 1);
	nemoshow_item_set_x(one, 0.0f);
	nemoshow_item_set_y(one, 0.0f);
	nemoshow_item_set_width(one, size);
	nemoshow_item_set_height(one, size);
	nemoshow_item_set_filter(one, context->inner);
	nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
	nemoshow_item_set_tsr(one);
	nemoshow_item_translate(one, (context->columns / 2) * context->size, 0.0f);
	nemoshow_item_load_svg(one, "/home/root/images/Game/Mine/Mine-bomb.svg");

	context->font = font = nemoshow_font_create();
	nemoshow_attach_one(show, font);
	nemoshow_font_load(font, "/usr/share/fonts/ttf/LiberationMono-Regular.ttf");
	nemoshow_font_use_harfbuzz(font);

	context->time = one = nemoshow_item_create(NEMOSHOW_TEXT_ITEM);
	nemoshow_attach_one(show, one);
	nemoshow_item_attach_one(canvas, one);
	nemoshow_item_set_font(one, font);
	nemoshow_item_set_fontsize(one, size);
	nemoshow_item_set_anchor(one, 1.0f, 0.5f);
	nemoshow_item_set_stroke_color(one, 0x1e, 0xdc, 0xdc, 0xff);
	nemoshow_item_set_tsr(one);
	nemoshow_item_translate(one, (context->columns - 2) * context->size, context->size / 2.0f);
	nemoshow_item_set_text(one, "0");

	context->ones = (struct mineone *)malloc(sizeof(struct mineone) * context->columns * context->rows);
	if (context->ones == NULL)
		goto err3;
	memset(context->ones, 0, sizeof(struct mineone) * context->columns * context->rows);

	context->timer = timer = nemotimer_create(tool);
	nemotimer_set_callback(timer, nemomine_dispatch_timer);
	nemotimer_set_userdata(timer, context);

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
