#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <glpolar.h>
#include <glshader.h>
#include <fbohelper.h>
#include <oshelper.h>
#include <nemomisc.h>

static const char GLPOLAR_VERTEX_SHADER[] =
"attribute vec2 position;\n"
"attribute vec2 texcoord;\n"
"varying vec2 vtexcoord;\n"
"void main()\n"
"{\n"
"  gl_Position = vec4(position, 0.0, 1.0);\n"
"  vtexcoord = texcoord;\n"
"}\n";

static const char GLPOLAR_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D tex;\n"
"uniform float width;\n"
"uniform float height;\n"
"uniform vec4 color;\n"
"void main()\n"
"{\n"
"  gl_FragColor = texture2D(tex, vtexcoord) * color;\n"
"}\n";

static GLuint nemofx_glpolar_create_program(const char *shader)
{
	const char *vertexshader = GLPOLAR_VERTEX_SHADER;
	GLuint frag, vert;
	GLuint program;
	GLint status;

	frag = glshader_compile(GL_FRAGMENT_SHADER, 1, &shader);
	vert = glshader_compile(GL_VERTEX_SHADER, 1, &vertexshader);

	program = glCreateProgram();
	glAttachShader(program, frag);
	glAttachShader(program, vert);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (!status) {
		GLsizei len;
		char log[1000];

		glGetProgramInfoLog(program, 1000, &len, log);
		fprintf(stderr, "Error: linking:\n%*s\n", len, log);

		return 0;
	}

	glUseProgram(program);
	glBindAttribLocation(program, 0, "position");
	glBindAttribLocation(program, 1, "texcoord");

	return program;
}

struct glpolar *nemofx_glpolar_create(int32_t width, int32_t height)
{
	struct glpolar *polar;
	int size;

	polar = (struct glpolar *)malloc(sizeof(struct glpolar));
	if (polar == NULL)
		return NULL;
	memset(polar, 0, sizeof(struct glpolar));

	glGenTextures(1, &polar->texture);

	glBindTexture(GL_TEXTURE_2D, polar->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	polar->program = nemofx_glpolar_create_program(GLPOLAR_FRAGMENT_SHADER);
	if (polar->program == 0)
		goto err1;

	polar->utexture = glGetUniformLocation(polar->program, "tex");
	polar->uwidth = glGetUniformLocation(polar->program, "width");
	polar->uheight = glGetUniformLocation(polar->program, "height");
	polar->ucolor = glGetUniformLocation(polar->program, "color");

	fbo_prepare_context(polar->texture, width, height, &polar->fbo, &polar->dbo);

	polar->width = width;
	polar->height = height;

	return polar;

err1:
	glDeleteTextures(1, &polar->texture);

	free(polar);

	return NULL;
}

void nemofx_glpolar_destroy(struct glpolar *polar)
{
	glDeleteTextures(1, &polar->texture);

	glDeleteFramebuffers(1, &polar->fbo);
	glDeleteRenderbuffers(1, &polar->dbo);

	glDeleteProgram(polar->program);

	free(polar);
}

void nemofx_glpolar_set_color(struct glpolar *polar, float r, float g, float b, float a)
{
	polar->color[0] = r;
	polar->color[1] = g;
	polar->color[2] = b;
	polar->color[3] = a;
}

void nemofx_glpolar_resize(struct glpolar *polar, int32_t width, int32_t height)
{
	if (polar->width != width || polar->height != height) {
		glBindTexture(GL_TEXTURE_2D, polar->texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		glDeleteFramebuffers(1, &polar->fbo);
		glDeleteRenderbuffers(1, &polar->dbo);

		fbo_prepare_context(polar->texture, width, height, &polar->fbo, &polar->dbo);

		polar->width = width;
		polar->height = height;
	}
}

void nemofx_glpolar_dispatch(struct glpolar *polar, GLuint texture)
{
	static GLfloat vertices[] = {
		-1.0f, -1.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 1.0f
	};

	glBindFramebuffer(GL_FRAMEBUFFER, polar->fbo);

	glViewport(0, 0, polar->width, polar->height);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(polar->program);
	glUniform1i(polar->utexture, 0);
	glUniform1f(polar->uwidth, polar->width);
	glUniform1f(polar->uheight, polar->height);
	glUniform4fv(polar->ucolor, 1, polar->color);

	glBindTexture(GL_TEXTURE_2D, texture);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[2]);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
