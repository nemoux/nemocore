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

#include <glfilter.h>
#include <glhelper.h>
#include <nemomisc.h>

struct glfilter {
	GLuint texture;
	GLuint fbo, dbo;

	GLuint vshader;
	GLuint fshader;
	GLuint program;

	GLint utexture;
	GLint uwidth;
	GLint uheight;
	GLint utime;

	int32_t width, height;
};

static const char GLFILTER_SIMPLE_VERTEX_SHADER[] =
"attribute vec2 position;\n"
"attribute vec2 texcoord;\n"
"varying vec2 vtexcoord;\n"
"void main()\n"
"{\n"
"  gl_Position = vec4(position, 0.0, 1.0);\n"
"  vtexcoord = texcoord;\n"
"}\n";

static const char GLFILTER_SIMPLE_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D tex;\n"
"uniform float width;\n"
"uniform float height;\n"
"uniform float time;\n"
"void main()\n"
"{\n"
"  gl_FragColor = texture2D(tex, vtexcoord);\n"
"}\n";

static const char GLFILTER_GAUSSIAN_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D tex;\n"
"uniform float width;\n"
"uniform float height;\n"
"uniform float time;\n"
"void main()\n"
"{\n"
"  float sw = 1.0 / width;\n"
"  float sh = 1.0 / height;\n"
"  vec4 s = vec4(0.0);\n"
"  s += texture2D(tex, vtexcoord + vec2(-sw, -sh)) * 1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(0.0, -sh)) * 2.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(sw, -sh)) * 1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(-sw, 0.0)) * 2.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(0.0, 0.0)) * 4.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(sw, 0.0)) * 2.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(-sw, sh)) * 1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(0.0, sh)) * 2.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(sw, sh)) * 1.0;\n"
"  gl_FragColor = clamp(s / 16.0 + 0.0, 0.0, 1.0);\n"
"}\n";

static const char GLFILTER_LAPLACIAN_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D tex;\n"
"uniform float width;\n"
"uniform float height;\n"
"uniform float time;\n"
"void main()\n"
"{\n"
"  float sw = 1.0 / width;\n"
"  float sh = 1.0 / height;\n"
"  vec4 s = vec4(0.0);\n"
"  s += texture2D(tex, vtexcoord + vec2(-sw, -sh)) * 0.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(0.0, -sh)) * 1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(sw, -sh)) * 0.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(-sw, 0.0)) * 1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(0.0, 0.0)) * -4.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(sw, 0.0)) * 1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(-sw, sh)) * 0.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(0.0, sh)) * 1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(sw, sh)) * 0.0;\n"
"  gl_FragColor = clamp(s / 0.1 + 0.0, 0.0, 1.0);\n"
"}\n";

static const char GLFILTER_EDGEDETECTION_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D tex;\n"
"uniform float width;\n"
"uniform float height;\n"
"uniform float time;\n"
"void main()\n"
"{\n"
"  float sw = 1.0 / width;\n"
"  float sh = 1.0 / height;\n"
"  vec4 s = vec4(0.0);\n"
"  s += texture2D(tex, vtexcoord + vec2(-sw, -sh)) * -1.0 / 8.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(0.0, -sh)) * -1.0 / 8.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(sw, -sh)) * -1.0 / 8.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(-sw, 0.0)) * -1.0 / 8.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(0.0, 0.0)) * 1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(sw, 0.0)) * -1.0 / 8.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(-sw, sh)) * -1.0 / 8.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(0.0, sh)) * -1.0 / 8.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(sw, sh)) * -1.0 / 8.0;\n"
"  gl_FragColor = clamp(s / 0.1 + 0.0, 0.0, 1.0);\n"
"}\n";

static const char GLFILTER_EMBOSS_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D tex;\n"
"uniform float width;\n"
"uniform float height;\n"
"uniform float time;\n"
"void main()\n"
"{\n"
"  float sw = 1.0 / width;\n"
"  float sh = 1.0 / height;\n"
"  vec4 s = vec4(0.0);\n"
"  s += texture2D(tex, vtexcoord + vec2(-sw, -sh)) * 2.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(0.0, -sh)) * 0.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(sw, -sh)) * 0.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(-sw, 0.0)) * 0.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(0.0, 0.0)) * -1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(sw, 0.0)) * 0.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(-sw, sh)) * 0.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(0.0, sh)) * 0.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(sw, sh)) * -1.0;\n"
"  gl_FragColor = clamp(s / 1.0 + 0.5, 0.0, 1.0);\n"
"}\n";

static const char GLFILTER_SHARPNESS_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D tex;\n"
"uniform float width;\n"
"uniform float height;\n"
"uniform float time;\n"
"void main()\n"
"{\n"
"  float sw = 1.0 / width;\n"
"  float sh = 1.0 / height;\n"
"  vec4 s = vec4(0.0);\n"
"  s += texture2D(tex, vtexcoord + vec2(-sw, -sh)) * -1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(0.0, -sh)) * -1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(sw, -sh)) * -1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(-sw, 0.0)) * -1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(0.0, 0.0)) * 9.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(sw, 0.0)) * -1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(-sw, sh)) * -1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(0.0, sh)) * -1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(sw, sh)) * -1.0;\n"
"  gl_FragColor = clamp(s / 1.0 + 0.0, 0.0, 1.0);\n"
"}\n";

struct glfilter *nemofx_glfilter_create(int32_t width, int32_t height)
{
	struct glfilter *filter;

	filter = (struct glfilter *)malloc(sizeof(struct glfilter));
	if (filter == NULL)
		return NULL;
	memset(filter, 0, sizeof(struct glfilter));

	filter->width = width;
	filter->height = height;

	return filter;
}

void nemofx_glfilter_destroy(struct glfilter *filter)
{
	if (filter->texture > 0)
		glDeleteTextures(1, &filter->texture);
	if (filter->fbo > 0)
		glDeleteFramebuffers(1, &filter->fbo);
	if (filter->dbo > 0)
		glDeleteRenderbuffers(1, &filter->dbo);

	if (filter->vshader > 0)
		glDeleteShader(filter->vshader);
	if (filter->fshader > 0)
		glDeleteShader(filter->fshader);
	if (filter->program > 0)
		glDeleteProgram(filter->program);

	free(filter);
}

void nemofx_glfilter_use_fbo(struct glfilter *filter)
{
	filter->texture = gl_create_texture(GL_LINEAR, GL_CLAMP_TO_EDGE, filter->width, filter->height);

	gl_create_fbo(filter->texture, filter->width, filter->height, &filter->fbo, &filter->dbo);
}

void nemofx_glfilter_set_program(struct glfilter *filter, const char *shaderpath)
{
	if (filter->program > 0) {
		glDeleteShader(filter->vshader);
		glDeleteShader(filter->fshader);
		glDeleteProgram(filter->program);

		filter->vshader = 0;
		filter->fshader = 0;
		filter->program = 0;
	}

	if (shaderpath[0] != '@') {
		char *shader;
		int size;

		if (os_load_path(shaderpath, &shader, &size) >= 0) {
			filter->program = gl_compile_program(GLFILTER_SIMPLE_VERTEX_SHADER, shader, &filter->vshader, &filter->fshader);

			free(shader);
		}
	} else {
		const char *shader = NULL;

		if (strcmp(shaderpath, "@gaussian") == 0)
			shader = GLFILTER_GAUSSIAN_FRAGMENT_SHADER;
		else if (strcmp(shaderpath, "@laplacian") == 0)
			shader = GLFILTER_LAPLACIAN_FRAGMENT_SHADER;
		else if (strcmp(shaderpath, "@sharpness") == 0)
			shader = GLFILTER_SHARPNESS_FRAGMENT_SHADER;
		else if (strcmp(shaderpath, "@edgedetection") == 0)
			shader = GLFILTER_EDGEDETECTION_FRAGMENT_SHADER;
		else if (strcmp(shaderpath, "@emboss") == 0)
			shader = GLFILTER_EMBOSS_FRAGMENT_SHADER;

		if (shader != NULL)
			filter->program = gl_compile_program(GLFILTER_SIMPLE_VERTEX_SHADER, shader, &filter->vshader, &filter->fshader);
	}

	if (filter->program > 0) {
		glUseProgram(filter->program);
		glBindAttribLocation(filter->program, 0, "position");
		glBindAttribLocation(filter->program, 1, "texcoord");

		filter->utexture = glGetUniformLocation(filter->program, "tex");
		filter->uwidth = glGetUniformLocation(filter->program, "width");
		filter->uheight = glGetUniformLocation(filter->program, "height");
		filter->utime = glGetUniformLocation(filter->program, "time");
	}
}

void nemofx_glfilter_resize(struct glfilter *filter, int32_t width, int32_t height)
{
	if (filter->width == width && filter->height == height)
		return;

	if (filter->texture > 0) {
		glBindTexture(GL_TEXTURE_2D, filter->texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		glDeleteFramebuffers(1, &filter->fbo);
		glDeleteRenderbuffers(1, &filter->dbo);

		gl_create_fbo(filter->texture, width, height, &filter->fbo, &filter->dbo);
	}

	filter->width = width;
	filter->height = height;
}

uint32_t nemofx_glfilter_dispatch(struct glfilter *filter, uint32_t texture)
{
	static GLfloat vertices[] = {
		-1.0f, -1.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 1.0f
	};

	if (filter->program <= 0)
		return texture;

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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[2]);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return filter->texture;
}

int32_t nemofx_glfilter_get_width(struct glfilter *filter)
{
	return filter->width;
}

int32_t nemofx_glfilter_get_height(struct glfilter *filter)
{
	return filter->height;
}

uint32_t nemofx_glfilter_get_texture(struct glfilter *filter)
{
	return filter->texture;
}
