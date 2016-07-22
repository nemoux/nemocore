#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>
#include <ctype.h>

#include <nemoshow.h>
#include <showhelper.h>
#include <fbohelper.h>
#include <glhelper.h>
#include <nemohelper.h>
#include <nemolog.h>
#include <nemomisc.h>

struct motecontext {
	struct nemotool *tool;

	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;

	struct showone *view;
};

static void nemomote_dispatch_show_resize(struct nemoshow *show, int32_t width, int32_t height)
{
	struct motecontext *context = (struct motecontext *)nemoshow_get_userdata(show);

	nemoshow_view_resize(context->show, width, height);
}

static void nemomote_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, void *event)
{
	struct motecontext *context = (struct motecontext *)nemoshow_get_userdata(show);

	if (nemoshow_event_is_touch_down(show, event) || nemoshow_event_is_touch_up(show, event)) {
		nemoshow_event_update_taps(show, canvas, event);

		if (nemoshow_event_is_more_taps(show, event, 3)) {
			nemoshow_view_pick_distant(show, event, NEMOSHOW_VIEW_PICK_ALL_TYPE);

			nemoshow_event_set_cancel(event);

			nemoshow_dispatch_grab_all(show, event);
		}
	}
}

static void nemomote_prepare_compute(struct motecontext *context)
{
	static const char *vertexshader =
		"attribute vec2 position;\n"
		"attribute vec2 texcoord;\n"
		"varying vec2 vtexcoord;\n"
		"void main()\n"
		"{\n"
		"  gl_Position = vec4(position, 0.0, 1.0);\n"
		"  vtexcoord = texcoord;\n"
		"}\n";

	static const char *fragmentshader =
		"precision mediump float;\n"
		"varying vec2 vtexcoord;\n"
		"uniform sampler2D tex;\n"
		"void main()\n"
		"{\n"
		"  gl_FragColor = texture2D(tex, vtexcoord);\n"
		"  gl_FragColor.a = 1.0;\n"
		"}\n";

	static const char *computeshader =
		"#version 310 es\n"
		"uniform float roll;\n"
		"layout(rgba32f, binding = 0) writeonly uniform mediump image2D tex0;\n"
		"layout(local_size_x = 16, local_size_y = 16) in;\n"
		"void main() {\n"
		"  ivec2 pos = ivec2(gl_GlobalInvocationID.xy);\n"
		"  float lcoef = length(vec2(ivec2(gl_LocalInvocationID.xy) - 8) / 8.0);\n"
		"  float gcoef = sin(float(gl_WorkGroupID.x + gl_WorkGroupID.y) * 0.1 + roll) * 0.5;\n"
		"  imageStore(tex0, pos, vec4(1.0 - gcoef * lcoef, 1.0, 1.0, 1.0));\n"
		"}\n";

	GLfloat vertices[] = {
		-1.0f, -1.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 1.0f
	};

	GLuint fbo, dbo;
	GLuint program;
	GLuint frag, vert;
	GLuint comp;
	GLuint texture;

	fbo_prepare_context(
			nemoshow_canvas_get_texture(context->view),
			nemoshow_canvas_get_width(context->view),
			nemoshow_canvas_get_height(context->view),
			&fbo, &dbo);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, 0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, nemoshow_canvas_get_width(context->view));
	glTexImage2D(GL_TEXTURE_2D, 0,
			GL_RGBA32F,
			nemoshow_canvas_get_width(context->view),
			nemoshow_canvas_get_height(context->view),
			0, GL_RGBA, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glViewport(0, 0,
			nemoshow_canvas_get_width(context->view),
			nemoshow_canvas_get_height(context->view));

	glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	NEMO_CHECK((comp = glshader_compile(GL_COMPUTE_SHADER, 1, &computeshader)) == GL_NONE, "failed to compile shader\n");

	program = glCreateProgram();
	glAttachShader(program, comp);
	glLinkProgram(program);

	glUseProgram(program);

	glUniform1i(glGetUniformLocation(program, "tex0"), 0);
	glUniform1f(glGetUniformLocation(program, "roll"), (float)0.1f);

	glBindTexture(GL_TEXTURE_2D, texture);

	glBindImageTexture(0,
			texture,
			0,
			GL_FALSE,
			0,
			GL_WRITE_ONLY,
			GL_RGBA32F);

	glDispatchCompute(16, 16, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	NEMO_CHECK((frag = glshader_compile(GL_FRAGMENT_SHADER, 1, &fragmentshader)) == GL_NONE, "failed to compile shader\n");
	NEMO_CHECK((vert = glshader_compile(GL_VERTEX_SHADER, 1, &vertexshader)) == GL_NONE, "failed to compile shader\n");

	program = glCreateProgram();
	glAttachShader(program, frag);
	glAttachShader(program, vert);
	glLinkProgram(program);

	glUseProgram(program);

	glBindTexture(GL_TEXTURE_2D, texture);

	glBindAttribLocation(program, 0, "position");
	glBindAttribLocation(program, 1, "texcoord");

	glUniform1i(glGetUniformLocation(program, "tex"), 0);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[2]);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void nemomote_finish_compute(struct motecontext *context)
{
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "file",				required_argument,			NULL,			'f' },
		{ 0 }
	};

	struct motecontext *context;
	struct nemotool *tool;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct showone *one;
	char *file = NULL;
	int width = 640;
	int height = 480;
	int opt;

	opterr = 0;

	while (opt = getopt_long(argc, argv, "f:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'f':
				file = strdup(optarg);
				break;

			default:
				break;
		}
	}

	context = (struct motecontext *)malloc(sizeof(struct motecontext));
	if (context == NULL)
		goto err1;
	memset(context, 0, sizeof(struct motecontext));

	context->tool = tool = nemotool_create();
	if (tool == NULL)
		goto err2;
	nemotool_connect_wayland(tool, NULL);

	context->show = show = nemoshow_create_view(tool, width, height);
	if (show == NULL)
		goto err3;
	nemoshow_set_dispatch_resize(show, nemomote_dispatch_show_resize);
	nemoshow_set_userdata(show, context);

	context->scene = scene = nemoshow_scene_create();
	nemoshow_scene_set_width(scene, width);
	nemoshow_scene_set_height(scene, height);
	nemoshow_set_scene(show, scene);

	context->back = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_BACK_TYPE);
	nemoshow_canvas_set_fill_color(canvas, 1.0f, 1.0f, 1.0f, 1.0f);
	nemoshow_one_attach(scene, canvas);

	context->view = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_OPENGL_TYPE);
	nemoshow_canvas_set_dispatch_event(canvas, nemomote_dispatch_canvas_event);
	nemoshow_one_attach(scene, canvas);

	nemomote_prepare_compute(context);

	nemoshow_dispatch_frame(show);

	nemotool_run(tool);

	nemomote_finish_compute(context);

	nemoshow_destroy_view(show);

err3:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

err2:
	free(context);

err1:
	return 0;
}
