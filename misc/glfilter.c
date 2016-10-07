#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <glfilter.h>
#include <glhelper.h>
#include <fbohelper.h>
#include <oshelper.h>
#include <nemomisc.h>

static GLuint glfilter_create_program(const char *shader)
{
	const char *vertexshader = GLFILTER_SIMPLE_VERTEX_SHADER;
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

struct glfilter *glfilter_create(const char *shaderpath, int32_t width, int32_t height)
{
	struct glfilter *filter;
	char *shader;
	int size;

	filter = (struct glfilter *)malloc(sizeof(struct glfilter));
	if (filter == NULL)
		return NULL;
	memset(filter, 0, sizeof(struct glfilter));

	glGenTextures(1, &filter->texture);
	glBindTexture(GL_TEXTURE_2D, filter->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	if (os_load_path(shaderpath, &shader, &size) < 0)
		goto err1;

	filter->program = glfilter_create_program(shader);
	if (filter->program == 0)
		goto err2;
	free(shader);

	filter->utexture = glGetUniformLocation(filter->program, "tex");
	filter->uwidth = glGetUniformLocation(filter->program, "width");
	filter->uheight = glGetUniformLocation(filter->program, "height");
	filter->utime = glGetUniformLocation(filter->program, "time");

	fbo_prepare_context(filter->texture, width, height, &filter->fbo, &filter->dbo);

	filter->width = width;
	filter->height = height;

	return filter;

err2:
	free(shader);

err1:
	glDeleteTextures(1, &filter->texture);

	free(filter);

	return NULL;
}

void glfilter_destroy(struct glfilter *filter)
{
	glDeleteTextures(1, &filter->texture);

	glDeleteFramebuffers(1, &filter->fbo);
	glDeleteRenderbuffers(1, &filter->dbo);

	glDeleteProgram(filter->program);

	free(filter);
}

void glfilter_resize(struct glfilter *filter, int32_t width, int32_t height)
{
	if (filter->width != width || filter->height != height) {
		glBindTexture(GL_TEXTURE_2D, filter->texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		glDeleteFramebuffers(1, &filter->fbo);
		glDeleteRenderbuffers(1, &filter->dbo);

		fbo_prepare_context(filter->texture, width, height, &filter->fbo, &filter->dbo);

		filter->width = width;
		filter->height = height;
	}
}

void glfilter_dispatch(struct glfilter *filter, GLuint texture)
{
	static GLfloat vertices[] = {
		-1.0f, -1.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 1.0f
	};

	glBindFramebuffer(GL_FRAMEBUFFER, filter->fbo);

	glViewport(0, 0, filter->width, filter->height);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(filter->program);
	glUniform1i(filter->utexture, 0);
	glUniform1f(filter->uwidth, filter->width);
	glUniform1f(filter->uheight, filter->height);
	glUniform1f(filter->utime, (float)time_current_nsecs() / 1000000000.0f);

	glBindTexture(GL_TEXTURE_2D, texture);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[2]);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
