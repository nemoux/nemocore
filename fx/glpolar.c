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

#include <glpolar.h>
#include <glhelper.h>
#include <oshelper.h>
#include <nemomisc.h>

struct glpolar {
	GLuint texture;
	GLuint fbo, dbo;

	GLuint vshader;
	GLuint fshader;
	GLuint program;

	GLint utexture;
	GLint uwidth, uheight;
	GLint ucolor;

	int32_t width, height;

	float color[4];
};

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

	polar->program = gl_compile_program(GLPOLAR_VERTEX_SHADER, GLPOLAR_FRAGMENT_SHADER, &polar->vshader, &polar->fshader);
	if (polar->program == 0)
		goto err1;
	glUseProgram(polar->program);
	glBindAttribLocation(polar->program, 0, "position");
	glBindAttribLocation(polar->program, 1, "texcoord");

	polar->utexture = glGetUniformLocation(polar->program, "tex");
	polar->uwidth = glGetUniformLocation(polar->program, "width");
	polar->uheight = glGetUniformLocation(polar->program, "height");
	polar->ucolor = glGetUniformLocation(polar->program, "color");

	gl_create_fbo(polar->texture, width, height, &polar->fbo, &polar->dbo);

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

	glDeleteShader(polar->vshader);
	glDeleteShader(polar->fshader);
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

		gl_create_fbo(polar->texture, width, height, &polar->fbo, &polar->dbo);

		polar->width = width;
		polar->height = height;
	}
}

void nemofx_glpolar_dispatch(struct glpolar *polar, uint32_t texture)
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

uint32_t nemofx_glpolar_get_texture(struct glpolar *polar)
{
	return polar->texture;
}
