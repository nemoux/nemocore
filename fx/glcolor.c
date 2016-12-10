#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <glcolor.h>
#include <glhelper.h>
#include <nemomisc.h>

struct glcolor {
	GLuint texture;
	GLuint fbo, dbo;

	GLuint vshader;
	GLuint fshader;
	GLuint program;

	GLint ucolor;

	int32_t width, height;

	float color[4];
};

static const char GLCOLOR_VERTEX_SHADER[] =
"attribute vec2 position;\n"
"attribute vec2 texcoord;\n"
"void main()\n"
"{\n"
"  gl_Position = vec4(position, 0.0, 1.0);\n"
"}\n";

static const char GLCOLOR_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"uniform vec4 color;\n"
"void main()\n"
"{\n"
"  gl_FragColor = color;\n"
"}\n";

struct glcolor *nemofx_glcolor_create(int32_t width, int32_t height)
{
	struct glcolor *color;

	color = (struct glcolor *)malloc(sizeof(struct glcolor));
	if (color == NULL)
		return NULL;
	memset(color, 0, sizeof(struct glcolor));

	color->program = gl_compile_program(GLCOLOR_VERTEX_SHADER, GLCOLOR_FRAGMENT_SHADER, &color->vshader, &color->fshader);
	if (color->program == 0)
		goto err1;
	glUseProgram(color->program);
	glBindAttribLocation(color->program, 0, "position");
	glBindAttribLocation(color->program, 1, "texcoord");

	color->ucolor = glGetUniformLocation(color->program, "color");

	color->texture = gl_create_texture(GL_NEAREST, GL_CLAMP_TO_EDGE, width, height);

	gl_create_fbo(color->texture, width, height, &color->fbo, &color->dbo);

	color->width = width;
	color->height = height;

	return color;

err1:
	free(color);

	return NULL;
}

void nemofx_glcolor_destroy(struct glcolor *color)
{
	glDeleteTextures(1, &color->texture);

	glDeleteFramebuffers(1, &color->fbo);
	glDeleteRenderbuffers(1, &color->dbo);

	glDeleteShader(color->vshader);
	glDeleteShader(color->fshader);
	glDeleteProgram(color->program);

	free(color);
}

void nemofx_glcolor_set_color(struct glcolor *color, float r, float g, float b, float a)
{
	color->color[0] = r;
	color->color[1] = g;
	color->color[2] = b;
	color->color[3] = a;
}

void nemofx_glcolor_resize(struct glcolor *color, int32_t width, int32_t height)
{
	if (color->width != width || color->height != height) {
		glBindTexture(GL_TEXTURE_2D, color->texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		glDeleteFramebuffers(1, &color->fbo);
		glDeleteRenderbuffers(1, &color->dbo);

		gl_create_fbo(color->texture, width, height, &color->fbo, &color->dbo);

		color->width = width;
		color->height = height;
	}
}

uint32_t nemofx_glcolor_dispatch(struct glcolor *color)
{
	static GLfloat vertices[] = {
		-1.0f, -1.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 1.0f
	};

	glBindFramebuffer(GL_FRAMEBUFFER, color->fbo);

	glViewport(0, 0, color->width, color->height);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(color->program);
	glUniform4fv(color->ucolor, 1, color->color);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[2]);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return color->texture;
}

uint32_t nemofx_glcolor_get_texture(struct glcolor *color)
{
	return color->texture;
}
