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
	struct playshader *shader;

	shader = (struct playshader *)malloc(sizeof(struct playshader));
	if (shader == NULL)
		return NULL;
	memset(shader, 0, sizeof(struct playshader));

	return shader;
}

void nemoplay_shader_destroy(struct playshader *shader)
{
	if (shader->fbo > 0)
		glDeleteFramebuffers(1, &shader->fbo);
	if (shader->dbo > 0)
		glDeleteRenderbuffers(1, &shader->dbo);

	if (shader->texy > 0)
		glDeleteTextures(1, &shader->texy);
	if (shader->texu > 0)
		glDeleteTextures(1, &shader->texu);
	if (shader->texv > 0)
		glDeleteTextures(1, &shader->texv);

	if (shader->shaders[0] > 0)
		glDeleteShader(shader->shaders[0]);
	if (shader->shaders[1] > 0)
		glDeleteShader(shader->shaders[1]);
	if (shader->program > 0)
		glDeleteProgram(shader->program);

	free(shader);
}

int nemoplay_shader_set_texture(struct playshader *shader, int32_t width, int32_t height)
{
	if (shader->texy > 0)
		glDeleteTextures(1, &shader->texy);
	if (shader->texu > 0)
		glDeleteTextures(1, &shader->texu);
	if (shader->texv > 0)
		glDeleteTextures(1, &shader->texv);

	glGenTextures(1, &shader->texy);
	glBindTexture(GL_TEXTURE_2D, shader->texy);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, width);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_LUMINANCE,
			width,
			height,
			0,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &shader->texu);
	glBindTexture(GL_TEXTURE_2D, shader->texu);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, width / 2);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_LUMINANCE,
			width / 2,
			height / 2,
			0,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &shader->texv);
	glBindTexture(GL_TEXTURE_2D, shader->texv);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, width / 2);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_LUMINANCE,
			width / 2,
			height / 2,
			0,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	shader->texture_width = width;
	shader->texture_height = height;

	return 0;
}

int nemoplay_shader_set_viewport(struct playshader *shader, GLuint texture, int32_t width, int32_t height)
{
	if (shader->fbo > 0)
		glDeleteFramebuffers(1, &shader->fbo);
	if (shader->dbo > 0)
		glDeleteRenderbuffers(1, &shader->dbo);

	fbo_prepare_context(texture, width, height, &shader->fbo, &shader->dbo);

	shader->texture = texture;
	shader->viewport_width = width;
	shader->viewport_height = height;

	return 0;
}

int nemoplay_shader_prepare(struct playshader *shader, const char *vertex_source, const char *fragment_source)
{
	GLuint frag, vert;
	GLuint program;
	GLint status;

	frag = glshader_compile(GL_FRAGMENT_SHADER, 1, &fragment_source);
	vert = glshader_compile(GL_VERTEX_SHADER, 1, &vertex_source);

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

	shader->shaders[0] = frag;
	shader->shaders[1] = vert;
	shader->program = program;

	return 0;

err1:
	glDeleteShader(frag);
	glDeleteShader(vert);
	glDeleteProgram(program);

	return -1;
}

void nemoplay_shader_finish(struct playshader *shader)
{
	glDeleteShader(shader->shaders[0]);
	glDeleteShader(shader->shaders[1]);
	glDeleteProgram(shader->program);

	shader->shaders[0] = 0;
	shader->shaders[1] = 0;
	shader->program = 0;
}

int nemoplay_shader_update(struct playshader *shader, uint8_t *y, uint8_t *u, uint8_t *v)
{
	glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, 0);

	glBindTexture(GL_TEXTURE_2D, shader->texv);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, shader->texture_width / 2);
	glTexSubImage2D(GL_TEXTURE_2D, 0,
			0, 0,
			shader->texture_width / 2,
			shader->texture_height / 2,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			v);

	glBindTexture(GL_TEXTURE_2D, shader->texu);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, shader->texture_width / 2);
	glTexSubImage2D(GL_TEXTURE_2D, 0,
			0, 0,
			shader->texture_width / 2,
			shader->texture_height / 2,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			u);

	glBindTexture(GL_TEXTURE_2D, shader->texy);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, shader->texture_width);
	glTexSubImage2D(GL_TEXTURE_2D, 0,
			0, 0,
			shader->texture_width,
			shader->texture_height,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			y);

	return 0;
}

int nemoplay_shader_clear(struct playshader *shader)
{
	uint8_t y[(shader->texture_width) * (shader->texture_height)];
	uint8_t u[(shader->texture_width / 2) * (shader->texture_height / 2)];
	uint8_t v[(shader->texture_width / 2) * (shader->texture_height / 2)];

	memset(y, 0, (shader->texture_width) * (shader->texture_height));
	memset(u, 0, (shader->texture_width / 2) * (shader->texture_height / 2));
	memset(v, 0, (shader->texture_width / 2) * (shader->texture_height / 2));

	glBindTexture(GL_TEXTURE_2D, shader->texv);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, shader->texture_width / 2);
	glTexSubImage2D(GL_TEXTURE_2D, 0,
			0, 0,
			shader->texture_width / 2,
			shader->texture_height / 2,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			v);

	glBindTexture(GL_TEXTURE_2D, shader->texu);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, shader->texture_width / 2);
	glTexSubImage2D(GL_TEXTURE_2D, 0,
			0, 0,
			shader->texture_width / 2,
			shader->texture_height / 2,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			u);

	glBindTexture(GL_TEXTURE_2D, shader->texy);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, shader->texture_width);
	glTexSubImage2D(GL_TEXTURE_2D, 0,
			0, 0,
			shader->texture_width,
			shader->texture_height,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			y);

	return 0;
}

int nemoplay_shader_dispatch(struct playshader *shader)
{
	GLfloat vertices[] = {
		-1.0f, -1.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 1.0f
	};

	glBindFramebuffer(GL_FRAMEBUFFER, shader->fbo);

	glViewport(0, 0, shader->viewport_width, shader->viewport_height);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shader->program);
	glUniform1i(shader->utexy, 0);
	glUniform1i(shader->utexu, 1);
	glUniform1i(shader->utexv, 2);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shader->texv);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, shader->texu);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, shader->texy);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[2]);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return 0;
}
