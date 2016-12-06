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

#include <glmask.h>
#include <glhelper.h>
#include <oshelper.h>
#include <nemomisc.h>

struct glmask {
	GLuint texture;
	GLuint fbo, dbo;

	GLuint vshader;
	GLuint fshader;
	GLuint program;

	GLint utexture;
	GLint uoverlay;
	GLint uwidth, uheight;

	int32_t width, height;
};

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

struct glmask *nemofx_glmask_create(int32_t width, int32_t height)
{
	struct glmask *mask;

	mask = (struct glmask *)malloc(sizeof(struct glmask));
	if (mask == NULL)
		return NULL;
	memset(mask, 0, sizeof(struct glmask));

	mask->program = gl_compile_program(GLMASK_VERTEX_SHADER, GLMASK_FRAGMENT_SHADER, &mask->vshader, &mask->fshader);
	if (mask->program == 0)
		goto err1;
	glUseProgram(mask->program);
	glBindAttribLocation(mask->program, 0, "position");
	glBindAttribLocation(mask->program, 1, "texcoord");

	mask->utexture = glGetUniformLocation(mask->program, "tex");
	mask->uoverlay = glGetUniformLocation(mask->program, "mask");
	mask->uwidth = glGetUniformLocation(mask->program, "width");
	mask->uheight = glGetUniformLocation(mask->program, "height");

	mask->texture = gl_create_texture(GL_LINEAR, GL_CLAMP_TO_EDGE, width, height);

	gl_create_fbo(mask->texture, width, height, &mask->fbo, &mask->dbo);

	mask->width = width;
	mask->height = height;

	return mask;

err1:
	free(mask);

	return NULL;
}

void nemofx_glmask_destroy(struct glmask *mask)
{
	glDeleteTextures(1, &mask->texture);

	glDeleteFramebuffers(1, &mask->fbo);
	glDeleteRenderbuffers(1, &mask->dbo);

	glDeleteShader(mask->vshader);
	glDeleteShader(mask->fshader);
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

		gl_create_fbo(mask->texture, width, height, &mask->fbo, &mask->dbo);

		mask->width = width;
		mask->height = height;
	}
}

uint32_t nemofx_glmask_dispatch(struct glmask *mask, uint32_t texture, uint32_t overlay)
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

	return mask->texture;
}

int32_t nemofx_glmask_get_width(struct glmask *mask)
{
	return mask->width;
}

int32_t nemofx_glmask_get_height(struct glmask *mask)
{
	return mask->height;
}

uint32_t nemofx_glmask_get_texture(struct glmask *mask)
{
	return mask->texture;
}
