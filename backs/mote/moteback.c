#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <time.h>
#include <getopt.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoegl.h>
#include <moteback.h>
#include <showhelper.h>
#include <colorhelper.h>
#include <glfilter.h>
#include <nemolog.h>
#include <nemomisc.h>

static void nemoback_mote_dispatch_pipeline_canvas_redraw(struct nemoshow *show, struct showone *one)
{
	nemoshow_canvas_redraw_one(show, one);

	nemoshow_canvas_damage_all(one);
	nemoshow_dispatch_feedback(show);
}

static void nemoback_mote_dispatch_tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
	uint32_t id = nemotale_node_get_id(node);

	if (id == 1) {
		if (nemotale_dispatch_grab(tale, event->device, type, event) == 0) {
			struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);
			struct moteback *mote = (struct moteback *)nemoshow_get_userdata(show);

			if (nemotale_is_touch_down(tale, event, type)) {
				uint32_t tag;

				tag = nemoshow_canvas_pick_tag(mote->canvasp, event->x, event->y);
				if (tag == 1) {
				}
			}
		}
	}
}

static void nemoback_mote_dispatch_canvas_fullscreen(struct nemocanvas *canvas, int32_t active, int32_t opaque)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);
	struct moteback *mote = (struct moteback *)nemoshow_get_userdata(show);

	if (active == 0)
		mote->is_sleeping = 0;
	else
		mote->is_sleeping = 1;
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "file",						required_argument,			NULL,		'f' },
		{ "shader",					required_argument,			NULL,		's' },
		{ "width",					required_argument,			NULL,		'w' },
		{ "height",					required_argument,			NULL,		'h' },
		{ "logo",						required_argument,			NULL,		'l' },
		{ "pixelsize",			required_argument,			NULL,		'p' },
		{ "pixelcount",			required_argument,			NULL,		'c' },
		{ "speedmax",				required_argument,			NULL,		'e' },
		{ "mutualgravity",	required_argument,			NULL,		'g' },
		{ "mincolor",				required_argument,			NULL,		'n' },
		{ "maxcolor",				required_argument,			NULL,		'm' },
		{ "textcolor",			required_argument,			NULL,		't' },
		{ 0 }
	};

	struct moteback *mote;
	struct nemotool *tool;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct showone *blur;
	struct showone *ease;
	struct showone *pipe;
	struct showone *one;
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *set0;
	struct showone *set1;
	struct nemomatrix matrix;
	char *filepath = NULL;
	char *shaderpath = NULL;
	char *logo = NULL;
	int32_t width = 1920;
	int32_t height = 1080;
	double textsize = 18.0f;
	double pixelsize = 8.0f;
	double speedmax = 300.0f;
	double mutualgravity = 5000.0f;
	double color0[4] = { 0.0f, 0.5f, 0.5f, 0.1f };
	double color1[4] = { 0.0f, 0.5f, 0.5f, 0.3f };
	double textcolor[4] = { 0.0f, 1.0f, 1.0f, 1.0f };
	uint32_t color;
	int pixelcount = 500;
	int opt;

	nemolog_set_file(2);

	while (opt = getopt_long(argc, argv, "f:s:w:h:l:p:c:e:g:n:m:t:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'f':
				filepath = strdup(optarg);
				break;

			case 's':
				shaderpath = strdup(optarg);
				break;

			case 'w':
				width = strtoul(optarg, NULL, 10);
				break;

			case 'h':
				height = strtoul(optarg, NULL, 10);
				break;

			case 'l':
				logo = strdup(optarg);
				break;

			case 'p':
				pixelsize = strtod(optarg, NULL);
				break;

			case 'c':
				pixelcount = strtoul(optarg, NULL, 10);
				break;

			case 'e':
				speedmax = strtod(optarg, NULL);
				break;

			case 'g':
				mutualgravity = strtod(optarg, NULL);
				break;

			case 'n':
				color = color_parse(optarg);

				color0[0] = COLOR_DOUBLE_R(color);
				color0[1] = COLOR_DOUBLE_G(color);
				color0[2] = COLOR_DOUBLE_B(color);
				color0[3] = COLOR_DOUBLE_A(color);
				break;

			case 'm':
				color = color_parse(optarg);

				color1[0] = COLOR_DOUBLE_R(color);
				color1[1] = COLOR_DOUBLE_G(color);
				color1[2] = COLOR_DOUBLE_B(color);
				color1[3] = COLOR_DOUBLE_A(color);
				break;

			case 't':
				color = color_parse(optarg);

				textcolor[0] = COLOR_DOUBLE_R(color);
				textcolor[1] = COLOR_DOUBLE_G(color);
				textcolor[2] = COLOR_DOUBLE_B(color);
				textcolor[3] = COLOR_DOUBLE_A(color);
				break;

			default:
				break;
		}
	}

	if (filepath == NULL)
		return 0;

	mote = (struct moteback *)malloc(sizeof(struct moteback));
	if (mote == NULL)
		return -1;
	memset(mote, 0, sizeof(struct moteback));

	mote->width = width;
	mote->height = height;

	mote->tool = tool = nemotool_create();
	if (tool == NULL)
		goto err1;
	nemotool_connect_wayland(tool, NULL);

	mote->show = show = nemoshow_create_canvas(tool, width, height, nemoback_mote_dispatch_tale_event);
	if (show == NULL)
		goto err2;
	nemoshow_set_userdata(show, mote);

	nemocanvas_opaque(NEMOSHOW_AT(show, canvas), 0, 0, width, height);
	nemocanvas_set_layer(NEMOSHOW_AT(show, canvas), NEMO_SURFACE_LAYER_TYPE_BACKGROUND);
	nemocanvas_set_input_type(NEMOSHOW_AT(show, canvas), NEMO_SURFACE_INPUT_TYPE_TOUCH);
	nemocanvas_set_dispatch_fullscreen(NEMOSHOW_AT(show, canvas), nemoback_mote_dispatch_canvas_fullscreen);

	mote->scene = scene = nemoshow_scene_create();
	nemoshow_scene_set_width(scene, width);
	nemoshow_scene_set_height(scene, height);
	nemoshow_attach_one(show, scene);

	mote->back = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_BACK_TYPE);
	nemoshow_canvas_set_fill_color(canvas, 0.0f, 0.0f, 0.0f, 255.0f);
	nemoshow_canvas_set_alpha(canvas, 0.0f);
	nemoshow_canvas_set_event(canvas, 1);
	nemoshow_attach_one(show, canvas);
	nemoshow_one_attach(scene, canvas);

	mote->canvasb = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, 512.0f);
	nemoshow_canvas_set_height(canvas, 512.0f);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
	if (shaderpath != NULL)
		nemoshow_canvas_load_filter(canvas, shaderpath);
	nemoshow_attach_one(show, canvas);

	mote->canvast = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, 512.0f);
	nemoshow_canvas_set_height(canvas, 512.0f);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
	nemoshow_attach_one(show, canvas);

	mote->onet = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_attach_one(show, one);
	nemoshow_one_attach(canvas, one);
	nemoshow_item_set_x(one, 0.0f);
	nemoshow_item_set_y(one, 0.0f);
	nemoshow_item_set_width(one, 512.0f);
	nemoshow_item_set_height(one, 512.0f);
	nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
	nemoshow_item_set_tsr(one);
	nemoshow_item_pivot(one, 512.0f / 2.0f, 512.0f / 2.0f);
	nemoshow_item_load_svg(one, filepath);

	mote->canvasp = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_PIPELINE_TYPE);
	nemoshow_canvas_set_event(canvas, 1);
	nemoshow_canvas_set_dispatch_redraw(canvas, nemoback_mote_dispatch_pipeline_canvas_redraw);
	nemoshow_attach_one(show, canvas);
	nemoshow_one_attach(scene, canvas);

	mote->pipe = pipe = nemoshow_pipe_create(NEMOSHOW_LIGHTING_TEXTURE_PIPE);
	nemoshow_attach_one(show, pipe);
	nemoshow_one_attach(canvas, pipe);
	nemoshow_pipe_set_light(pipe, 1.0f, 1.0f, -1.0f, 1.0f);
	nemoshow_pipe_set_aspect_ratio(pipe,
			nemoshow_canvas_get_aspect_ratio(mote->canvasp));

	mote->one = one = nemoshow_poly_create(NEMOSHOW_QUAD_POLY);
	nemoshow_attach_one(show, one);
	nemoshow_one_attach(pipe, one);
	nemoshow_one_set_tag(one, 1);
	nemoshow_poly_set_color(one, 1.0f, 1.0f, 1.0f, 1.0f);
	nemoshow_poly_set_canvas(one, mote->canvasb);
	nemoshow_poly_use_texcoords(one, 1);
	nemoshow_poly_use_normals(one, 1);
	nemoshow_poly_use_vbo(one, 1);

	nemomatrix_init_identity(&matrix);
	nemomatrix_scale_xyz(&matrix, 1.0f / nemoshow_canvas_get_aspect_ratio(mote->canvasp), 1.0f, 1.0f);
	nemoshow_poly_transform_vertices(one, &matrix);

	nemoshow_set_scene(show, scene);
	nemoshow_set_size(show, width, height);

	mote->ease0 = ease = nemoshow_ease_create();
	nemoshow_ease_set_type(ease, NEMOEASE_CUBIC_INOUT_TYPE);

	mote->ease1 = ease = nemoshow_ease_create();
	nemoshow_ease_set_type(ease, NEMOEASE_CUBIC_OUT_TYPE);

	mote->ease2 = ease = nemoshow_ease_create();
	nemoshow_ease_set_type(ease, NEMOEASE_LINEAR_TYPE);

	mote->inner = blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_attach_one(show, blur);
	nemoshow_filter_set_blur(blur, "high", "inner", 3.0f);

	mote->outer = blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_attach_one(show, blur);
	nemoshow_filter_set_blur(blur, "high", "outer", 3.0f);

	mote->solid = blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_attach_one(show, blur);
	nemoshow_filter_set_blur(blur, "high", "solid", 5.0f);

	trans = nemoshow_transition_create(mote->ease2, 18000, 0);
	nemoshow_transition_dirty_one(trans, mote->canvasb, NEMOSHOW_FILTER_DIRTY);
	nemoshow_transition_set_repeat(trans, 0);
	nemoshow_attach_transition(mote->show, trans);

	set0 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set0, mote->pipe);
	nemoshow_sequence_set_fattr_offset(set0, "light", 0, -1.0f, NEMOSHOW_REDRAW_DIRTY);
	nemoshow_sequence_set_fattr_offset(set0, "light", 1, 1.0f, NEMOSHOW_REDRAW_DIRTY);
	nemoshow_sequence_set_fattr_offset(set0, "light", 2, -1.0f, NEMOSHOW_REDRAW_DIRTY);
	nemoshow_sequence_set_fattr_offset(set0, "light", 3, 1.0f, NEMOSHOW_REDRAW_DIRTY);

	set1 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set1, mote->pipe);
	nemoshow_sequence_set_fattr_offset(set1, "light", 0, 1.0f, NEMOSHOW_REDRAW_DIRTY);
	nemoshow_sequence_set_fattr_offset(set1, "light", 1, 1.0f, NEMOSHOW_REDRAW_DIRTY);
	nemoshow_sequence_set_fattr_offset(set1, "light", 2, -1.0f, NEMOSHOW_REDRAW_DIRTY);
	nemoshow_sequence_set_fattr_offset(set1, "light", 3, 1.0f, NEMOSHOW_REDRAW_DIRTY);

	sequence = nemoshow_sequence_create_easy(mote->show,
			nemoshow_sequence_create_frame_easy(mote->show,
				0.5f, set0, NULL),
			nemoshow_sequence_create_frame_easy(mote->show,
				1.0f, set1, NULL),
			NULL);

	trans = nemoshow_transition_create(mote->ease0, 12000, 0);
	nemoshow_transition_check_one(trans, mote->pipe);
	nemoshow_transition_set_repeat(trans, 0);
	nemoshow_transition_attach_sequence(trans, sequence);
	nemoshow_attach_transition(mote->show, trans);

	nemoshow_dispatch_frame(show);

	nemotool_run(tool);

err2:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

err1:
	free(mote);

	return 0;
}
