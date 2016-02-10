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
#include <atommisc.h>
#include <showhelper.h>
#include <nemolog.h>
#include <nemomisc.h>

static void nemoback_atom_dispatch_show_render_canvas(struct nemoshow *show, struct showone *one)
{
	struct atomback *atom = (struct atomback *)nemoshow_get_userdata(show);
	GLfloat color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

	glBindFramebuffer(GL_FRAMEBUFFER, atom->fbo);

	glViewport(0, 0,
			nemoshow_canvas_get_viewport_width(atom->canvas0),
			nemoshow_canvas_get_viewport_height(atom->canvas0));

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	nemoshow_pipe_dispatch(one, atom->pipe);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void nemoback_atom_dispatch_show_resize_canvas(struct nemoshow *show, struct showone *one, int32_t width, int32_t height)
{
	struct atomback *atom = (struct atomback *)nemoshow_get_userdata(show);

	fbo_prepare_context(
			nemoshow_canvas_get_texture(atom->canvas0),
			width, height,
			&atom->fbo, &atom->dbo);
}

static void nemoback_atom_dispatch_tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
	uint32_t id = nemotale_node_get_id(node);

	if (id == 1) {
		if (nemotale_dispatch_grab(tale, event->device, type, event) == 0) {
			struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);
			struct atomback *atom = (struct atomback *)nemoshow_get_userdata(show);
		}
	}
}

static void nemoback_atom_dispatch_canvas_fullscreen(struct nemocanvas *canvas, int32_t active, int32_t opaque)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);
	struct atomback *atom = (struct atomback *)nemoshow_get_userdata(show);

	if (active == 0)
		atom->is_sleeping = 0;
	else
		atom->is_sleeping = 1;
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "width",					required_argument,			NULL,		'w' },
		{ "height",					required_argument,			NULL,		'h' },
		{ "log",						required_argument,			NULL,		'l' },
		{ 0 }
	};

	struct atomback *atom;
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
	int32_t width = 1920;
	int32_t height = 1080;
	int opt;

	nemolog_set_file(2);

	while (opt = getopt_long(argc, argv, "w:h:l:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'w':
				width = strtoul(optarg, NULL, 10);
				break;

			case 'h':
				height = strtoul(optarg, NULL, 10);
				break;

			case 'l':
				nemolog_open_socket(optarg);
				break;

			default:
				break;
		}
	}

	atom = (struct atomback *)malloc(sizeof(struct atomback));
	if (atom == NULL)
		return -1;
	memset(atom, 0, sizeof(struct atomback));

	atom->width = width;
	atom->height = height;
	atom->aspect = (double)height / (double)width;

	nemomatrix_init_identity(&atom->matrix);

	atom->tool = tool = nemotool_create();
	if (tool == NULL)
		goto err1;
	nemotool_connect_wayland(tool, NULL);

	atom->show = show = nemoshow_create_canvas(tool, width, height, nemoback_atom_dispatch_tale_event);
	if (show == NULL)
		goto err2;
	nemoshow_set_userdata(show, atom);

	nemocanvas_opaque(NEMOSHOW_AT(show, canvas), 0, 0, width, height);
	nemocanvas_set_layer(NEMOSHOW_AT(show, canvas), NEMO_SURFACE_LAYER_TYPE_BACKGROUND);
	nemocanvas_set_dispatch_fullscreen(NEMOSHOW_AT(show, canvas), nemoback_atom_dispatch_canvas_fullscreen);

	atom->scene = scene = nemoshow_scene_create();
	nemoshow_scene_set_width(scene, width);
	nemoshow_scene_set_height(scene, height);
	nemoshow_attach_one(show, scene);

	atom->back = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_BACK_TYPE);
	nemoshow_canvas_set_fill_color(canvas, 0.0f, 0.0f, 0.0f, 255.0f);
	nemoshow_canvas_set_alpha(canvas, 0.0f);
	nemoshow_canvas_set_event(canvas, 1);
	nemoshow_attach_one(show, canvas);
	nemoshow_one_attach(scene, canvas);

	atom->canvas0 = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_OPENGL_TYPE);
	nemoshow_canvas_set_dispatch_render(canvas, nemoback_atom_dispatch_show_render_canvas);
	nemoshow_canvas_set_dispatch_resize(canvas, nemoback_atom_dispatch_show_resize_canvas);
	nemoshow_attach_one(show, canvas);
	nemoshow_one_attach(scene, canvas);

	atom->pipe = pipe = nemoshow_pipe_create(NEMOSHOW_SIMPLE_PIPE);
	nemoshow_attach_one(show, pipe);
	nemoshow_one_attach(canvas, pipe);

	atom->one = one = nemoshow_poly_create(NEMOSHOW_QUAD_POLY);
	nemoshow_attach_one(show, one);
	nemoshow_one_attach(pipe, one);
	nemoshow_poly_set_vertex(one, 0, -0.0f, -0.0f, 0.0f);
	nemoshow_poly_set_vertex(one, 1, 0.5f, -0.5f, 0.0f);
	nemoshow_poly_set_vertex(one, 2, 0.5f, 0.5f, 0.0f);
	nemoshow_poly_set_vertex(one, 3, -0.5f, 0.5f, 0.0f);
	nemoshow_poly_set_color(one, 0.0f, 0.0f, 0.0f, 1.0f);

	atom->canvas1 = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
	nemoshow_attach_one(show, canvas);
	nemoshow_one_attach(atom->canvas0, canvas);

	one = nemoshow_item_create(NEMOSHOW_RRECT_ITEM);
	nemoshow_attach_one(atom->show, one);
	nemoshow_one_attach(atom->canvas1, one);
	nemoshow_item_set_x(one, 0.0f);
	nemoshow_item_set_y(one, 0.0f);
	nemoshow_item_set_width(one, width);
	nemoshow_item_set_height(one, height);
	nemoshow_item_set_rx(one, 10.0f);
	nemoshow_item_set_ry(one, 10.0f);
	nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);

	nemoshow_set_scene(show, scene);
	nemoshow_set_size(show, width, height);

	atom->ease0 = ease = nemoshow_ease_create();
	nemoshow_ease_set_type(ease, NEMOEASE_CUBIC_INOUT_TYPE);

	atom->ease1 = ease = nemoshow_ease_create();
	nemoshow_ease_set_type(ease, NEMOEASE_CUBIC_OUT_TYPE);

	atom->ease2 = ease = nemoshow_ease_create();
	nemoshow_ease_set_type(ease, NEMOEASE_LINEAR_TYPE);

	atom->inner = blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_attach_one(show, blur);
	nemoshow_filter_set_blur(blur, "high", "inner", 3.0f);

	atom->outer = blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_attach_one(show, blur);
	nemoshow_filter_set_blur(blur, "high", "outer", 3.0f);

	atom->solid = blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_attach_one(show, blur);
	nemoshow_filter_set_blur(blur, "high", "solid", 5.0f);

	set0 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set0, atom->one);
	nemoshow_sequence_set_fattr_offset(set0, "color", NEMOSHOW_POLY_RED_COLOR, 1.0f, NEMOSHOW_STYLE_DIRTY);
	nemoshow_sequence_set_fattr_offset(set0, "color", NEMOSHOW_POLY_GREEN_COLOR, 1.0f, NEMOSHOW_STYLE_DIRTY);
	nemoshow_sequence_set_fattr_offset(set0, "color", NEMOSHOW_POLY_BLUE_COLOR, 1.0f, NEMOSHOW_STYLE_DIRTY);
	nemoshow_sequence_set_fattr_offset(set0, "vertex", NEMOSHOW_POLY_X_OFFSET(atom->one, 0), -0.5f, NEMOSHOW_SHAPE_DIRTY);
	nemoshow_sequence_set_fattr_offset(set0, "vertex", NEMOSHOW_POLY_Y_OFFSET(atom->one, 0), -0.5f, NEMOSHOW_SHAPE_DIRTY);

	sequence = nemoshow_sequence_create_easy(atom->show,
			nemoshow_sequence_create_frame_easy(atom->show,
				1.0f, set0, NULL),
			NULL);

	trans = nemoshow_transition_create(atom->ease1, 1800, 0);
	nemoshow_transition_check_one(trans, atom->one);
	nemoshow_transition_attach_sequence(trans, sequence);
	nemoshow_attach_transition(atom->show, trans);

	nemoshow_dispatch_frame(show);

	nemotool_run(tool);

err2:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

err1:
	free(atom);

	return 0;
}
