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

	nemomote_tween_update(mote->mote, 5, secs, &mote->ease0, 6, NEMOMOTE_POSITION_TWEEN | NEMOMOTE_COLOR_TWEEN | NEMOMOTE_MASS_TWEEN);
	nemomote_sleeptime_set(mote->mote, 6, 5.0f, 4.5f);
	nemomote_type_set(mote->mote, 6, 7);

	nemomote_sleep_update(mote->mote, 7, secs, 8);
	nemomote_tweener_set_color(mote->mote, 8,
			mote->colors1[0], mote->colors0[0],
			mote->colors1[1], mote->colors0[1],
			mote->colors1[2], mote->colors0[2],
			mote->colors1[3], mote->colors0[3]);
	nemomote_tweener_set_mass(mote->mote, 8,
			mote->pixelsize, mote->pixelsize * 0.0f);
	nemomote_tweener_set_duration(mote->mote, 8,
			3.0f, 1.0f);
	nemomote_explosion_update(mote->mote, 8, secs, mote->speedmax, -mote->speedmax, mote->speedmax, -mote->speedmax);
	nemomote_type_set(mote->mote, 8, 9);

	nemomote_tween_update(mote->mote, 9, secs, &mote->ease1, 1, NEMOMOTE_COLOR_TWEEN | NEMOMOTE_MASS_TWEEN);
	nemomote_boundingbox_update(mote->mote, 9, secs, &mote->box, 0.8f);
	nemomote_move_update(mote->mote, 9, secs);

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

		nemoshow_poly_set_diffuse(mote->mesh, i * 6 + 0,
				NEMOMOTE_COLOR_R(mote->mote, i),
				NEMOMOTE_COLOR_G(mote->mote, i),
				NEMOMOTE_COLOR_B(mote->mote, i),
				NEMOMOTE_COLOR_A(mote->mote, i));
		nemoshow_poly_set_diffuse(mote->mesh, i * 6 + 1,
				NEMOMOTE_COLOR_R(mote->mote, i),
				NEMOMOTE_COLOR_G(mote->mote, i),
				NEMOMOTE_COLOR_B(mote->mote, i),
				NEMOMOTE_COLOR_A(mote->mote, i));
		nemoshow_poly_set_diffuse(mote->mesh, i * 6 + 2,
				NEMOMOTE_COLOR_R(mote->mote, i),
				NEMOMOTE_COLOR_G(mote->mote, i),
				NEMOMOTE_COLOR_B(mote->mote, i),
				NEMOMOTE_COLOR_A(mote->mote, i));
		nemoshow_poly_set_diffuse(mote->mesh, i * 6 + 3,
				NEMOMOTE_COLOR_R(mote->mote, i),
				NEMOMOTE_COLOR_G(mote->mote, i),
				NEMOMOTE_COLOR_B(mote->mote, i),
				NEMOMOTE_COLOR_A(mote->mote, i));
		nemoshow_poly_set_diffuse(mote->mesh, i * 6 + 4,
				NEMOMOTE_COLOR_R(mote->mote, i),
				NEMOMOTE_COLOR_G(mote->mote, i),
				NEMOMOTE_COLOR_B(mote->mote, i),
				NEMOMOTE_COLOR_A(mote->mote, i));
		nemoshow_poly_set_diffuse(mote->mesh, i * 6 + 5,
				NEMOMOTE_COLOR_R(mote->mote, i),
				NEMOMOTE_COLOR_G(mote->mote, i),
				NEMOMOTE_COLOR_B(mote->mote, i),
				NEMOMOTE_COLOR_A(mote->mote, i));
	}

	nemoshow_canvas_redraw_one(show, one);
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

					nemomote_tweener_set_one_position(mote->mote, p,
							x + j * mote->textsize,
							y + i * mote->textsize);
					nemomote_tweener_set_one_color(mote->mote, p,
							mote->tcolors1[0], mote->tcolors0[0],
							mote->tcolors1[1], mote->tcolors0[1],
							mote->tcolors1[2], mote->tcolors0[2],
							mote->tcolors1[3], mote->tcolors0[3]);
					nemomote_tweener_set_one_mass(mote->mote, p,
							mote->textsize * 0.8f, mote->textsize * 0.5f);
					nemomote_tweener_set_one_duration(mote->mote, p,
							3.0f, 1.0f);
					nemomote_type_set_one(mote->mote, p, 5);
				}
			}
		}

		mote->type = (mote->type + 1) % 2;
	}

	nemotimer_set_timeout(mote->timer, 30 * 1000);
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
		{ "alpha",					required_argument,			NULL,		'a' },
		{ "framerate",			required_argument,			NULL,		'r' },
		{ 0 }
	};

	struct moteback *mote;
	struct nemotool *tool;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct showone *pipe;
	struct showone *one;
	struct showone *filter;
	struct showtransition *trans;
	struct nemomatrix matrix;
	char *filepath = NULL;
	char *shaderpath = NULL;
	char *logo = NULL;
	int32_t width = 1920;
	int32_t height = 1080;
	double pixelsize = 8.0f;
	double speedmax = 300.0f;
	double mutualgravity = 2500.0f;
	double color0[4] = { 0.0f, 0.5f, 0.5f, 0.3f };
	double color1[4] = { 0.0f, 0.5f, 0.5f, 0.7f };
	double textcolor[4] = { 0.0f, 1.0f, 1.0f, 1.0f };
	double alpha = 1.0f;
	uint32_t color;
	int pixelcount = 1800;
	int framerate = 30;
	int opt;
	int i;

	while (opt = getopt_long(argc, argv, "f:s:w:h:l:p:c:e:g:n:m:t:a:r:", options, NULL)) {
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

			case 'a':
				alpha = strtod(optarg, NULL);
				break;

			case 'r':
				framerate = strtoul(optarg, NULL, 10);
				break;

			default:
				break;
		}
	}

	if (filepath == NULL || pixelcount <= 0)
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
			mote->pixelsize * 0.0f);
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
			mote->pixelsize,
			mote->pixelsize * 0.5f);
	nemomote_type_update(mote->mote, 2);
	nemomote_commit(mote->mote);

	nemoease_set(&mote->ease0, NEMOEASE_CUBIC_INOUT_TYPE);
	nemoease_set(&mote->ease1, NEMOEASE_CUBIC_INOUT_TYPE);

	mote->secs = (double)time_current_nsecs() / 1000000000;

	mote->tool = tool = nemotool_create();
	if (tool == NULL)
		goto err1;
	nemotool_connect_wayland(tool, NULL);

	mote->timer = nemotimer_create(tool);
	nemotimer_set_callback(mote->timer, nemoback_mote_dispatch_timer_event);
	nemotimer_set_userdata(mote->timer, mote);
	nemotimer_set_timeout(mote->timer, 5000);

	mote->show = show = nemoshow_create_view(tool, width, height);
	if (show == NULL)
		goto err2;
	nemoshow_set_userdata(show, mote);

	nemoshow_view_set_layer(show, "background");
	nemoshow_view_put_state(show, "keypad");
	nemoshow_view_put_state(show, "sound");
	nemoshow_view_set_opaque(show, 0, 0, width, height);

	mote->scene = scene = nemoshow_scene_create();
	nemoshow_scene_set_width(scene, width);
	nemoshow_scene_set_height(scene, height);
	nemoshow_set_scene(show, scene);

	mote->back = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_BACK_TYPE);
	nemoshow_canvas_set_fill_color(canvas, 0.0f, 0.0f, 0.0f, 255.0f);
	nemoshow_canvas_set_alpha(canvas, 0.0f);
	nemoshow_one_attach(scene, canvas);

	mote->canvasb = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
	if (shaderpath != NULL)
		nemoshow_canvas_load_filter(canvas, shaderpath);

	if (filepath != NULL) {
		one = nemoshow_item_create(NEMOSHOW_IMAGE_ITEM);
		nemoshow_one_attach(canvas, one);
		nemoshow_item_set_x(one, 0.0f);
		nemoshow_item_set_y(one, 0.0f);
		nemoshow_item_set_width(one, width);
		nemoshow_item_set_height(one, height);
		nemoshow_item_set_uri(one, filepath);
	}

	mote->canvast = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, pixelsize * 2.0f);
	nemoshow_canvas_set_height(canvas, pixelsize * 2.0f);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);

	filter = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_filter_set_blur(filter, "high", "solid", pixelsize * 0.2f);
	nemoshow_filter_update(filter);

	one = nemoshow_item_create(NEMOSHOW_CIRCLE_ITEM);
	nemoshow_one_attach(canvas, one);
	nemoshow_item_set_cx(one, pixelsize);
	nemoshow_item_set_cy(one, pixelsize);
	nemoshow_item_set_r(one, pixelsize * 0.8f);
	nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
	nemoshow_item_set_filter(one, filter);

	mote->canvasp = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_PIPELINE_TYPE);
	nemoshow_canvas_set_dispatch_redraw(canvas, nemoback_mote_dispatch_pipeline_canvas_redraw);
	nemoshow_canvas_set_alpha(canvas, alpha);
	nemoshow_one_attach(scene, canvas);

	mote->pipe = pipe = nemoshow_pipe_create(NEMOSHOW_LIGHTING_DIFFUSE_TEXTURE_PIPE);
	nemoshow_one_attach(canvas, pipe);
	nemoshow_pipe_set_light(pipe, 1.0f, 1.0f, -1.0f, 1.0f);
	nemoshow_pipe_set_aspect_ratio(pipe,
			nemoshow_canvas_get_aspect_ratio(mote->canvasp));

	mote->quad = one = nemoshow_poly_create(NEMOSHOW_QUAD_POLY);
	nemoshow_one_attach(pipe, one);
	nemoshow_one_set_tag(one, 1);
	nemoshow_one_set_state(one, NEMOSHOW_PICK_STATE);
	nemoshow_poly_set_color(one, 1.0f, 1.0f, 1.0f, 1.0f);
	nemoshow_poly_set_canvas(one, mote->canvasb);
	nemoshow_poly_use_texcoords(one, 1);
	nemoshow_poly_use_normals(one, 1);
	nemoshow_poly_use_diffuses(one, 1);
	nemoshow_poly_use_vbo(one, 1);
	nemoshow_poly_set_diffuse(one, 0, 1.0f, 1.0f, 1.0f, 1.0f);
	nemoshow_poly_set_diffuse(one, 1, 1.0f, 1.0f, 1.0f, 1.0f);
	nemoshow_poly_set_diffuse(one, 2, 1.0f, 1.0f, 1.0f, 1.0f);
	nemoshow_poly_set_diffuse(one, 3, 1.0f, 1.0f, 1.0f, 1.0f);

	nemomatrix_init_identity(&matrix);
	nemomatrix_scale_xyz(&matrix, 1.0f / nemoshow_canvas_get_aspect_ratio(mote->canvasp), 1.0f, 1.0f);
	nemoshow_poly_transform_vertices(one, &matrix);

	mote->mesh = one = nemoshow_poly_create(NEMOSHOW_MESH_POLY);
	nemoshow_one_attach(pipe, one);
	nemoshow_poly_set_color(one, 1.0f, 1.0f, 1.0f, 1.0f);
	nemoshow_poly_set_canvas(one, mote->canvast);
	nemoshow_poly_set_vertices(one, (float *)malloc(sizeof(float[3]) * pixelcount * 6), pixelcount * 6);
	nemoshow_poly_use_texcoords(one, 1);
	nemoshow_poly_use_diffuses(one, 1);
	nemoshow_poly_use_normals(one, 1);
	nemoshow_poly_use_vbo(one, 1);

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

	if (shaderpath != NULL) {
		trans = nemoshow_transition_create(NEMOSHOW_LINEAR_EASE, 18000, 0);
		nemoshow_transition_dirty_one(trans, mote->canvasb, NEMOSHOW_FILTER_DIRTY);
		nemoshow_transition_set_repeat(trans, 0);
		nemoshow_transition_set_framerate(trans, framerate);
		nemoshow_attach_transition(mote->show, trans);
	}

	trans = nemoshow_transition_create(NEMOSHOW_LINEAR_EASE, 18000, 0);
	nemoshow_transition_dirty_one(trans, mote->canvasp, NEMOSHOW_REDRAW_DIRTY);
	nemoshow_transition_set_repeat(trans, 0);
	nemoshow_transition_set_framerate(trans, framerate);
	nemoshow_attach_transition(mote->show, trans);

	nemoshow_dispatch_frame(show);

	nemotool_run(tool);

	nemoshow_destroy_view(show);

err2:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

err1:
	free(mote);

	return 0;
}
