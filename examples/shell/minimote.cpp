#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <minimote.h>
#include <showhelper.h>
#include <glhelper.h>
#include <fbohelper.h>
#include <pixmanhelper.h>
#include <nemomisc.h>

static GLuint minishell_mote_create_shader(void)
{
	static const char *vert_shader_text =
		"uniform mat4 projection;\n"
		"attribute vec2 position;\n"
		"attribute vec2 texcoord;\n"
		"varying vec2 v_texcoord;\n"
		"void main() {\n"
		"  gl_Position = projection * vec4(position, 0.0, 1.0f);\n"
		"  v_texcoord = texcoord;\n"
		"}\n";
	static const char *frag_shader_text =
		"precision mediump float;\n"
		"varying vec2 v_texcoord;\n"
		"uniform sampler2D tex0;\n"
		"uniform vec4 color;\n"
		"void main() {\n"
		"  gl_FragColor = texture2D(tex0, v_texcoord) * color;\n"
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

struct minimote *minishell_mote_create(struct showone *one)
{
	struct minimote *mote;
	GLfloat vertices[16] = {
		1.0f, -1.0f, 1.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 0.0f,
	};
	GLfloat rgba[4] = { 0.0f, 1.0f, 1.0f, 1.0f };
	GLuint texture;
	pixman_image_t *image;

	mote = (struct minimote *)malloc(sizeof(struct minimote));
	if (mote == NULL)
		return NULL;
	memset(mote, 0, sizeof(struct minimote));

	mote->node = nemoshow_canvas_get_node(one);
	if (mote->node == NULL)
		goto err1;

	mote->tex = nemotale_node_get_texture(mote->node);
	mote->width = nemotale_node_get_width(mote->node);
	mote->height = nemotale_node_get_height(mote->node);

	mote->program = minishell_mote_create_shader();

	mote->utex0 = glGetUniformLocation(mote->program, "tex0");
	mote->uprojection = glGetUniformLocation(mote->program, "projection");
	mote->ucolor = glGetUniformLocation(mote->program, "color");

	fbo_prepare_context(mote->tex, mote->width, mote->height, &mote->fbo, &mote->dbo);

	nemomatrix_init_identity(&mote->matrix);
	nemomatrix_scale(&mote->matrix, 0.5f, -0.5f);

	glGenVertexArrays(1, &mote->vertex_array);
	glBindVertexArray(mote->vertex_array);

	glGenBuffers(1, &mote->vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, mote->vertex_buffer);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *)sizeof(GLfloat[2]));
	glEnableVertexAttribArray(1);

	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 16, vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glUseProgram(mote->program);

	glActiveTexture(GL_TEXTURE0);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
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

	glBindFramebuffer(GL_FRAMEBUFFER, mote->fbo);

	glViewport(0, 0, mote->width, mote->height);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUniform1i(mote->utex0, 0);
	glUniformMatrix4fv(mote->uprojection, 1, GL_FALSE, (GLfloat *)mote->matrix.d);
	glUniform4fv(mote->ucolor, 1, rgba);

	glBindVertexArray(mote->vertex_array);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glBindVertexArray(0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindTexture(GL_TEXTURE_2D, 0);

	return mote;

err1:
	free(mote);

	return NULL;
}

void minishell_mote_destroy(struct minimote *mote)
{
	glDeleteBuffers(1, &mote->vertex_buffer);
	glDeleteVertexArrays(1, &mote->vertex_array);

	glDeleteFramebuffers(1, &mote->fbo);
	glDeleteRenderbuffers(1, &mote->dbo);

	free(mote);
}
