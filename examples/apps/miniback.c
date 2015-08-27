#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <miniback.h>

#include <nemotool.h>
#include <nemoegl.h>
#include <talehelper.h>
#include <pixmanhelper.h>
#include <nemomisc.h>

static GLuint miniback_create_shader(void)
{
	static const char *vert_shader_text =
		"uniform mat4 projection;\n"
		"attribute vec2 position;\n"
		"attribute vec2 texcoord;\n"
		"varying vec2 v_texcoord;\n"
		"uniform vec4 color;\n"
		"varying vec4 v_color;\n"
		"void main() {\n"
		"  gl_Position = projection * vec4(position, 0.0, 1.0);\n"
		"  v_texcoord = texcoord;\n"
		"  v_color = color;\n"
		"}\n";
	static const char *frag_shader_text =
		"precision mediump float;\n"
		"uniform sampler2D tex0;\n"
		"varying vec2 v_texcoord;\n"
		"varying vec4 v_color;\n"
		"void main() {\n"
		"  gl_FragColor = texture2D(tex0, v_texcoord) * v_color;\n"
		"}\n";
	GLuint frag, vert;
	GLuint program;
	GLint status;

	frag = glshader_compile(GL_FRAGMENT_SHADER, 1, &frag_shader_text);
	vert = glshader_compile(GL_VERTEX_SHADER, 1, &vert_shader_text);

	program = glCreateProgram();
	glAttachShader(program, frag);
	glAttachShader(program, vert);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (!status) {
		char log[1000];
		GLsizei len;
		glGetProgramInfoLog(program, 1000, &len, log);
		fprintf(stderr, "Error: linking:\n%*s\n", len, log);
		exit(1);
	}

	glUseProgram(program);

	glBindAttribLocation(program, 0, "position");
	glBindAttribLocation(program, 1, "texcoord");
	glLinkProgram(program);

	return program;
}

static void miniback_prepare(struct miniback *mini)
{
	GLfloat vertices[16] = {
		1.0f, -1.0f, 1.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 0.0f,
	};
	pixman_image_t *image;

	mini->program = miniback_create_shader();

	mini->utex0 = glGetUniformLocation(mini->program, "tex0");
	mini->uprojection = glGetUniformLocation(mini->program, "projection");
	mini->ucolor = glGetUniformLocation(mini->program, "color");

	fbo_prepare_context(
			nemotale_node_get_texture(mini->node),
			mini->width, mini->height,
			&mini->fbo, &mini->dbo);

	nemomatrix_init_identity(&mini->matrix);
	nemomatrix_scale(&mini->matrix, 0.5f, -0.5f);

	glGenVertexArrays(1, &mini->varray);
	glBindVertexArray(mini->varray);

	glGenBuffers(1, &mini->vbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mini->vbuffer);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *)sizeof(GLfloat[2]));
	glEnableVertexAttribArray(1);

	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 16, vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glActiveTexture(GL_TEXTURE0);

	glGenTextures(1, &mini->texture);
	glBindTexture(GL_TEXTURE_2D, mini->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	image = pixman_load_png_file("/home/root/.config/nemo.png");
	glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT,
			pixman_image_get_width(image),
			pixman_image_get_height(image),
			0, GL_BGRA_EXT, GL_UNSIGNED_BYTE,
			pixman_image_get_data(image));
	pixman_image_unref(image);
}

static void miniback_finish(struct miniback *mini)
{
	glDeleteBuffers(1, &mini->vbuffer);
	glDeleteVertexArrays(1, &mini->varray);

	glDeleteFramebuffers(1, &mini->fbo);
	glDeleteRenderbuffers(1, &mini->dbo);
}

static void miniback_render(struct miniback *mini)
{
	GLfloat rgba[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

	glUseProgram(mini->program);

	glBindFramebuffer(GL_FRAMEBUFFER, mini->fbo);

	glViewport(0, 0, mini->width, mini->height);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindTexture(GL_TEXTURE_2D, mini->texture);

	glUniform1i(mini->utex0, 0);
	glUniformMatrix4fv(mini->uprojection, 1, GL_FALSE, (GLfloat *)mini->matrix.d);
	glUniform4fv(mini->ucolor, 1, rgba);

	glBindVertexArray(mini->varray);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glBindVertexArray(0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
}

static void miniback_dispatch_tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
	uint32_t id = nemotale_node_get_id(node);
}

static void miniback_dispatch_canvas_resize(struct nemocanvas *canvas, int32_t width, int32_t height)
{
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "type",				required_argument,			NULL,		't' },
		{ 0 }
	};
	struct miniback *mini;
	struct nemotool *tool;
	struct eglcontext *egl;
	struct eglcanvas *canvas;
	struct nemotale *tale;
	struct talenode *node;
	const char *type = NULL;
	int32_t width = 3840;
	int32_t height = 1080;
	int opt;

	while (opt = getopt_long(argc, argv, "t:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 't':
				type = strdup(optarg);
				break;

			default:
				break;
		}
	}

	mini = (struct miniback *)malloc(sizeof(struct miniback));
	if (mini == NULL)
		return -1;
	memset(mini, 0, sizeof(struct miniback));

	mini->width = width;
	mini->height = height;

	mini->tool = tool = nemotool_create();
	if (tool == NULL)
		return -1;
	nemotool_connect_wayland(tool, NULL);

	mini->egl = egl = nemotool_create_egl(tool);

	mini->eglcanvas = canvas = nemotool_create_egl_canvas(egl, width, height);
	nemocanvas_set_nemosurface(NTEGL_CANVAS(canvas), NEMO_SHELL_SURFACE_TYPE_NORMAL);
	nemocanvas_set_dispatch_resize(NTEGL_CANVAS(canvas), miniback_dispatch_canvas_resize);

	mini->canvas = NTEGL_CANVAS(canvas);

	mini->tale = tale = nemotale_create_gl();
	nemotale_set_backend(tale,
			nemotale_create_egl(
				NTEGL_DISPLAY(egl),
				NTEGL_CONTEXT(egl),
				NTEGL_CONFIG(egl),
				(EGLNativeWindowType)NTEGL_WINDOW(canvas)));
	nemotale_resize(tale, width, height);

	nemotale_attach_canvas(tale, NTEGL_CANVAS(canvas), miniback_dispatch_tale_event);
	nemotale_set_userdata(tale, mini);

	mini->node = node = nemotale_node_create_gl(width, height);
	nemotale_node_set_id(node, 1);
	nemotale_attach_node(tale, node);

	miniback_prepare(mini);
	miniback_render(mini);

	nemotale_node_damage_all(node);

	nemotale_composite_egl(tale, NULL);

	nemotool_run(tool);

	miniback_finish(mini);

	nemotale_destroy_gl(tale);

	nemotool_destroy_egl_canvas(canvas);
	nemotool_destroy_egl(egl);

	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

	free(mini);

	return 0;
}
