#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemoplay.h>
#include <playqueue.h>
#include <playshader.h>
#include <glhelper.h>
#include <nemomisc.h>

static const char NEMOPLAY_YUV420_VERTEX_SHADER[] =
"attribute vec2 position;\n"
"attribute vec2 texcoord;\n"
"varying vec2 vtexcoord;\n"
"void main()\n"
"{\n"
"  gl_Position = vec4(position, 0.0, 1.0);\n"
"  vtexcoord = texcoord;\n"
"}\n";

static const char NEMOPLAY_YUV420_FRAGMENT_SHADER[] =
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

static const char NEMOPLAY_BGRA_VERTEX_SHADER[] =
"attribute vec2 position;\n"
"attribute vec2 texcoord;\n"
"varying vec2 vtexcoord;\n"
"void main()\n"
"{\n"
"  gl_Position = vec4(position, 0.0, 1.0);\n"
"  vtexcoord = texcoord;\n"
"}\n";

static const char NEMOPLAY_BGRA_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D tex;\n"
"void main()\n"
"{\n"
"  vec4 c = texture2D(tex, vtexcoord);\n"
"  gl_FragColor = vec4(c.r, c.g, c.b, c.a);\n"
"}\n";

static const char NEMOPLAY_RGBA_VERTEX_SHADER[] =
"attribute vec2 position;\n"
"attribute vec2 texcoord;\n"
"varying vec2 vtexcoord;\n"
"void main()\n"
"{\n"
"  gl_Position = vec4(position, 0.0, 1.0);\n"
"  vtexcoord = texcoord;\n"
"}\n";

static const char NEMOPLAY_RGBA_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D tex;\n"
"void main()\n"
"{\n"
"  vec4 c = texture2D(tex, vtexcoord);\n"
"  gl_FragColor = vec4(c.b, c.g, c.r, c.a);\n"
"}\n";

static const GLfloat NEMOPLAY_POLYGONS[4][16] = {
	{
		-1.0f, -1.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 1.0f
	},
	{
		-1.0f, 1.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 1.0f
	},
	{
		1.0f, 1.0f, 0.0f, 0.0f,
		-1.0f, 1.0f, 1.0f, 0.0f,
		-1.0f, -1.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 0.0f, 1.0f
	},
	{
		1.0f, -1.0f, 0.0f, 0.0f,
		-1.0f, -1.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 0.0f, 1.0f
	}
};

static int nemoplay_shader_update_yuv420(struct playshader *shader, struct playone *one)
{
	glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, 0);

	glBindTexture(GL_TEXTURE_2D, shader->texv);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, nemoplay_one_get_linesize(one, 2));
	glTexSubImage2D(GL_TEXTURE_2D, 0,
			0, 0,
			shader->texture_width / 2,
			shader->texture_height / 2,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			nemoplay_one_get_data(one, 2));
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindTexture(GL_TEXTURE_2D, shader->texu);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, nemoplay_one_get_linesize(one, 1));
	glTexSubImage2D(GL_TEXTURE_2D, 0,
			0, 0,
			shader->texture_width / 2,
			shader->texture_height / 2,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			nemoplay_one_get_data(one, 1));
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindTexture(GL_TEXTURE_2D, shader->texy);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, nemoplay_one_get_linesize(one, 0));
	glTexSubImage2D(GL_TEXTURE_2D, 0,
			0, 0,
			shader->texture_width,
			shader->texture_height,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			nemoplay_one_get_data(one, 0));
	glBindTexture(GL_TEXTURE_2D, 0);

	return 0;
}

static int nemoplay_shader_update_yuv420_pbo(struct playshader *shader, struct playone *one)
{
	GLubyte *ptr;

	glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, 0);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, shader->pbov);

	ptr = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, shader->texture_width * shader->texture_height / 4, GL_MAP_WRITE_BIT);
	memcpy(ptr, nemoplay_one_get_data(one, 2), shader->texture_width * shader->texture_height / 4);
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

	glBindTexture(GL_TEXTURE_2D, shader->texv);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, nemoplay_one_get_linesize(one, 2));
	glTexSubImage2D(GL_TEXTURE_2D, 0,
			0, 0,
			shader->texture_width / 2,
			shader->texture_height / 2,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, shader->pbou);

	ptr = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, shader->texture_width * shader->texture_height / 4, GL_MAP_WRITE_BIT);
	memcpy(ptr, nemoplay_one_get_data(one, 1), shader->texture_width * shader->texture_height / 4);
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

	glBindTexture(GL_TEXTURE_2D, shader->texu);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, nemoplay_one_get_linesize(one, 1));
	glTexSubImage2D(GL_TEXTURE_2D, 0,
			0, 0,
			shader->texture_width / 2,
			shader->texture_height / 2,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, shader->pboy);

	ptr = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, shader->texture_width * shader->texture_height, GL_MAP_WRITE_BIT);
	memcpy(ptr, nemoplay_one_get_data(one, 0), shader->texture_width * shader->texture_height);
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

	glBindTexture(GL_TEXTURE_2D, shader->texy);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, nemoplay_one_get_linesize(one, 0));
	glTexSubImage2D(GL_TEXTURE_2D, 0,
			0, 0,
			shader->texture_width,
			shader->texture_height,
			GL_LUMINANCE,
			GL_UNSIGNED_BYTE,
			NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	return 0;
}

static int nemoplay_shader_update_rgba(struct playshader *shader, struct playone *one)
{
	glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, 0);

	glBindTexture(GL_TEXTURE_2D, shader->tex);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, nemoplay_one_get_linesize(one, 0) / 4);
	glTexSubImage2D(GL_TEXTURE_2D, 0,
			0, 0,
			shader->texture_width,
			shader->texture_height,
			GL_BGRA_EXT,
			GL_UNSIGNED_BYTE,
			nemoplay_one_get_data(one, 0));
	glBindTexture(GL_TEXTURE_2D, 0);

	return 0;
}

static int nemoplay_shader_update_rgba_pbo(struct playshader *shader, struct playone *one)
{
	GLubyte *ptr;

	glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, 0);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, shader->pbo);

	ptr = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, shader->texture_width * shader->texture_height * 4, GL_MAP_WRITE_BIT);
	memcpy(ptr, nemoplay_one_get_data(one, 0), shader->texture_width * shader->texture_height * 4);
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

	glBindTexture(GL_TEXTURE_2D, shader->tex);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, nemoplay_one_get_linesize(one, 0) / 4);
	glTexSubImage2D(GL_TEXTURE_2D, 0,
			0, 0,
			shader->texture_width,
			shader->texture_height,
			GL_BGRA_EXT,
			GL_UNSIGNED_BYTE,
			NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	return 0;
}

static int nemoplay_shader_dispatch_yuv420(struct playshader *shader)
{
	glBindFramebuffer(GL_FRAMEBUFFER, shader->fbo);

	glViewport(0, 0, shader->viewport_width, shader->viewport_height);

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

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &NEMOPLAY_POLYGONS[shader->polygon][0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &NEMOPLAY_POLYGONS[shader->polygon][2]);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return 0;
}

static int nemoplay_shader_dispatch_rgba(struct playshader *shader)
{
	glBindFramebuffer(GL_FRAMEBUFFER, shader->fbo);

	glViewport(0, 0, shader->viewport_width, shader->viewport_height);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(shader->program);
	glUniform1i(shader->utex, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, shader->tex);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &NEMOPLAY_POLYGONS[shader->polygon][0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &NEMOPLAY_POLYGONS[shader->polygon][2]);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return 0;
}

static int nemoplay_shader_resize_yuv420(struct playshader *shader, int32_t width, int32_t height)
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

	return 0;
}

static int nemoplay_shader_resize_yuv420_pbo(struct playshader *shader, int32_t width, int32_t height)
{
	if (shader->texy > 0)
		glDeleteTextures(1, &shader->texy);
	if (shader->texu > 0)
		glDeleteTextures(1, &shader->texu);
	if (shader->texv > 0)
		glDeleteTextures(1, &shader->texv);
	if (shader->pboy > 0)
		glDeleteBuffers(1, &shader->pboy);
	if (shader->pbou > 0)
		glDeleteBuffers(1, &shader->pbou);
	if (shader->pbov > 0)
		glDeleteBuffers(1, &shader->pbov);

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

	glGenBuffers(1, &shader->pboy);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, shader->pboy);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	glGenBuffers(1, &shader->pbou);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, shader->pbou);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height / 4, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	glGenBuffers(1, &shader->pbov);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, shader->pbov);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height / 4, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	return 0;
}

static int nemoplay_shader_resize_rgba(struct playshader *shader, int32_t width, int32_t height)
{
	if (shader->tex > 0)
		glDeleteTextures(1, &shader->tex);

	glGenTextures(1, &shader->tex);
	glBindTexture(GL_TEXTURE_2D, shader->tex);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, width);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_BGRA_EXT,
			width,
			height,
			0,
			GL_BGRA_EXT,
			GL_UNSIGNED_BYTE,
			NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	return 0;
}

static int nemoplay_shader_resize_rgba_pbo(struct playshader *shader, int32_t width, int32_t height)
{
	if (shader->tex > 0)
		glDeleteTextures(1, &shader->tex);
	if (shader->pbo > 0)
		glDeleteBuffers(1, &shader->pbo);

	glGenTextures(1, &shader->tex);
	glBindTexture(GL_TEXTURE_2D, shader->tex);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, width);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_BGRA_EXT,
			width,
			height,
			0,
			GL_BGRA_EXT,
			GL_UNSIGNED_BYTE,
			NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenBuffers(1, &shader->pbo);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, shader->pbo);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height * 4, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	return 0;
}

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

	if (shader->tex > 0)
		glDeleteTextures(1, &shader->tex);
	if (shader->texy > 0)
		glDeleteTextures(1, &shader->texy);
	if (shader->texu > 0)
		glDeleteTextures(1, &shader->texu);
	if (shader->texv > 0)
		glDeleteTextures(1, &shader->texv);

	if (shader->pbo > 0)
		glDeleteBuffers(1, &shader->pbo);
	if (shader->pboy > 0)
		glDeleteBuffers(1, &shader->pboy);
	if (shader->pbou > 0)
		glDeleteBuffers(1, &shader->pbou);
	if (shader->pbov > 0)
		glDeleteBuffers(1, &shader->pbo);

	if (shader->shaders[0] > 0)
		glDeleteShader(shader->shaders[0]);
	if (shader->shaders[1] > 0)
		glDeleteShader(shader->shaders[1]);
	if (shader->program > 0)
		glDeleteProgram(shader->program);

	free(shader);
}

static inline void nemoplay_shader_update_callbacks(struct playshader *shader, int format, int use_pbo)
{
	if (NEMOPLAY_PIXEL_IS_YUV420_FORMAT(format)) {
		if (use_pbo == 0) {
			shader->resize = nemoplay_shader_resize_yuv420;
			shader->update = nemoplay_shader_update_yuv420;
			shader->dispatch = nemoplay_shader_dispatch_yuv420;
		} else {
			shader->resize = nemoplay_shader_resize_yuv420_pbo;
			shader->update = nemoplay_shader_update_yuv420_pbo;
			shader->dispatch = nemoplay_shader_dispatch_yuv420;
		}
	} else if (NEMOPLAY_PIXEL_IS_RGBA_FORMAT(format)) {
		if (use_pbo == 0) {
			shader->resize = nemoplay_shader_resize_rgba;
			shader->update = nemoplay_shader_update_rgba;
			shader->dispatch = nemoplay_shader_dispatch_rgba;
		} else {
			shader->resize = nemoplay_shader_resize_rgba_pbo;
			shader->update = nemoplay_shader_update_rgba_pbo;
			shader->dispatch = nemoplay_shader_dispatch_rgba;
		}
	}
}

int nemoplay_shader_set_format(struct playshader *shader, int format)
{
	if (shader->shaders[0] > 0)
		glDeleteShader(shader->shaders[0]);
	if (shader->shaders[1] > 0)
		glDeleteShader(shader->shaders[1]);
	if (shader->program > 0)
		glDeleteProgram(shader->program);

	if (format == NEMOPLAY_YUV420_PIXEL_FORMAT) {
		shader->program = gl_compile_program(NEMOPLAY_YUV420_VERTEX_SHADER, NEMOPLAY_YUV420_FRAGMENT_SHADER, &shader->shaders[0], &shader->shaders[1]);
		if (shader->program == 0)
			return -1;
		glUseProgram(shader->program);
		glBindAttribLocation(shader->program, 0, "position");
		glBindAttribLocation(shader->program, 1, "texcoord");

		shader->utexy = glGetUniformLocation(shader->program, "texy");
		shader->utexu = glGetUniformLocation(shader->program, "texu");
		shader->utexv = glGetUniformLocation(shader->program, "texv");
	} else if (format == NEMOPLAY_BGRA_PIXEL_FORMAT) {
		shader->program = gl_compile_program(NEMOPLAY_BGRA_VERTEX_SHADER, NEMOPLAY_BGRA_FRAGMENT_SHADER, &shader->shaders[0], &shader->shaders[1]);
		if (shader->program == 0)
			return -1;
		glUseProgram(shader->program);
		glBindAttribLocation(shader->program, 0, "position");
		glBindAttribLocation(shader->program, 1, "texcoord");

		shader->utex = glGetUniformLocation(shader->program, "tex");
	} else if (format == NEMOPLAY_RGBA_PIXEL_FORMAT) {
		shader->program = gl_compile_program(NEMOPLAY_RGBA_VERTEX_SHADER, NEMOPLAY_RGBA_FRAGMENT_SHADER, &shader->shaders[0], &shader->shaders[1]);
		if (shader->program == 0)
			return -1;
		glUseProgram(shader->program);
		glBindAttribLocation(shader->program, 0, "position");
		glBindAttribLocation(shader->program, 1, "texcoord");

		shader->utex = glGetUniformLocation(shader->program, "tex");
	}

	shader->format = format;

	nemoplay_shader_update_callbacks(shader, shader->format, shader->use_pbo);

	return 0;
}

int nemoplay_shader_set_viewport(struct playshader *shader, uint32_t texture, int32_t width, int32_t height)
{
	if (shader->fbo > 0)
		glDeleteFramebuffers(1, &shader->fbo);
	if (shader->dbo > 0)
		glDeleteRenderbuffers(1, &shader->dbo);

	if (texture > 0) {
		gl_create_fbo(texture, width, height, &shader->fbo, &shader->dbo);

		shader->texture = texture;
	} else {
		shader->texture = 0;
		shader->fbo = 0;
		shader->dbo = 0;
	}

	shader->viewport_width = width;
	shader->viewport_height = height;

	return 0;
}

int nemoplay_shader_set_polygon(struct playshader *shader, int polygon)
{
	shader->polygon = polygon;

	return 0;
}

int nemoplay_shader_set_pbo(struct playshader *shader, int use_pbo)
{
	shader->use_pbo = use_pbo;

	nemoplay_shader_update_callbacks(shader, shader->format, shader->use_pbo);

	return 0;
}
