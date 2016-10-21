#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <nemoshow.h>
#include <showhelper.h>
#include <skiahelper.hpp>
#include <glshader.h>
#include <fbohelper.h>
#include <fxsph.h>
#include <nemohelper.h>
#include <nemolist.h>
#include <nemolog.h>
#include <nemomisc.h>

struct fluidcontext {
	struct nemotool *tool;

	int width, height;
	int is_fullscreen;

	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;

	struct showone *canvas;
	struct showone *sprite;

	struct fxsph *sph;
};

static void nemofluid_dispatch_canvas_redraw(struct nemoshow *show, struct showone *one)
{
	static const char *vertexshader =
		"attribute vec3 position;\n"
		"void main()\n"
		"{\n"
		"  gl_Position = vec4(position.xy, 0.0, 1.0);\n"
		"  gl_PointSize = position.z;\n"
		"}\n";

	static const char *fragmentshader =
		"precision mediump float;\n"
		"uniform sampler2D tex;\n"
		"void main()\n"
		"{\n"
		"  gl_FragColor = texture2D(tex, gl_PointCoord);\n"
		"}\n";

	struct fluidcontext *context = (struct fluidcontext *)nemoshow_get_userdata(show);
	uint32_t particles = nemofx_sph_get_particles(context->sph);
	GLuint width = nemoshow_canvas_get_viewport_width(one);
	GLuint height = nemoshow_canvas_get_viewport_height(one);
	GLuint fbo, dbo;
	GLuint program;
	GLuint frag, vert;
	GLfloat vertices[particles * 3];
	int i;

	nemofx_sph_build_cell(context->sph);
	nemofx_sph_compute_density_pressure(context->sph);
	nemofx_sph_compute_force(context->sph);
	nemofx_sph_advect(context->sph, 0.002f);

	for (i = 0; i < nemofx_sph_get_particles(context->sph); i++) {
		vertices[i * 3 + 0] = nemofx_sph_get_particle_x(context->sph, i) / 2.56f * 2.0f - 1.0f;
		vertices[i * 3 + 1] = nemofx_sph_get_particle_y(context->sph, i) / 2.56f * 2.0f - 1.0f;
		vertices[i * 3 + 2] = width * 0.04f;
	}

	fbo_prepare_context(
			nemoshow_canvas_get_texture(one),
			width, height,
			&fbo, &dbo);

	NEMO_CHECK((frag = glshader_compile(GL_FRAGMENT_SHADER, 1, &fragmentshader)) == GL_NONE, "failed to compile shader\n");
	NEMO_CHECK((vert = glshader_compile(GL_VERTEX_SHADER, 1, &vertexshader)) == GL_NONE, "failed to compile shader\n");

	program = glCreateProgram();
	glAttachShader(program, frag);
	glAttachShader(program, vert);
	glLinkProgram(program);
	glUseProgram(program);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_POINT_SMOOTH);
	glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

	glViewport(0, 0, width, height);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUniform1i(glGetUniformLocation(program, "tex"), 0);

	glBindAttribLocation(program, 0, "position");

	glBindTexture(GL_TEXTURE_2D, nemoshow_canvas_get_texture(context->sprite));

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), &vertices[0]);
	glEnableVertexAttribArray(0);

	glDrawArrays(GL_POINTS, 0, particles);

	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDeleteProgram(program);

	glDeleteFramebuffers(1, &fbo);
	glDeleteRenderbuffers(1, &dbo);
}

static void nemofluid_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, struct showevent *event)
{
	struct fluidcontext *context = (struct fluidcontext *)nemoshow_get_userdata(show);
	float x = nemoshow_event_get_x(event) * nemoshow_canvas_get_viewport_sx(canvas);
	float y = nemoshow_event_get_y(event) * nemoshow_canvas_get_viewport_sy(canvas);

	nemoshow_event_update_taps(show, canvas, event);

	if (context->is_fullscreen == 0) {
		if (nemoshow_event_is_touch_down(show, event) || nemoshow_event_is_touch_up(show, event)) {
			if (nemoshow_event_is_more_taps(show, event, 3)) {
				nemoshow_view_pick_distant(show, event, NEMOSHOW_VIEW_PICK_ROTATE_TYPE | NEMOSHOW_VIEW_PICK_TRANSLATE_TYPE);

				nemoshow_event_set_cancel(event);

				nemoshow_dispatch_grab_all(show, event);
			}
		}
	}
}

static void nemofluid_dispatch_show_resize(struct nemoshow *show, int32_t width, int32_t height)
{
	struct fluidcontext *context = (struct fluidcontext *)nemoshow_get_userdata(show);

	nemoshow_view_resize(context->show, width, height);

	context->width = width;
	context->height = height;

	nemoshow_view_redraw(context->show);
}

static void nemofluid_dispatch_show_fullscreen(struct nemoshow *show, const char *id, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct fluidcontext *context = (struct fluidcontext *)nemoshow_get_userdata(show);

	if (id == NULL)
		context->is_fullscreen = 0;
	else
		context->is_fullscreen = 1;
}

static int nemofluid_prepare_sph(struct fluidcontext *context)
{
	float x, y;
	int index = 0;

	context->sph = nemofx_sph_create(1800, 2.56f, 2.56f, 0.04f, 0.02f);

	for (y = 2.56f * 0.3f; y <= 2.56f * 0.7f; y += 0.04f * 0.6f) {
		for (x = 2.56f * 0.3f; x <= 2.56f * 0.7f; x += 0.04f * 0.6f) {
			nemofx_sph_set_particle(context->sph, index++, x, y, 0.0f, 0.0f);

			if (index >= 1800)
				goto out;
		}
	}

out:
	return 0;
}

static void nemofluid_finish_sph(struct fluidcontext *context)
{
	nemofx_sph_destroy(context->sph);
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "fullscreen",			required_argument,			NULL,		'f' },
		{ 0 }
	};

	struct fluidcontext *context;
	struct nemotool *tool;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct showone *one;
	struct showone *blur;
	struct showtransition *trans;
	char *fullscreen = NULL;
	int width = 800;
	int height = 800;
	int opt;

	opterr = 0;

	while (opt = getopt_long(argc, argv, "f:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'f':
				fullscreen = strdup(optarg);
				break;

			default:
				break;
		}
	}

	context = (struct fluidcontext *)malloc(sizeof(struct fluidcontext));
	if (context == NULL)
		goto err1;
	memset(context, 0, sizeof(struct fluidcontext));

	context->width = width;
	context->height = height;

	context->tool = tool = nemotool_create();
	if (tool == NULL)
		goto err2;
	nemotool_connect_wayland(tool, NULL);

	context->show = show = nemoshow_create_view(tool, width, height);
	if (show == NULL)
		goto err3;
	nemoshow_set_dispatch_resize(show, nemofluid_dispatch_show_resize);
	nemoshow_set_dispatch_fullscreen(show, nemofluid_dispatch_show_fullscreen);
	nemoshow_set_userdata(show, context);

	if (fullscreen != NULL)
		nemoshow_view_set_fullscreen(show, fullscreen);

	context->scene = scene = nemoshow_scene_create();
	nemoshow_scene_set_width(scene, width);
	nemoshow_scene_set_height(scene, height);
	nemoshow_set_scene(show, scene);

	context->back = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_BACK_TYPE);
	nemoshow_canvas_set_fill_color(canvas, 0.0f, 0.0f, 0.0f, 1.0f);
	nemoshow_one_attach(scene, canvas);

	context->canvas = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_OPENGL_TYPE);
	nemoshow_canvas_set_dispatch_redraw(canvas, nemofluid_dispatch_canvas_redraw);
	nemoshow_canvas_set_dispatch_event(canvas, nemofluid_dispatch_canvas_event);
	nemoshow_one_attach(scene, canvas);

	context->sprite = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, 64);
	nemoshow_canvas_set_height(canvas, 64);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
	nemoshow_attach_one(show, canvas);

	blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_filter_set_blur(blur, "solid", 8.0f);

	one = nemoshow_item_create(NEMOSHOW_CIRCLE_ITEM);
	nemoshow_one_attach(canvas, one);
	nemoshow_item_set_cx(one, 64.0f / 2.0f);
	nemoshow_item_set_cy(one, 64.0f / 2.0f);
	nemoshow_item_set_r(one, 64.0f / 3.0f);
	nemoshow_item_set_fill_color(one, 255.0f, 255.0f, 255.0f, 255.0f);
	nemoshow_item_set_filter(one, blur);

	nemofluid_prepare_sph(context);

	trans = nemoshow_transition_create(NEMOSHOW_LINEAR_EASE, 18000, 0);
	nemoshow_transition_dirty_one(trans, context->canvas, NEMOSHOW_REDRAW_DIRTY);
	nemoshow_transition_set_repeat(trans, 0);
	nemoshow_attach_transition(show, trans);

	nemoshow_dispatch_frame(show);

	nemotool_run(tool);

	nemofluid_finish_sph(context);

	nemoshow_destroy_view(show);

err3:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

err2:
	free(context);

err1:
	return 0;
}
