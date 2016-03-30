#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <playshader.h>
#include <fbohelper.h>
#include <glhelper.h>
#include <nemomisc.h>

struct playshader *nemoplay_shader_create(void)
{
	static const char *play_vertex_shader =
		"attribute vec2 position;\n"
		"attribute vec2 texcoord;\n"
		"varying vec2 vtexcoord;\n"
		"void main()\n"
		"{\n"
		"  gl_Position = vec4(position, 0.0, 1.0);\n"
		"  vtexcoord = texcoord;\n"
		"}\n";

	static const char *play_fragment_shader =
		"precision mediump float;\n"
		"varying vec2 vtexcoord;\n"
		"uniform sampler2D texy;\n"
		"uniform sampler2D texu;\n"
		"uniform sampler2D texv;\n"
		"void main()\n"
		"{\n"
		"  float y = texture2D(texy, vtexcoord).r;\n"
		"  float u = texture2D(texu, vtexcoord).r - 0.5;\n"
		"  float v = texture2D(texv, vtexcoord).r - 0.5;\n"
		"  float r = y + 1.402 * v;\n"
		"  float g = y - 0.344 * u - 0.714 * v;\n"
		"  float b = y + 1.772 * u;\n"
		"  gl_FragColor = vec4(r, g, b, 1.0);\n"
		"}\n";

	struct playshader *shader;
	GLuint frag, vert;
	GLuint program;
	GLint status;

	shader = (struct playshader *)malloc(sizeof(struct playshader));
	if (shader == NULL)
		return NULL;
	memset(shader, 0, sizeof(struct playshader));

	frag = glshader_compile(GL_FRAGMENT_SHADER, 1, &play_fragment_shader);
	vert = glshader_compile(GL_VERTEX_SHADER, 1, &play_vertex_shader);

	program = glCreateProgram();
	glAttachShader(program, frag);
	glAttachShader(program, vert);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (!status)
		goto err1;

	glUseProgram(program);
	glBindAttribLocation(program, 0, "position");
	glBindAttribLocation(program, 1, "texcoord");

	shader->utexy = glGetUniformLocation(program, "texy");
	shader->utexu = glGetUniformLocation(program, "texu");
	shader->utexv = glGetUniformLocation(program, "texv");

	shader->program = program;

	return shader;

err1:
	free(shader);

	return NULL;
}

void nemoplay_shader_destroy(struct playshader *shader)
{
	if (shader->fbo > 0)
		glDeleteFramebuffers(1, &shader->fbo);
	if (shader->dbo > 0)
		glDeleteRenderbuffers(1, &shader->dbo);
	if (shader->program > 0)
		glDeleteProgram(shader->program);

	if (shader->texy > 0)
		glDeleteTextures(1, &shader->texy);
	if (shader->texu > 0)
		glDeleteTextures(1, &shader->texu);
	if (shader->texv > 0)
		glDeleteTextures(1, &shader->texv);

	free(shader);
}

int nemoplay_shader_set_texture(struct playshader *shader, GLuint texture, int32_t width, int32_t height)
{
	shader->texture = texture;
	shader->width = width;
	shader->height = height;

	fbo_prepare_context(texture, width, height, &shader->fbo, &shader->dbo);

	glGenTextures(1, &shader->texy);
	glBindTexture(GL_TEXTURE_2D, shader->texy);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_LUMINANCE,
			shader->width,
			shader->height,
			0,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &shader->texu);
	glBindTexture(GL_TEXTURE_2D, shader->texu);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_LUMINANCE,
			shader->width / 2,
			shader->height / 2,
			0,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &shader->texv);
	glBindTexture(GL_TEXTURE_2D, shader->texv);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_LUMINANCE,
			shader->width / 2,
			shader->height / 2,
			0,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			NULL);
	glBindTexture(GL_TEXTURE_2D, 0);
}

int nemoplay_shader_dispatch(struct playshader *shader, uint8_t *y, uint8_t *u, uint8_t *v)
{
	GLfloat vertices[] = {
		-1.0f, -1.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 1.0f
	};

	glBindFramebuffer(GL_FRAMEBUFFER, shader->fbo);

	glViewport(0, 0, shader->width, shader->height);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shader->program);
	glUniform1i(shader->utexy, 0);
	glUniform1i(shader->utexu, 1);
	glUniform1i(shader->utexv, 2);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shader->texv);
	glTexSubImage2D(GL_TEXTURE_2D, 0,
			0, 0,
			shader->width / 2, shader->height / 2,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			v);
	glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, shader->texu);
	glTexSubImage2D(GL_TEXTURE_2D, 0,
			0, 0,
			shader->width / 2, shader->height / 2,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			u);
	glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, shader->texy);
	glTexSubImage2D(GL_TEXTURE_2D, 0,
			0, 0,
			shader->width, shader->height,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			y);
	glBindTexture(GL_TEXTURE_2D, 0);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[2]);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
