#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <glmask.h>
#include <glshader.h>
#include <fbohelper.h>
#include <oshelper.h>
#include <nemomisc.h>

static const char GLMASK_VERTEX_SHADER[] =
"attribute vec2 position;\n"
"attribute vec2 texcoord;\n"
"varying vec2 vtexcoord;\n"
"void main()\n"
"{\n"
"  gl_Position = vec4(position, 0.0, 1.0);\n"
"  vtexcoord = texcoord;\n"
"}\n";

static const char GLMASK_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D tex;\n"
"uniform sampler2D mask;\n"
"uniform float width;\n"
"uniform float height;\n"
"void main()\n"
"{\n"
"  gl_FragColor = texture2D(tex, vtexcoord) * texture2D(mask, vtexcoord);\n"
"}\n";

static GLuint nemofx_glmask_create_program(const char *shader)
{
	const char *vertexshader = GLMASK_VERTEX_SHADER;
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

struct glmask *nemofx_glmask_create(int32_t width, int32_t height)
{
	struct glmask *mask;
	int size;

	mask = (struct glmask *)malloc(sizeof(struct glmask));
	if (mask == NULL)
		return NULL;
	memset(mask, 0, sizeof(struct glmask));

	glGenTextures(1, &mask->texture);

	glBindTexture(GL_TEXTURE_2D, mask->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	mask->program = nemofx_glmask_create_program(GLMASK_FRAGMENT_SHADER);
	if (mask->program == 0)
		goto err1;

	mask->utexture = glGetUniformLocation(mask->program, "tex");
	mask->uoverlay = glGetUniformLocation(mask->program, "mask");
	mask->uwidth = glGetUniformLocation(mask->program, "width");
	mask->uheight = glGetUniformLocation(mask->program, "height");

	fbo_prepare_context(mask->texture, width, height, &mask->fbo, &mask->dbo);

	mask->width = width;
	mask->height = height;

	return mask;

err1:
	glDeleteTextures(1, &mask->texture);

	free(mask);

	return NULL;
}

void nemofx_glmask_destroy(struct glmask *mask)
{
	glDeleteTextures(1, &mask->texture);

	glDeleteFramebuffers(1, &mask->fbo);
	glDeleteRenderbuffers(1, &mask->dbo);

	glDeleteProgram(mask->program);

	free(mask);
}

void nemofx_glmask_resize(struct glmask *mask, int32_t width, int32_t height)
{
	if (mask->width != width || mask->height != height) {
		glBindTexture(GL_TEXTURE_2D, mask->texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		glDeleteFramebuffers(1, &mask->fbo);
		glDeleteRenderbuffers(1, &mask->dbo);

		fbo_prepare_context(mask->texture, width, height, &mask->fbo, &mask->dbo);

		mask->width = width;
		mask->height = height;
	}
}

void nemofx_glmask_dispatch(struct glmask *mask, uint32_t texture, uint32_t overlay)
{
	static GLfloat vertices[] = {
		-1.0f, -1.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 1.0f
	};

	glBindFramebuffer(GL_FRAMEBUFFER, mask->fbo);

	glViewport(0, 0, mask->width, mask->height);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, overlay);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glUseProgram(mask->program);
	glUniform1i(mask->utexture, 0);
	glUniform1i(mask->uoverlay, 1);
	glUniform1f(mask->uwidth, mask->width);
	glUniform1f(mask->uheight, mask->height);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[2]);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
