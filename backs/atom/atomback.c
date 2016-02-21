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
#include <atomback.h>
#include <showhelper.h>
#include <glfilter.h>
#include <nemolog.h>
#include <nemomisc.h>

static void nemoback_atom_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, void *event)
{
	struct atomback *atom = (struct atomback *)nemoshow_get_userdata(show);

	if (nemoshow_event_is_down(show, event)) {
		uint32_t tag;

		tag = nemoshow_canvas_pick_tag(atom->canvasp, nemoshow_event_get_x(event), nemoshow_event_get_y(event));
		if (tag == 1) {
			struct showtransition *trans;
			struct showone *sequence;
			struct showone *set0, *set1;

			set0 = nemoshow_sequence_create_set();
			nemoshow_sequence_set_source(set0, atom->one0);
			nemoshow_sequence_set_fattr_offset(set0, "color", NEMOSHOW_POLY_RED_COLOR, 0.1f, NEMOSHOW_STYLE_DIRTY);
			nemoshow_sequence_set_fattr_offset(set0, "color", NEMOSHOW_POLY_GREEN_COLOR, 0.1f, NEMOSHOW_STYLE_DIRTY);
			nemoshow_sequence_set_fattr_offset(set0, "color", NEMOSHOW_POLY_BLUE_COLOR, 0.1f, NEMOSHOW_STYLE_DIRTY);

			set1 = nemoshow_sequence_create_set();
			nemoshow_sequence_set_source(set1, atom->one0);
			nemoshow_sequence_set_fattr_offset(set1, "color", NEMOSHOW_POLY_RED_COLOR, 1.0f, NEMOSHOW_STYLE_DIRTY);
			nemoshow_sequence_set_fattr_offset(set1, "color", NEMOSHOW_POLY_GREEN_COLOR, 1.0f, NEMOSHOW_STYLE_DIRTY);
			nemoshow_sequence_set_fattr_offset(set1, "color", NEMOSHOW_POLY_BLUE_COLOR, 1.0f, NEMOSHOW_STYLE_DIRTY);

			sequence = nemoshow_sequence_create_easy(atom->show,
					nemoshow_sequence_create_frame_easy(atom->show,
						0.5f, set0, NULL),
					nemoshow_sequence_create_frame_easy(atom->show,
						1.0f, set1, NULL),
					NULL);

			trans = nemoshow_transition_create(NEMOSHOW_CUBIC_INOUT_EASE, 800, 0);
			nemoshow_transition_check_one(trans, atom->one0);
			nemoshow_transition_attach_sequence(trans, sequence);
			nemoshow_attach_transition(atom->show, trans);
		} else if (tag == 2) {
			struct showtransition *trans;
			struct showone *sequence;
			struct showone *set0, *set1;

			set0 = nemoshow_sequence_create_set();
			nemoshow_sequence_set_source(set0, atom->one1);
			nemoshow_sequence_set_fattr_offset(set0, "color", NEMOSHOW_POLY_RED_COLOR, 0.0f, NEMOSHOW_STYLE_DIRTY);
			nemoshow_sequence_set_fattr_offset(set0, "color", NEMOSHOW_POLY_GREEN_COLOR, 0.0f, NEMOSHOW_STYLE_DIRTY);
			nemoshow_sequence_set_fattr_offset(set0, "color", NEMOSHOW_POLY_BLUE_COLOR, 0.0f, NEMOSHOW_STYLE_DIRTY);

			set1 = nemoshow_sequence_create_set();
			nemoshow_sequence_set_source(set1, atom->one1);
			nemoshow_sequence_set_fattr_offset(set1, "color", NEMOSHOW_POLY_RED_COLOR, 1.0f, NEMOSHOW_STYLE_DIRTY);
			nemoshow_sequence_set_fattr_offset(set1, "color", NEMOSHOW_POLY_GREEN_COLOR, 1.0f, NEMOSHOW_STYLE_DIRTY);
			nemoshow_sequence_set_fattr_offset(set1, "color", NEMOSHOW_POLY_BLUE_COLOR, 1.0f, NEMOSHOW_STYLE_DIRTY);

			sequence = nemoshow_sequence_create_easy(atom->show,
					nemoshow_sequence_create_frame_easy(atom->show,
						0.5f, set0, NULL),
					nemoshow_sequence_create_frame_easy(atom->show,
						1.0f, set1, NULL),
					NULL);

			trans = nemoshow_transition_create(NEMOSHOW_CUBIC_INOUT_EASE, 800, 0);
			nemoshow_transition_check_one(trans, atom->one1);
			nemoshow_transition_attach_sequence(trans, sequence);
			nemoshow_attach_transition(atom->show, trans);
		}

		if (tag == 1) {
			struct showone *one;
			float tx, ty;
			int plane;

			if ((plane = nemoshow_poly_pick_one(atom->one0, nemoshow_event_get_x(event), nemoshow_event_get_y(event), &tx, &ty)) > 0) {
				one = nemoshow_item_create(NEMOSHOW_CIRCLE_ITEM);
				nemoshow_one_attach(atom->canvast, one);
				nemoshow_item_set_x(one, tx * nemoshow_canvas_get_width(atom->canvast));
				nemoshow_item_set_y(one, ty * nemoshow_canvas_get_height(atom->canvast));
				nemoshow_item_set_r(one, 5.0f);
				nemoshow_item_set_fill_color(one, 0xff, 0xff, 0xff, 0xff);
			}
		} else if (tag == 2) {
			struct showone *one;
			float tx, ty;
			int plane;

			if ((plane = nemoshow_poly_pick_one(atom->one1, nemoshow_event_get_x(event), nemoshow_event_get_y(event), &tx, &ty)) > 0) {
			}
		}
	}
}

static void nemoback_atom_dispatch_canvas_fullscreen(struct nemoshow *show, int32_t active, int32_t opaque)
{
	struct atomback *atom = (struct atomback *)nemoshow_get_userdata(show);

	if (active == 0)
		atom->is_sleeping = 0;
	else
		atom->is_sleeping = 1;
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "file",						required_argument,			NULL,		'f' },
		{ "shader",					required_argument,			NULL,		's' },
		{ "width",					required_argument,			NULL,		'w' },
		{ "height",					required_argument,			NULL,		'h' },
		{ "alpha",					required_argument,			NULL,		'a' },
		{ "log",						required_argument,			NULL,		'o' },
		{ 0 }
	};

	struct atomback *atom;
	struct nemotool *tool;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct showone *pipe;
	struct showone *one;
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *set0;
	struct showone *set1;
	struct showone *set2;
	struct showone *set3;
	struct nemomatrix matrix;
	char *filepath = NULL;
	char *shaderpath = NULL;
	double alpha = 1.0f;
	int32_t width = 1920;
	int32_t height = 1080;
	int opt;

	nemolog_set_file(2);

	while (opt = getopt_long(argc, argv, "f:s:w:h:o:a:", options, NULL)) {
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

			case 'o':
				nemolog_open_socket(optarg);
				break;

			case 'a':
				alpha = strtod(optarg, NULL);
				break;

			default:
				break;
		}
	}

	if (filepath == NULL)
		return 0;

	atom = (struct atomback *)malloc(sizeof(struct atomback));
	if (atom == NULL)
		return -1;
	memset(atom, 0, sizeof(struct atomback));

	atom->width = width;
	atom->height = height;

	atom->tool = tool = nemotool_create();
	if (tool == NULL)
		goto err1;
	nemotool_connect_wayland(tool, NULL);

	atom->show = show = nemoshow_create_view(tool, width, height);
	if (show == NULL)
		goto err2;
	nemoshow_set_dispatch_fullscreen(show, nemoback_atom_dispatch_canvas_fullscreen);
	nemoshow_set_userdata(show, atom);

	nemoshow_view_set_layer(show, "background");
	nemoshow_view_set_input(show, "touch");
	nemoshow_view_put_sound(show);
	nemoshow_view_set_opaque(show, 0, 0, width, height);

	atom->scene = scene = nemoshow_scene_create();
	nemoshow_scene_set_width(scene, width);
	nemoshow_scene_set_height(scene, height);
	nemoshow_set_scene(show, scene);

	atom->back = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_BACK_TYPE);
	nemoshow_canvas_set_fill_color(canvas, 0.0f, 0.0f, 0.0f, 255.0f);
	nemoshow_canvas_set_alpha(canvas, 0.0f);
	nemoshow_canvas_set_event(canvas, 1);
	nemoshow_one_attach(scene, canvas);

	atom->canvasb = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, 512.0f);
	nemoshow_canvas_set_height(canvas, 512.0f);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
	if (shaderpath != NULL)
		nemoshow_canvas_load_filter(canvas, shaderpath);

	atom->canvast = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, 512.0f);
	nemoshow_canvas_set_height(canvas, 512.0f);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);

	atom->onet = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_one_attach(canvas, one);
	nemoshow_item_set_x(one, 0.0f);
	nemoshow_item_set_y(one, 0.0f);
	nemoshow_item_set_width(one, 512.0f);
	nemoshow_item_set_height(one, 512.0f);
	nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
	nemoshow_item_set_tsr(one);
	nemoshow_item_pivot(one, 512.0f / 2.0f, 512.0f / 2.0f);
	nemoshow_item_load_svg(one, filepath);

	atom->canvasp = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_PIPELINE_TYPE);
	nemoshow_canvas_set_dispatch_event(canvas, nemoback_atom_dispatch_canvas_event);
	nemoshow_canvas_set_alpha(canvas, alpha);
	nemoshow_one_attach(scene, canvas);

	atom->pipe = pipe = nemoshow_pipe_create(NEMOSHOW_LIGHTING_TEXTURE_PIPE);
	nemoshow_one_attach(canvas, pipe);
	nemoshow_pipe_set_light(pipe, 1.0f, 1.0f, -1.0f, 1.0f);
	nemoshow_pipe_set_aspect_ratio(pipe,
			nemoshow_canvas_get_aspect_ratio(atom->canvasp));

	atom->one0 = one = nemoshow_poly_create(NEMOSHOW_QUAD_POLY);
	nemoshow_one_attach(pipe, one);
	nemoshow_one_set_tag(one, 1);
	nemoshow_poly_set_color(one, 1.0f, 1.0f, 1.0f, 1.0f);
	nemoshow_poly_set_canvas(one, atom->canvasb);
	nemoshow_poly_use_texcoords(one, 1);
	nemoshow_poly_use_normals(one, 1);
	nemoshow_poly_use_vbo(one, 1);
	nemoshow_poly_set_vertex(one, 0, -1.0f, -1.0f, 1.0f);
	nemoshow_poly_set_vertex(one, 1, 1.0f, -1.0f, 1.0f);
	nemoshow_poly_set_vertex(one, 2, 1.0f, 1.0f, 1.0f);
	nemoshow_poly_set_vertex(one, 3, -1.0f, 1.0f, 1.0f);
	nemoshow_poly_set_texcoord(one, 0, 0.0f, 1.0f);
	nemoshow_poly_set_texcoord(one, 1, 1.0f, 1.0f);
	nemoshow_poly_set_texcoord(one, 2, 1.0f, 0.0f);
	nemoshow_poly_set_texcoord(one, 3, 0.0f, 0.0f);

	nemomatrix_init_identity(&matrix);
	nemomatrix_scale_xyz(&matrix, 1.0f / nemoshow_canvas_get_aspect_ratio(atom->canvasp), 1.0f, 1.0f);
	nemoshow_poly_transform_vertices(one, &matrix);

	atom->one1 = one = nemoshow_poly_create(NEMOSHOW_CUBE_POLY);
	nemoshow_one_attach(pipe, one);
	nemoshow_one_set_tag(one, 2);
	nemoshow_poly_set_canvas(one, atom->canvast);
	nemoshow_poly_set_color(one, 1.0f, 1.0f, 1.0f, 1.0f);
	nemoshow_poly_use_texcoords(one, 1);
	nemoshow_poly_use_normals(one, 1);
	nemoshow_poly_use_vbo(one, 1);

	nemomatrix_init_identity(&matrix);
	nemomatrix_scale_xyz(&matrix, 0.5f, 0.5f, 0.5f);
	nemoshow_poly_transform_vertices(one, &matrix);

	trans = nemoshow_transition_create(NEMOSHOW_LINEAR_EASE, 18000, 0);
	nemoshow_transition_dirty_one(trans, atom->canvasb, NEMOSHOW_FILTER_DIRTY);
	nemoshow_transition_set_repeat(trans, 0);
	nemoshow_attach_transition(atom->show, trans);

	set0 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set0, atom->one1);
	nemoshow_sequence_set_dattr(set0, "rx", 360.0f, NEMOSHOW_MATRIX_DIRTY);
	nemoshow_sequence_set_dattr(set0, "ry", 360.0f, NEMOSHOW_MATRIX_DIRTY);

	set1 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set1, atom->one1);
	nemoshow_sequence_set_dattr(set1, "rx", 0.0f, NEMOSHOW_MATRIX_DIRTY);
	nemoshow_sequence_set_dattr(set1, "ry", 0.0f, NEMOSHOW_MATRIX_DIRTY);

	set2 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set2, atom->onet);
	nemoshow_sequence_set_cattr(set2, "fill", 0xff, 0x8c, 0x32, 0xff, NEMOSHOW_STYLE_DIRTY);

	set3 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set3, atom->onet);
	nemoshow_sequence_set_cattr(set3, "fill", 0x1e, 0xdc, 0xdc, 0xff, NEMOSHOW_STYLE_DIRTY);

	sequence = nemoshow_sequence_create_easy(atom->show,
			nemoshow_sequence_create_frame_easy(atom->show,
				0.5f, set0, set2, NULL),
			nemoshow_sequence_create_frame_easy(atom->show,
				1.0f, set1, set3, NULL),
			NULL);

	trans = nemoshow_transition_create(NEMOSHOW_CUBIC_INOUT_EASE, 18000, 0);
	nemoshow_transition_check_one(trans, atom->one1);
	nemoshow_transition_check_one(trans, atom->onet);
	nemoshow_transition_set_repeat(trans, 0);
	nemoshow_transition_attach_sequence(trans, sequence);
	nemoshow_attach_transition(atom->show, trans);

	set0 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set0, atom->pipe);
	nemoshow_sequence_set_fattr_offset(set0, "light", 0, -1.0f, NEMOSHOW_REDRAW_DIRTY);
	nemoshow_sequence_set_fattr_offset(set0, "light", 1, 1.0f, NEMOSHOW_REDRAW_DIRTY);
	nemoshow_sequence_set_fattr_offset(set0, "light", 2, -1.0f, NEMOSHOW_REDRAW_DIRTY);
	nemoshow_sequence_set_fattr_offset(set0, "light", 3, 1.0f, NEMOSHOW_REDRAW_DIRTY);

	set1 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set1, atom->pipe);
	nemoshow_sequence_set_fattr_offset(set1, "light", 0, 1.0f, NEMOSHOW_REDRAW_DIRTY);
	nemoshow_sequence_set_fattr_offset(set1, "light", 1, 1.0f, NEMOSHOW_REDRAW_DIRTY);
	nemoshow_sequence_set_fattr_offset(set1, "light", 2, -1.0f, NEMOSHOW_REDRAW_DIRTY);
	nemoshow_sequence_set_fattr_offset(set1, "light", 3, 1.0f, NEMOSHOW_REDRAW_DIRTY);

	sequence = nemoshow_sequence_create_easy(atom->show,
			nemoshow_sequence_create_frame_easy(atom->show,
				0.5f, set0, NULL),
			nemoshow_sequence_create_frame_easy(atom->show,
				1.0f, set1, NULL),
			NULL);

	trans = nemoshow_transition_create(NEMOSHOW_CUBIC_INOUT_EASE, 12000, 0);
	nemoshow_transition_check_one(trans, atom->pipe);
	nemoshow_transition_set_repeat(trans, 0);
	nemoshow_transition_attach_sequence(trans, sequence);
	nemoshow_attach_transition(atom->show, trans);

	nemoshow_dispatch_frame(show);

	nemotool_run(tool);

	nemoshow_destroy_view(show);

err2:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

err1:
	free(atom);

	return 0;
}
