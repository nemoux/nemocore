#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <glmotion.h>
#include <glshader.h>
#include <fbohelper.h>
#include <oshelper.h>
#include <nemomisc.h>

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
"uniform float velocity;\n"
"void main()\n"
"{\n"
"  gl_FragColor = texture2D(tex, vtexcoord) * velocity;\n"
"}\n";

static GLuint nemofx_glmotion_create_program(const char *shader)
{
	const char *vertexshader = GLMOTION_SIMPLE_VERTEX_SHADER;
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

struct glmotion *nemofx_glmotion_create(int32_t width, int32_t height)
{
	struct glmotion *motion;
	int size;

	motion = (struct glmotion *)malloc(sizeof(struct glmotion));
	if (motion == NULL)
		return NULL;
	memset(motion, 0, sizeof(struct glmotion));

	glGenTextures(2, &motion->texture[0]);

	glBindTexture(GL_TEXTURE_2D, motion->texture[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindTexture(GL_TEXTURE_2D, motion->texture[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	motion->program = nemofx_glmotion_create_program(GLMOTION_ACCUMULATE_FRAGMENT_SHADER);
	if (motion->program == 0)
		goto err1;

	motion->utexture = glGetUniformLocation(motion->program, "tex");
	motion->uwidth = glGetUniformLocation(motion->program, "width");
	motion->uheight = glGetUniformLocation(motion->program, "height");
	motion->uvelocity = glGetUniformLocation(motion->program, "velocity");

	fbo_prepare_context(motion->texture[0], width, height, &motion->fbo[0], &motion->dbo[0]);
	fbo_prepare_context(motion->texture[1], width, height, &motion->fbo[1], &motion->dbo[1]);

	motion->width = width;
	motion->height = height;

	return motion;

err1:
	glDeleteTextures(2, &motion->texture[0]);

	free(motion);

	return NULL;
}

void nemofx_glmotion_destroy(struct glmotion *motion)
{
	glDeleteTextures(2, &motion->texture[0]);

	glDeleteFramebuffers(1, &motion->fbo[0]);
	glDeleteRenderbuffers(1, &motion->dbo[0]);
	glDeleteFramebuffers(1, &motion->fbo[1]);
	glDeleteRenderbuffers(1, &motion->dbo[1]);

	glDeleteProgram(motion->program);

	free(motion);
}

void nemofx_glmotion_set_velocity(struct glmotion *motion, float velocity)
{
	motion->velocity = velocity;
}

void nemofx_glmotion_resize(struct glmotion *motion, int32_t width, int32_t height)
{
	if (motion->width != width || motion->height != height) {
		int i;

		for (i = 0; i < 2; i++) {
			glBindTexture(GL_TEXTURE_2D, motion->texture[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
			glBindTexture(GL_TEXTURE_2D, 0);

			glDeleteFramebuffers(1, &motion->fbo[i]);
			glDeleteRenderbuffers(1, &motion->dbo[i]);

			fbo_prepare_context(motion->texture[i], width, height, &motion->fbo[i], &motion->dbo[i]);
		}

		motion->width = width;
		motion->height = height;
	}
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

void nemofx_glmotion_dispatch(struct glmotion *motion, GLuint texture)
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

	glUseProgram(motion->program);
	glUniform1i(motion->utexture, 0);
	glUniform1f(motion->uwidth, motion->width);
	glUniform1f(motion->uheight, motion->height);
	glUniform1f(motion->uvelocity, motion->velocity);

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

	glUseProgram(motion->program);
	glUniform1i(motion->utexture, 0);
	glUniform1f(motion->uwidth, motion->width);
	glUniform1f(motion->uheight, motion->height);
	glUniform1f(motion->uvelocity, 1.0f);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[2]);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glBindTexture(GL_TEXTURE_2D, 0);

	glBlendEquation(GL_FUNC_ADD);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
