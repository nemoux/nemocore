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

	glUseProgram(atom->program);

	glBindFramebuffer(GL_FRAMEBUFFER, atom->fbo);

	glViewport(0, 0,
			nemoshow_canvas_get_viewport_width(atom->canvas0),
			nemoshow_canvas_get_viewport_height(atom->canvas0));

	glClearColor(0.0f, 0.3f, 0.0f, 0.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUniformMatrix4fv(atom->umatrix, 1, GL_FALSE, (GLfloat *)atom->matrix.d);
	glUniform4fv(atom->ucolor, 1, color);

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
	struct showone *one;
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
	nemomatrix_scale_xyz(&atom->matrix, atom->aspect, -1.0f, atom->aspect * -1.0f);

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

	atom->canvas1 = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
	nemoshow_canvas_set_event(canvas, 1);
	nemoshow_attach_one(show, canvas);
	nemoshow_one_attach(scene, canvas);

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

	atom->program = nemoback_atom_create_shader(simple_fragment_shader, simple_vertex_shader);
	nemoback_atom_prepare_shader(atom, atom->program);

	nemoshow_dispatch_frame(show);

	nemotool_run(tool);

err2:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

err1:
	free(atom);

	return 0;
}
