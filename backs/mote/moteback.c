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
#include <nemomote.h>
#include <showhelper.h>
#include <colorhelper.h>
#include <glfilter.h>
#include <skiahelper.h>
#include <nemolog.h>
#include <nemomisc.h>

static void nemoback_mote_dispatch_pipeline_canvas_redraw(struct nemoshow *show, struct showone *one)
{
	struct moteback *mote = (struct moteback *)nemoshow_get_userdata(show);
	double secs0 = (double)time_current_nsecs() / 1000000000;
	double secs = secs0 - mote->secs;
	int i;

	nemomote_mutualgravity_update(mote->mote, 1, secs, 100.0f, mote->mutualgravity, 50.0f);
	nemomote_speedlimit_update(mote->mote, 2, secs, 0.0f, mote->speedmax * 0.2f);
	nemomote_collide_update(mote->mote, 2, 1, secs, 1.5f);
	nemomote_collide_update(mote->mote, 2, 2, secs, 1.5f);
	nemomote_speedlimit_update(mote->mote, 1, secs, 0.0f, mote->speedmax);

	if (nemomote_tween_update(mote->mote, 5, secs, &mote->ease, 6, NEMOMOTE_POSITION_TWEEN | NEMOMOTE_COLOR_TWEEN | NEMOMOTE_MASS_TWEEN) != 0) {
		nemomote_tweener_set(mote->mote, 6,
				0.0f, 0.0f,
				mote->colors1, mote->colors0,
				mote->pixelsize, mote->pixelsize * 0.5f,
				5.0f, 1.0f);
	}

	if (nemomote_tween_update(mote->mote, 6, secs, &mote->ease, 7, NEMOMOTE_COLOR_TWEEN | NEMOMOTE_MASS_TWEEN) != 0) {
		nemomote_explosion_update(mote->mote, 7, secs, -30.0f, 30.0f, -30.0f, 30.0f);
		nemomote_sleeptime_set(mote->mote, 7, 9.0f, 3.0f);
		nemomote_type_set(mote->mote, 7, 1);
	}

	nemomote_boundingbox_update(mote->mote, 2, secs, &mote->box, 0.8f);
	nemomote_move_update(mote->mote, 2, secs);
	nemomote_boundingbox_update(mote->mote, 1, secs, &mote->box, 0.8f);
	nemomote_move_update(mote->mote, 1, secs);

	nemomote_cleanup(mote->mote);

	mote->secs = secs0;

	for (i = 0; i < nemomote_get_count(mote->mote); i++) {
		double x = (NEMOMOTE_POSITION_X(mote->mote, i) / (double)mote->width * 2.0f - 1.0f) / mote->ratio;
		double y = (1.0f - NEMOMOTE_POSITION_Y(mote->mote, i) / (double)mote->height * 2.0f);
		double m = (NEMOMOTE_MASS(mote->mote, i) / (double)mote->height * 2.0f);
		double x0 = x - m;
		double y0 = y - m;
		double x1 = x + m;
		double y1 = y + m;

		nemoshow_poly_set_vertex(mote->mesh, i * 6 + 0, x0, y0, 0.0f);
		nemoshow_poly_set_vertex(mote->mesh, i * 6 + 1, x0, y1, 0.0f);
		nemoshow_poly_set_vertex(mote->mesh, i * 6 + 2, x1, y1, 0.0f);
		nemoshow_poly_set_vertex(mote->mesh, i * 6 + 3, x0, y0, 0.0f);
		nemoshow_poly_set_vertex(mote->mesh, i * 6 + 4, x1, y1, 0.0f);
		nemoshow_poly_set_vertex(mote->mesh, i * 6 + 5, x1, y0, 0.0f);
	}

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

static void nemoback_mote_dispatch_timer_event(struct nemotimer *timer, void *data)
{
	struct moteback *mote = (struct moteback *)data;

	if (mote->is_sleeping == 0) {
		char msg[64];
		int width = mote->fontsize * sizeof(msg);
		int height = mote->fontsize;
		uint32_t pixels[width * height];
		double x, y;
		int textwidth;
		int i, j, p;

		if (mote->type == 0) {
			time_t tt;
			struct tm *tm;

			time(&tt);
			tm = localtime(&tt);
			strftime(msg, sizeof(msg), "%m:%d-%I:%M", tm);
		} else {
			strcpy(msg, mote->logo);
		}

		skia_clear_canvas((void *)pixels, width, height);
		textwidth = skia_draw_text((void *)pixels, width, height, mote->font, mote->fontsize, msg, 0.0f, 0.0f, 0xffffffff);

		x = random_get_double(0.0f, mote->width - textwidth * mote->textsize);
		y = random_get_double(0.0f, mote->height - height * mote->textsize);

		for (i = 0; i < height; i++) {
			for (j = 0; j < width; j++) {
				if (pixels[i * width + j] != 0x0) {
					p = nemomote_get_one_by_type(mote->mote, 1);
					if (p < 0)
						return;

					nemomote_tweener_set_one(mote->mote, p,
							x + j * mote->textsize,
							y + i * mote->textsize,
							mote->tcolors1, mote->tcolors0,
							mote->textsize * 0.5f, mote->textsize * 0.3f,
							5.0f, 1.0f);
					nemomote_type_set_one(mote->mote, p, 5);
				}
			}
		}

		mote->type = (mote->type + 1) % 2;
	}

	nemotimer_set_timeout(mote->timer, 30 * 1000);
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
	double pixelsize = 8.0f;
	double speedmax = 500.0f;
	double mutualgravity = 3000.0f;
	double color0[4] = { 0.0f, 0.5f, 0.5f, 0.3f };
	double color1[4] = { 0.0f, 0.5f, 0.5f, 0.7f };
	double textcolor[4] = { 0.0f, 1.0f, 1.0f, 1.0f };
	uint32_t color;
	int pixelcount = 1800;
	int opt;
	int i;

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

	if (logo != NULL)
		mote->logo = logo;
	else
		mote->logo = strdup("NEMO-UX");

	mote->font = strdup("/usr/share/fonts/ttf/LiberationMono-Regular.ttf");
	mote->fontsize = 32;
	mote->textsize = 8.0f;
	mote->pixelsize = pixelsize;
	mote->speedmax = speedmax;
	mote->mutualgravity = mutualgravity;

	mote->colors0[0] = color0[0];
	mote->colors1[0] = color1[0];
	mote->colors0[1] = color0[1];
	mote->colors1[1] = color1[1];
	mote->colors0[2] = color0[2];
	mote->colors1[2] = color1[2];
	mote->colors0[3] = color0[3];
	mote->colors1[3] = color1[3];

	mote->tcolors0[0] = textcolor[0];
	mote->tcolors1[0] = textcolor[0];
	mote->tcolors0[1] = textcolor[1];
	mote->tcolors1[1] = textcolor[1];
	mote->tcolors0[2] = textcolor[2];
	mote->tcolors1[2] = textcolor[2];
	mote->tcolors0[3] = textcolor[3];
	mote->tcolors1[3] = textcolor[3];

	mote->mote = nemomote_create(pixelcount);
	nemomote_random_set_property(&mote->random, 5.0f, 1.0f);
	nemozone_set_cube(&mote->box, mote->width * 0.0f, mote->width * 1.0f, mote->height * 0.0f, mote->height * 1.0f);
	nemozone_set_disc(&mote->disc, mote->width * 0.5f, mote->height * 0.5f, mote->width * 0.2f);
	nemozone_set_disc(&mote->speed, 0.0f, 0.0f, 50.0f);

	nemomote_blast_emit(mote->mote, pixelcount * 0.92f);
	nemomote_position_update(mote->mote, &mote->box);
	nemomote_velocity_update(mote->mote, &mote->speed);
	nemomote_color_update(mote->mote,
			mote->colors1[0], mote->colors0[0],
			mote->colors1[1], mote->colors0[1],
			mote->colors1[2], mote->colors0[2],
			mote->colors1[3], mote->colors0[3]);
	nemomote_mass_update(mote->mote,
			mote->pixelsize,
			mote->pixelsize * 0.5f);
	nemomote_type_update(mote->mote, 1);
	nemomote_commit(mote->mote);

	nemomote_blast_emit(mote->mote, pixelcount * 0.05f);
	nemomote_position_update(mote->mote, &mote->box);
	nemomote_velocity_update(mote->mote, &mote->speed);
	nemomote_color_update(mote->mote,
			mote->colors1[0], mote->colors0[0],
			mote->colors1[1], mote->colors0[1],
			mote->colors1[2], mote->colors0[2],
			mote->colors1[3], mote->colors1[3]);
	nemomote_mass_update(mote->mote,
			mote->pixelsize * 1.5f,
			mote->pixelsize);
	nemomote_type_update(mote->mote, 2);
	nemomote_commit(mote->mote);

	nemoease_set(&mote->ease, NEMOEASE_CUBIC_INOUT_TYPE);

	mote->secs = (double)time_current_nsecs() / 1000000000;

	mote->tool = tool = nemotool_create();
	if (tool == NULL)
		goto err1;
	nemotool_connect_wayland(tool, NULL);

	mote->timer = nemotimer_create(tool);
	nemotimer_set_callback(mote->timer, nemoback_mote_dispatch_timer_event);
	nemotimer_set_userdata(mote->timer, mote);
	nemotimer_set_timeout(mote->timer, 5000);

	mote->show = show = nemoshow_create_canvas(tool, width, height, nemoback_mote_dispatch_tale_event);
	if (show == NULL)
		goto err2;
	nemoshow_set_userdata(show, mote);

	nemocanvas_opaque(NEMOSHOW_AT(show, canvas), 0, 0, width, height);
	nemocanvas_set_layer(NEMOSHOW_AT(show, canvas), NEMO_SURFACE_LAYER_TYPE_BACKGROUND);
	nemocanvas_set_input_type(NEMOSHOW_AT(show, canvas), NEMO_SURFACE_INPUT_TYPE_TOUCH);
	nemocanvas_set_dispatch_fullscreen(NEMOSHOW_AT(show, canvas), nemoback_mote_dispatch_canvas_fullscreen);

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
	nemoshow_filter_set_blur(blur, "high", "solid", 16.0f);

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
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
	if (shaderpath != NULL)
		nemoshow_canvas_load_filter(canvas, shaderpath);
	nemoshow_attach_one(show, canvas);

	if (filepath != NULL) {
		one = nemoshow_item_create(NEMOSHOW_IMAGE_ITEM);
		nemoshow_attach_one(show, one);
		nemoshow_one_attach(canvas, one);
		nemoshow_item_set_x(one, 0.0f);
		nemoshow_item_set_y(one, 0.0f);
		nemoshow_item_set_width(one, width);
		nemoshow_item_set_height(one, height);
		nemoshow_item_set_uri(one, filepath);
	}

	mote->canvast = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, 128.0f);
	nemoshow_canvas_set_height(canvas, 128.0f);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
	nemoshow_attach_one(show, canvas);

	one = nemoshow_item_create(NEMOSHOW_CIRCLE_ITEM);
	nemoshow_attach_one(show, one);
	nemoshow_one_attach(canvas, one);
	nemoshow_item_set_x(one, 64.0f);
	nemoshow_item_set_y(one, 64.0f);
	nemoshow_item_set_r(one, 48.0f);
	nemoshow_item_set_stroke_color(one, 0x1e, 0xdc, 0xdc, 0x40);
	nemoshow_item_set_stroke_width(one, 12.0f);
	nemoshow_item_set_filter(one, mote->solid);

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

	mote->quad = one = nemoshow_poly_create(NEMOSHOW_QUAD_POLY);
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

	mote->mesh = one = nemoshow_poly_create(NEMOSHOW_MESH_POLY);
	nemoshow_attach_one(show, one);
	nemoshow_one_attach(pipe, one);
	nemoshow_poly_set_color(one, 1.0f, 1.0f, 1.0f, 1.0f);
	nemoshow_poly_set_canvas(one, mote->canvast);
	nemoshow_poly_set_vertices(one, (float *)malloc(sizeof(float[3]) * pixelcount * 6), pixelcount * 6);
	nemoshow_poly_use_texcoords(one, 1);
	nemoshow_poly_use_normals(one, 1);

	for (i = 0; i < pixelcount; i++) {
		nemoshow_poly_set_texcoord(one, i * 6 + 0, 0.0f, 1.0f);
		nemoshow_poly_set_texcoord(one, i * 6 + 1, 0.0f, 0.0f);
		nemoshow_poly_set_texcoord(one, i * 6 + 2, 1.0f, 0.0f);
		nemoshow_poly_set_texcoord(one, i * 6 + 3, 0.0f, 1.0f);
		nemoshow_poly_set_texcoord(one, i * 6 + 4, 1.0f, 0.0f);
		nemoshow_poly_set_texcoord(one, i * 6 + 5, 1.0f, 1.0f);

		nemoshow_poly_set_normal(one, i * 6 + 0, 0.0f, 0.0f, -1.0f);
		nemoshow_poly_set_normal(one, i * 6 + 1, 0.0f, 0.0f, -1.0f);
		nemoshow_poly_set_normal(one, i * 6 + 2, 0.0f, 0.0f, -1.0f);
		nemoshow_poly_set_normal(one, i * 6 + 3, 0.0f, 0.0f, -1.0f);
		nemoshow_poly_set_normal(one, i * 6 + 4, 0.0f, 0.0f, -1.0f);
		nemoshow_poly_set_normal(one, i * 6 + 5, 0.0f, 0.0f, -1.0f);
	}

	mote->ratio = nemoshow_canvas_get_aspect_ratio(mote->canvasp);

	nemoshow_set_scene(show, scene);
	nemoshow_set_size(show, width, height);

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
