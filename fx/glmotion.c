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

#include <glmotion.h>
#include <glhelper.h>
#include <nemomisc.h>

struct glmotion {
	GLuint texture[2];
	GLuint fbo[2], dbo[2];

	GLuint vshader;
	GLuint fshader;
	GLuint program;

	GLuint utexture;
	GLuint uwidth, uheight;
	GLuint udirectx, udirecty;
	GLuint ustep;

	int32_t width, height;

	float step;
};

static const char GLMOTION_SIMPLE_VERTEX_SHADER[] =
"attribute vec2 position;\n"
"attribute vec2 texcoord;\n"
"varying vec2 vtexcoord;\n"
"void main()\n"
"{\n"
"  gl_Position = vec4(position, 0.0, 1.0);\n"
"  vtexcoord = texcoord;\n"
"}\n";

static const char GLMOTION_ACCUMULATE_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D tex;\n"
"uniform float width;\n"
"uniform float height;\n"
"uniform float step;\n"
"void main()\n"
"{\n"
"  gl_FragColor = texture2D(tex, vtexcoord) * step;\n"
"}\n";

struct glmotion *nemofx_glmotion_create(int32_t width, int32_t height)
{
	struct glmotion *motion;

	motion = (struct glmotion *)malloc(sizeof(struct glmotion));
	if (motion == NULL)
		return NULL;
	memset(motion, 0, sizeof(struct glmotion));

	motion->program = gl_compile_program(GLMOTION_SIMPLE_VERTEX_SHADER, GLMOTION_ACCUMULATE_FRAGMENT_SHADER, &motion->vshader, &motion->fshader);
	if (motion->program == 0)
		goto err1;
	glUseProgram(motion->program);
	glBindAttribLocation(motion->program, 0, "position");
	glBindAttribLocation(motion->program, 1, "texcoord");

	motion->utexture = glGetUniformLocation(motion->program, "tex");
	motion->uwidth = glGetUniformLocation(motion->program, "width");
	motion->uheight = glGetUniformLocation(motion->program, "height");
	motion->ustep = glGetUniformLocation(motion->program, "step");

	motion->texture[1] = gl_create_texture(GL_LINEAR, GL_CLAMP_TO_EDGE, width, height);

	gl_create_fbo(motion->texture[1], width, height, &motion->fbo[1], &motion->dbo[1]);

	motion->width = width;
	motion->height = height;

	return motion;

err1:
	free(motion);

	return NULL;
}

void nemofx_glmotion_destroy(struct glmotion *motion)
{
	if (motion->texture[0] > 0)
		glDeleteTextures(1, &motion->texture[0]);
	if (motion->fbo[0] > 0)
		glDeleteFramebuffers(1, &motion->fbo[0]);
	if (motion->dbo[0] > 0)
		glDeleteRenderbuffers(1, &motion->dbo[0]);

	glDeleteTextures(1, &motion->texture[1]);
	glDeleteFramebuffers(1, &motion->fbo[1]);
	glDeleteRenderbuffers(1, &motion->dbo[1]);

	glDeleteShader(motion->vshader);
	glDeleteShader(motion->fshader);
	glDeleteProgram(motion->program);

	free(motion);
}

void nemofx_glmotion_use_fbo(struct glmotion *motion)
{
	motion->texture[0] = gl_create_texture(GL_LINEAR, GL_CLAMP_TO_EDGE, motion->width, motion->height);

	gl_create_fbo(motion->texture[0], motion->width, motion->height, &motion->fbo[0], &motion->dbo[0]);
}

void nemofx_glmotion_set_step(struct glmotion *motion, float step)
{
	motion->step = step;
}

void nemofx_glmotion_resize(struct glmotion *motion, int32_t width, int32_t height)
{
	if (motion->width == width && motion->height == height)
		return;

	if (motion->texture[0] > 0) {
		glBindTexture(GL_TEXTURE_2D, motion->texture[0]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		glDeleteFramebuffers(1, &motion->fbo[0]);
		glDeleteRenderbuffers(1, &motion->dbo[0]);

		gl_create_fbo(motion->texture[0], width, height, &motion->fbo[0], &motion->dbo[0]);
	}

	glBindTexture(GL_TEXTURE_2D, motion->texture[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glDeleteFramebuffers(1, &motion->fbo[1]);
	glDeleteRenderbuffers(1, &motion->dbo[1]);

	gl_create_fbo(motion->texture[1], width, height, &motion->fbo[1], &motion->dbo[1]);

	motion->width = width;
	motion->height = height;
}

static inline void nemofx_glmotion_switch(struct glmotion *motion)
{
	GLuint texture;
	GLuint fbo, dbo;

	texture = motion->texture[0];
	fbo = motion->fbo[0];
	dbo = motion->dbo[0];

	motion->texture[0] = motion->texture[1];
	motion->fbo[0] = motion->fbo[1];
	motion->dbo[0] = motion->dbo[1];

	motion->texture[1] = texture;
	motion->fbo[1] = fbo;
	motion->dbo[1] = dbo;
}

uint32_t nemofx_glmotion_dispatch(struct glmotion *motion, uint32_t texture)
{
	static GLfloat vertices[] = {
		-1.0f, -1.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 1.0f
	};

	nemofx_glmotion_switch(motion);

	// first pass
	glBindFramebuffer(GL_FRAMEBUFFER, motion->fbo[0]);

	glDisable(GL_BLEND);

	glViewport(0, 0, motion->width, motion->height);

	glBindTexture(GL_TEXTURE_2D, motion->texture[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glUseProgram(motion->program);
	glUniform1i(motion->utexture, 0);
	glUniform1f(motion->uwidth, motion->width);
	glUniform1f(motion->uheight, motion->height);
	glUniform1f(motion->ustep, motion->step);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[2]);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// second pass
	glBindFramebuffer(GL_FRAMEBUFFER, motion->fbo[0]);

	glEnable(GL_BLEND);
	glBlendEquation(GL_MAX);
	glBlendFunc(GL_ONE, GL_ONE);

	glViewport(0, 0, motion->width, motion->height);

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glUseProgram(motion->program);
	glUniform1i(motion->utexture, 0);
	glUniform1f(motion->uwidth, motion->width);
	glUniform1f(motion->uheight, motion->height);
	glUniform1f(motion->ustep, 1.0f);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[2]);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glBindTexture(GL_TEXTURE_2D, 0);

	glBlendEquation(GL_FUNC_ADD);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return motion->texture[0];
}

void nemofx_glmotion_clear(struct glmotion *motion)
{
	glBindFramebuffer(GL_FRAMEBUFFER, motion->fbo[0]);
	glViewport(0, 0, motion->width, motion->height);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, motion->fbo[1]);
	glViewport(0, 0, motion->width, motion->height);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

float nemofx_glmotion_get_step(struct glmotion *motion)
{
	return motion->step;
}

uint32_t nemofx_glmotion_get_texture(struct glmotion *motion)
{
	return motion->texture[0];
}
