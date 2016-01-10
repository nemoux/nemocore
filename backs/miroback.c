#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <nemotool.h>
#include <showhelper.h>
#include <nemomisc.h>

struct miroback {
	struct nemotool *tool;

	int32_t width, height;

	int32_t columns, rows;

	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;
	struct showone *canvas;
	struct showone *inner, *outer, *solid;
	struct showone *ease0;
	struct showone *ease1;

	struct showone **cones;
	struct showone **rones;
};

static void nemomiro_dispatch_show(struct miroback *miro, uint32_t duration, uint32_t interval);
static void nemomiro_dispatch_hide(struct miroback *miro, uint32_t duration, uint32_t interval);

static void nemomiro_dispatch_tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
	uint32_t id = nemotale_node_get_id(node);

	if (id == 1) {
	}
}

static void nemomiro_dispatch_show_transition_done(void *userdata)
{
	struct miroback *miro = (struct miroback *)userdata;

	nemomiro_dispatch_hide(miro, 1800, 100);
}

static void nemomiro_dispatch_show(struct miroback *miro, uint32_t duration, uint32_t interval)
{
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *set0;
	int i;

	for (i = 0; i <= miro->columns; i++) {
		set0 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set0, miro->cones[i]);
		nemoshow_sequence_set_dattr(set0, "height", miro->height, NEMOSHOW_SHAPE_DIRTY);

		sequence = nemoshow_sequence_create_easy(miro->show,
				nemoshow_sequence_create_frame_easy(miro->show,
					1.0f, set0, NULL),
				NULL);

		trans = nemoshow_transition_create(miro->ease0, duration, i * interval);
		nemoshow_transition_check_one(trans, miro->cones[i]);
		nemoshow_transition_attach_sequence(trans, sequence);
		nemoshow_attach_transition(miro->show, trans);
	}

	for (i = 0; i <= miro->rows; i++) {
		set0 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set0, miro->rones[i]);
		nemoshow_sequence_set_dattr(set0, "width", miro->width, NEMOSHOW_SHAPE_DIRTY);

		sequence = nemoshow_sequence_create_easy(miro->show,
				nemoshow_sequence_create_frame_easy(miro->show,
					1.0f, set0, NULL),
				NULL);

		trans = nemoshow_transition_create(miro->ease0, duration, i * interval);
		nemoshow_transition_check_one(trans, miro->rones[i]);
		nemoshow_transition_attach_sequence(trans, sequence);
		nemoshow_attach_transition(miro->show, trans);
	}

	nemoshow_set_dispatch_transition_done(miro->show, nemomiro_dispatch_show_transition_done, miro);

	nemocanvas_dispatch_frame(NEMOSHOW_AT(miro->show, canvas));
}

static void nemomiro_dispatch_hide_transition_done(void *userdata)
{
	struct miroback *miro = (struct miroback *)userdata;

	nemomiro_dispatch_show(miro, 1800, 100);
}

static void nemomiro_dispatch_hide(struct miroback *miro, uint32_t duration, uint32_t interval)
{
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *set0;
	int i;

	for (i = 0; i <= miro->columns; i++) {
		set0 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set0, miro->cones[i]);
		nemoshow_sequence_set_dattr(set0, "height", 0.0f, NEMOSHOW_SHAPE_DIRTY);

		sequence = nemoshow_sequence_create_easy(miro->show,
				nemoshow_sequence_create_frame_easy(miro->show,
					1.0f, set0, NULL),
				NULL);

		trans = nemoshow_transition_create(miro->ease0, duration, i * interval);
		nemoshow_transition_check_one(trans, miro->cones[i]);
		nemoshow_transition_attach_sequence(trans, sequence);
		nemoshow_attach_transition(miro->show, trans);
	}

	for (i = 0; i <= miro->rows; i++) {
		set0 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set0, miro->rones[i]);
		nemoshow_sequence_set_dattr(set0, "width", 0.0f, NEMOSHOW_SHAPE_DIRTY);

		sequence = nemoshow_sequence_create_easy(miro->show,
				nemoshow_sequence_create_frame_easy(miro->show,
					1.0f, set0, NULL),
				NULL);

		trans = nemoshow_transition_create(miro->ease0, duration, i * interval);
		nemoshow_transition_check_one(trans, miro->rones[i]);
		nemoshow_transition_attach_sequence(trans, sequence);
		nemoshow_attach_transition(miro->show, trans);
	}

	nemoshow_set_dispatch_transition_done(miro->show, nemomiro_dispatch_hide_transition_done, miro);

	nemocanvas_dispatch_frame(NEMOSHOW_AT(miro->show, canvas));
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "width",			required_argument,			NULL,		'w' },
		{ "height",			required_argument,			NULL,		'h' },
		{ "columns",		required_argument,			NULL,		'c' },
		{ "rows",				required_argument,			NULL,		'r' },
		{ 0 }
	};

	struct miroback *miro;
	struct nemotool *tool;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct showone *blur;
	struct showone *ease;
	struct showone *one;
	int32_t width = 1920;
	int32_t height = 1080;
	int32_t columns = 16;
	int32_t rows = 8;
	int opt;
	int i;

	while (opt = getopt_long(argc, argv, "w:h:c:r:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'w':
				width = strtoul(optarg, NULL, 10);
				break;

			case 'h':
				height = strtoul(optarg, NULL, 10);
				break;

			case 'c':
				columns = strtoul(optarg, NULL, 10);
				break;

			case 'r':
				rows = strtoul(optarg, NULL, 10);
				break;

			default:
				break;
		}
	}

	miro = (struct miroback *)malloc(sizeof(struct miroback));
	if (miro == NULL)
		return -1;
	memset(miro, 0, sizeof(struct miroback));

	miro->width = width;
	miro->height = height;

	miro->columns = columns;
	miro->rows = rows;

	miro->tool = tool = nemotool_create();
	if (tool == NULL)
		goto err1;
	nemotool_connect_wayland(tool, NULL);

	miro->show = show = nemoshow_create_canvas(tool, width, height, nemomiro_dispatch_tale_event);
	if (show == NULL)
		goto err2;
	nemoshow_set_userdata(show, miro);

	nemocanvas_opaque(NEMOSHOW_AT(show, canvas), 0, 0, width, height);
	nemocanvas_set_layer(NEMOSHOW_AT(show, canvas), NEMO_SURFACE_LAYER_TYPE_BACKGROUND);

	miro->scene = scene = nemoshow_scene_create();
	nemoshow_scene_set_width(scene, width);
	nemoshow_scene_set_height(scene, height);
	nemoshow_attach_one(show, scene);

	miro->back = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_BACK_TYPE);
	nemoshow_canvas_set_fill_color(canvas, 0.0f, 0.0f, 0.0f, 255.0f);
	nemoshow_canvas_set_alpha(canvas, 0.0f);
	nemoshow_attach_one(show, canvas);
	nemoshow_one_attach(scene, canvas);

	miro->canvas = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
	nemoshow_canvas_set_event(canvas, 1);
	nemoshow_attach_one(show, canvas);
	nemoshow_one_attach(scene, canvas);

	nemoshow_set_scene(show, scene);
	nemoshow_set_size(show, width, height);

	miro->ease0 = ease = nemoshow_ease_create();
	nemoshow_ease_set_type(ease, NEMOEASE_CUBIC_OUT_TYPE);

	miro->ease1 = ease = nemoshow_ease_create();
	nemoshow_ease_set_type(ease, NEMOEASE_LINEAR_TYPE);

	miro->inner = blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_attach_one(show, blur);
	nemoshow_filter_set_blur(blur, "high", "inner", 3.0f);

	miro->outer = blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_attach_one(show, blur);
	nemoshow_filter_set_blur(blur, "high", "outer", 3.0f);

	miro->solid = blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_attach_one(show, blur);
	nemoshow_filter_set_blur(blur, "high", "solid", 5.0f);

	miro->cones = (struct showone **)malloc(sizeof(struct showone *) * (columns + 1));
	miro->rones = (struct showone **)malloc(sizeof(struct showone *) * (rows + 1));

	for (i = 0; i <= columns; i++) {
		miro->cones[i] = one = nemoshow_item_create(NEMOSHOW_LINE_ITEM);
		nemoshow_attach_one(show, one);
		nemoshow_one_attach(canvas, one);
		nemoshow_item_set_x(one, i * (width / columns));
		nemoshow_item_set_y(one, 0.0f);
		nemoshow_item_set_width(one, i * (width / columns));
		nemoshow_item_set_height(one, 0.0f);
		nemoshow_item_set_stroke_color(one, 0x1e, 0xdc, 0xdc, 0xff);
		nemoshow_item_set_stroke_width(one, 2.0f);
		nemoshow_item_set_filter(one, miro->solid);
	}

	for (i = 0; i <= rows; i++) {
		miro->rones[i] = one = nemoshow_item_create(NEMOSHOW_LINE_ITEM);
		nemoshow_attach_one(show, one);
		nemoshow_one_attach(canvas, one);
		nemoshow_item_set_x(one, 0.0f);
		nemoshow_item_set_y(one, i * (height / rows));
		nemoshow_item_set_width(one, 0.0f);
		nemoshow_item_set_height(one, i * (height / rows));
		nemoshow_item_set_stroke_color(one, 0x1e, 0xdc, 0xdc, 0xff);
		nemoshow_item_set_stroke_width(one, 2.0f);
		nemoshow_item_set_filter(one, miro->solid);
	}

	nemomiro_dispatch_show(miro, 1800, 100);

	nemocanvas_dispatch_frame(NEMOSHOW_AT(show, canvas));

	nemotool_run(tool);

err3:
	nemoshow_destroy_canvas(show);

err2:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

err1:
	free(miro);

	return 0;
}
