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

	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;
	struct showone *canvas;
	struct showone *filter0;
	struct showone *filter1;

	struct showone *numbers[10];
	struct showone *bomb;
	struct showone *flag;

	struct showone *font;

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

static int nemomine_count_neighbors(struct minecontext *context, int x, int y)
{
	static int neighbors[8][2] = {
		{ -1, -1 },
		{ 0, -1 },
		{ +1, -1 },
		{ -1, 0 },
		{ +1, 0 },
		{ -1, +1 },
		{ 0, +1 },
		{ +1, +1 },
	};
	struct mineone *mone;
	int count = 0;
	int i;

	for (i = 0; i < 8; i++) {
		if (neighbors[i][0] + x < 0 || neighbors[i][0] + x >= context->columns ||
				neighbors[i][1] + y < 0 || neighbors[i][1] + y >= context->rows)
			continue;

		mone = &context->ones[(neighbors[i][0] + x) + (neighbors[i][1] + y) * context->columns];

		if (mone->is_bomb != 0)
			count++;
	}

	return count;
}

static void nemomine_prepare_game(struct minecontext *context)
{
	struct mineone *mone;
	int index;
	int i;

	for (i = 0; i < context->columns * context->rows; i++) {
		mone = &context->ones[i];

		mone->is_bomb = 0;

		mone->state = NEMOMINE_NONE_STATE;

		nemoshow_item_set_fill_color(mone->box, 0x1e, 0xdc, 0xdc, 0xff);
		nemoshow_item_set_stroke_color(mone->box, 0x1e, 0xdc, 0xdc, 0xff);
		nemoshow_item_set_stroke_width(mone->box, 1.0f);

		nemoshow_item_set_alpha(mone->one, 0.0f);
		nemoshow_item_set_fill_color(mone->one, 0x1e, 0xdc, 0xdc, 0xff);
		nemoshow_item_path_clear(mone->one);
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

	nemotimer_set_timeout(context->timer, 1000);

	context->secs = 0;
	context->found = 0;

	context->is_playing = 1;
}

static void nemomine_finish_game(struct minecontext *context)
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
	}

	nemoshow_item_set_fill_color(context->reset, 0xed, 0x1c, 0x24, 0xff);

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
	nemoshow_one_attach(frame, set0);

	sequence = nemoshow_sequence_create_easy(context->show, frame, NULL);

	trans = nemoshow_transition_create(NEMOSHOW_CUBIC_INOUT_EASE, 500, 0);
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
	struct showone *set0, *set1;

	mone->state = NEMOMINE_CHECK_STATE;

	nemoshow_item_path_append(mone->one, context->flag);

	frame = nemoshow_sequence_create_frame();
	nemoshow_sequence_set_timing(frame, 1.0f);

	set0 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set0, mone->box);
	nemoshow_sequence_set_cattr(set0, "fill", 0x1e, 0xdc, 0xdc, 0x40, NEMOSHOW_STYLE_DIRTY);
	nemoshow_one_attach(frame, set0);

	set1 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set1, mone->one);
	nemoshow_sequence_set_cattr(set1, "fill", 0xbb, 0xe5, 0xa9, 0xff, NEMOSHOW_STYLE_DIRTY);
	nemoshow_sequence_set_dattr(set1, "alpha", 1.0f, NEMOSHOW_STYLE_DIRTY);
	nemoshow_one_attach(frame, set1);

	sequence = nemoshow_sequence_create_easy(context->show, frame, NULL);

	trans = nemoshow_transition_create(NEMOSHOW_CUBIC_INOUT_EASE, 500, 0);
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
	struct showone *set0, *set1;

	mone->state = NEMOMINE_NONE_STATE;

	nemoshow_item_path_clear(mone->one);

	frame = nemoshow_sequence_create_frame();
	nemoshow_sequence_set_timing(frame, 1.0f);

	set0 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set0, mone->box);
	nemoshow_sequence_set_cattr(set0, "fill", 0x1e, 0xdc, 0xdc, 0xff, NEMOSHOW_STYLE_DIRTY);
	nemoshow_one_attach(frame, set0);

	set1 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set1, mone->one);
	nemoshow_sequence_set_dattr(set1, "alpha", 0.0f, NEMOSHOW_STYLE_DIRTY);
	nemoshow_one_attach(frame, set1);

	sequence = nemoshow_sequence_create_easy(context->show, frame, NULL);

	trans = nemoshow_transition_create(NEMOSHOW_CUBIC_INOUT_EASE, 500, 0);
	nemoshow_transition_check_one(trans, mone->one);
	nemoshow_transition_attach_sequence(trans, sequence);
	nemoshow_attach_transition(context->show, trans);
}

static void nemomine_confirm_mine(struct minecontext *context, uint32_t tag)
{
	static int neighbors[4][2] = {
		{ 0, -1 },
		{ -1, 0 },
		{ +1, 0 },
		{ 0, +1 }
	};
	struct mineone *mone = &context->ones[tag];
	struct mineone *none;
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *frame;
	struct showone *set0, *set1;
	int x = tag % context->columns;
	int y = tag / context->columns;
	int nneighbors;
	int i;

	mone->state = NEMOMINE_CONFIRM_STATE;

	nneighbors = nemomine_count_neighbors(context, x, y);
	if (nneighbors == 0) {
		for (i = 0; i < 4; i++) {
			if (neighbors[i][0] + x < 0 || neighbors[i][0] + x >= context->columns ||
					neighbors[i][1] + y < 0 || neighbors[i][1] + y >= context->rows)
				continue;

			none = &context->ones[(neighbors[i][0] + x) + (neighbors[i][1] + y) * context->columns];
			if (none->is_bomb != 0 || none->state == NEMOMINE_CONFIRM_STATE)
				continue;

			nemomine_confirm_mine(context, (neighbors[i][0] + x) + (neighbors[i][1] + y) * context->columns);
		}

		frame = nemoshow_sequence_create_frame();
		nemoshow_sequence_set_timing(frame, 1.0f);

		set0 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set0, mone->box);
		nemoshow_sequence_set_cattr(set0, "fill", 0x0, 0x0, 0x0, 0x0, NEMOSHOW_STYLE_DIRTY);
		nemoshow_sequence_set_dattr(set0, "stroke-width", 1.0f, NEMOSHOW_SHAPE_DIRTY | NEMOSHOW_STYLE_DIRTY);
		nemoshow_one_attach(frame, set0);

		set1 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set1, mone->one);
		nemoshow_sequence_set_dattr(set1, "alpha", 0.0f, NEMOSHOW_STYLE_DIRTY);
		nemoshow_one_attach(frame, set1);

		sequence = nemoshow_sequence_create_easy(context->show, frame, NULL);

		trans = nemoshow_transition_create(NEMOSHOW_CUBIC_INOUT_EASE, 500, 0);
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
		nemoshow_one_attach(frame, set0);

		set1 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set1, mone->box);
		nemoshow_sequence_set_cattr(set1, "fill", 0x0, 0x0, 0x0, 0x0, NEMOSHOW_STYLE_DIRTY);
		nemoshow_sequence_set_dattr(set1, "stroke-width", 1.0f, NEMOSHOW_SHAPE_DIRTY | NEMOSHOW_STYLE_DIRTY);
		nemoshow_one_attach(frame, set1);

		sequence = nemoshow_sequence_create_easy(context->show, frame, NULL);

		trans = nemoshow_transition_create(NEMOSHOW_CUBIC_INOUT_EASE, 500, 0);
		nemoshow_transition_check_one(trans, mone->one);
		nemoshow_transition_attach_sequence(trans, sequence);
		nemoshow_attach_transition(context->show, trans);
	}
}

static void nemomine_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, void *event)
{
	struct minecontext *context = (struct minecontext *)nemoshow_get_userdata(show);

	if (context->is_pinning != 0) {
		if (nemoshow_event_is_down(show, event) || nemoshow_event_is_up(show, event)) {
			nemoshow_event_update_taps(show, canvas, event);

			if (nemoshow_event_is_single_tap(show, event)) {
				nemoshow_view_move(show, nemoshow_event_get_serial_on(event, 0));
			} else if (nemoshow_event_is_many_taps(show, event)) {
				nemoshow_view_pick_distant(show, event, NEMOSHOW_VIEW_PICK_ALL_TYPE);
			}
		}

		if (nemoshow_event_is_single_click(show, event)) {
			struct showone *one;
			uint32_t tag;

			tag = nemoshow_canvas_pick_tag(context->canvas, nemoshow_event_get_x(event), nemoshow_event_get_y(event));
			if (tag-- > 0) {
				if (tag == 10001) {
					context->is_pinning = !context->is_pinning;

					nemomine_rotate_pin(context, 0.0f);

					nemoshow_dispatch_frame(show);
				} else if (tag == 10002) {
					nemomine_prepare_game(context);

					nemoshow_dispatch_frame(show);
				}
			}
		}
	} else {
		uint32_t tag;

		tag = nemoshow_canvas_pick_tag(context->canvas, nemoshow_event_get_x(event), nemoshow_event_get_y(event));
		if (tag-- > 0) {
			if (nemoshow_event_is_single_click(show, event)) {
				if (tag < 10000 && context->is_playing != 0) {
					struct mineone *mone = &context->ones[tag];

					if (mone->state != NEMOMINE_CONFIRM_STATE) {
						if (mone->is_bomb != 0) {
							nemomine_finish_game(context);

							nemoshow_dispatch_frame(show);
						} else {
							nemomine_confirm_mine(context, tag);

							nemoshow_dispatch_frame(show);
						}
					}
				} else if (tag == 10001) {
					context->is_pinning = !context->is_pinning;

					nemomine_rotate_pin(context, 45.0f);

					nemoshow_dispatch_frame(show);
				} else if (tag == 10002) {
					nemomine_prepare_game(context);

					nemoshow_dispatch_frame(show);
				}
			}

			if (nemoshow_event_is_long_press(show, event)) {
				if (tag < 10000 && context->is_playing != 0) {
					struct mineone *mone = &context->ones[tag];

					if (mone->state == NEMOMINE_NONE_STATE) {
						nemomine_check_mine(context, tag);

						nemoshow_dispatch_frame(show);
					} else if (mone->state == NEMOMINE_CHECK_STATE) {
						nemomine_uncheck_mine(context, tag);

						nemoshow_dispatch_frame(show);
					}
				}

				nemoshow_event_set_used(event);
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

	nemoshow_dispatch_frame(context->show);
}

static void nemomine_prepare_ui(struct minecontext *context)
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
			nemoshow_one_attach(context->canvas, one);
			nemoshow_one_set_tag(one, index + 1);
			nemoshow_item_set_x(one, 1.0f);
			nemoshow_item_set_y(one, 1.0f);
			nemoshow_item_set_rx(one, 2.0f);
			nemoshow_item_set_ry(one, 2.0f);
			nemoshow_item_set_width(one, context->size - 2.0f);
			nemoshow_item_set_height(one, context->size - 2.0f);
			nemoshow_item_set_filter(one, context->filter1);
			nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
			nemoshow_item_set_stroke_color(one, 0x1e, 0xdc, 0xdc, 0xff);
			nemoshow_item_set_stroke_width(one, 1.0f);
			nemoshow_item_set_tsr(one);
			nemoshow_item_translate(one, j * context->size, i * context->size + context->size);

			mone->one = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
			nemoshow_one_attach(context->canvas, one);
			nemoshow_one_set_tag(one, index + 1);
			nemoshow_item_set_x(one, 2.0f);
			nemoshow_item_set_y(one, 2.0f);
			nemoshow_item_set_width(one, context->size - 4.0f);
			nemoshow_item_set_height(one, context->size - 4.0f);
			nemoshow_item_set_filter(one, context->filter0);
			nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
			nemoshow_item_set_tsr(one);
			nemoshow_item_translate(one, j * context->size, i * context->size + context->size);
		}
	}
}

static void nemomine_finish_ui(struct minecontext *context)
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
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct showone *filter;
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

	context->show = show = nemoshow_create_view(tool, width, height);
	if (show == NULL)
		goto err2;
	nemoshow_set_userdata(show, context);

	context->scene = scene = nemoshow_scene_create();
	nemoshow_scene_set_width(scene, width);
	nemoshow_scene_set_height(scene, height);
	nemoshow_set_scene(show, scene);

	context->back = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_BACK_TYPE);
	nemoshow_canvas_set_fill_color(canvas, 0.0f, 0.0f, 0.0f, 0.0f);
	nemoshow_canvas_set_alpha(canvas, 0.0f);
	nemoshow_one_attach(scene, canvas);

	context->canvas = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
	nemoshow_canvas_set_dispatch_event(canvas, nemomine_dispatch_canvas_event);
	nemoshow_one_attach(scene, canvas);

	context->filter0 = filter = nemoshow_filter_create(NEMOSHOW_EMBOSS_FILTER);
	nemoshow_filter_set_light(filter, 1.0f, 1.0f, 1.0f, 128.0f, 32.0f);
	nemoshow_filter_set_radius(filter, 0.5f);

	context->filter1 = filter = nemoshow_filter_create(NEMOSHOW_EMBOSS_FILTER);
	nemoshow_filter_set_light(filter, 1.0f, 1.0f, 1.0f, 128.0f, 32.0f);
	nemoshow_filter_set_radius(filter, 1.0f);

	for (i = 0; i < 10; i++) {
		snprintf(name, sizeof(name), NEMOUX_MINESWEEPER_RESOURCES "/mine-%d.svg", i);

		context->numbers[i] = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
		nemoshow_item_set_x(one, 2.0f);
		nemoshow_item_set_y(one, 2.0f);
		nemoshow_item_set_width(one, size - 4.0f);
		nemoshow_item_set_height(one, size - 4.0f);
		nemoshow_item_load_svg(one, name);
	}

	context->bomb = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_item_set_x(one, 2.0f);
	nemoshow_item_set_y(one, 2.0f);
	nemoshow_item_set_width(one, size - 4.0f);
	nemoshow_item_set_height(one, size - 4.0f);
	nemoshow_item_load_svg(one, NEMOUX_MINESWEEPER_RESOURCES "/mine-bomb.svg");

	context->flag = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_item_set_x(one, 2.0f);
	nemoshow_item_set_y(one, 2.0f);
	nemoshow_item_set_width(one, size - 4.0f);
	nemoshow_item_set_height(one, size - 4.0f);
	nemoshow_item_load_svg(one, NEMOUX_MINESWEEPER_RESOURCES "/mine-finder.svg");

	context->pin = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_one_attach(canvas, one);
	nemoshow_one_set_tag(one, 10001 + 1);
	nemoshow_item_set_x(one, 0.0f);
	nemoshow_item_set_y(one, 0.0f);
	nemoshow_item_set_width(one, size);
	nemoshow_item_set_height(one, size);
	nemoshow_item_set_filter(one, context->filter0);
	nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
	nemoshow_item_set_tsr(one);
	nemoshow_item_pivot(one, size / 2.0f, size / 2.0f);
	nemoshow_item_translate(one, (context->columns - 1) * context->size, 0.0f);
	nemoshow_item_rotate(one, 45.0f);
	nemoshow_item_load_svg(one, NEMOUX_MINESWEEPER_RESOURCES "/mine-pin.svg");

	context->reset = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_one_attach(canvas, one);
	nemoshow_one_set_tag(one, 10002 + 1);
	nemoshow_item_set_x(one, 0.0f);
	nemoshow_item_set_y(one, 0.0f);
	nemoshow_item_set_width(one, size);
	nemoshow_item_set_height(one, size);
	nemoshow_item_set_filter(one, context->filter0);
	nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
	nemoshow_item_set_tsr(one);
	nemoshow_item_translate(one, (context->columns / 2) * context->size, 0.0f);
	nemoshow_item_load_svg(one, NEMOUX_MINESWEEPER_RESOURCES "/mine-bomb.svg");

	context->font = font = nemoshow_font_create();
	nemoshow_font_load(font, "/usr/share/fonts/ttf/LiberationMono-Regular.ttf");
	nemoshow_font_use_harfbuzz(font);

	context->time = one = nemoshow_item_create(NEMOSHOW_TEXT_ITEM);
	nemoshow_one_attach(canvas, one);
	nemoshow_item_set_font(one, font);
	nemoshow_item_set_fontsize(one, size);
	nemoshow_item_set_anchor(one, 1.0f, 0.5f);
	nemoshow_item_set_filter(one, context->filter0);
	nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
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

	nemomine_prepare_ui(context);
	nemomine_prepare_game(context);

	nemoshow_dispatch_frame(show);

	nemotool_run(tool);

	nemomine_finish_ui(context);

err3:
	nemoshow_destroy_view(show);

err2:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

err1:
	free(context);

	return 0;
}
