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

#include <glswirl.h>
#include <glshader.h>
#include <fbohelper.h>
#include <oshelper.h>
#include <nemomisc.h>

struct glswirl {
	GLuint texture;
	GLuint fbo, dbo;

	GLuint program;

	GLint utexture;
	GLint uwidth, uheight;
	GLint uradius;
	GLint uangle;
	GLint ucenter;

	int32_t width, height;

	float radius;
	float angle;
	float center[2];
};

static const char GLSWIRL_VERTEX_SHADER[] =
"attribute vec2 position;\n"
"attribute vec2 texcoord;\n"
"varying vec2 vtexcoord;\n"
"void main()\n"
"{\n"
"  gl_Position = vec4(position, 0.0, 1.0);\n"
"  vtexcoord = texcoord;\n"
"}\n";

static const char GLSWIRL_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D tex;\n"
"uniform float width;\n"
"uniform float height;\n"
"uniform float radius;\n"
"uniform float angle;\n"
"uniform vec2 center;\n"
"void main()\n"
"{\n"
"  vec2 t = vtexcoord - center;\n"
"  float d = length(t);\n"
"  if (d < radius) {\n"
"    float percent = (radius - d) / radius;\n"
"    float theta = percent * percent * angle * 8.0;\n"
"    float s = sin(theta);\n"
"    float c = cos(theta);\n"
"    t = vec2(dot(t, vec2(c, -s)), dot(t, vec2(s, c)));\n"
"  }\n"
"  gl_FragColor = texture2D(tex, t + center);\n"
"}\n";

static GLuint nemofx_glswirl_create_program(const char *shader)
{
	const char *vertexshader = GLSWIRL_VERTEX_SHADER;
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

struct glswirl *nemofx_glswirl_create(int32_t width, int32_t height)
{
	struct glswirl *swirl;
	int size;

	swirl = (struct glswirl *)malloc(sizeof(struct glswirl));
	if (swirl == NULL)
		return NULL;
	memset(swirl, 0, sizeof(struct glswirl));

	glGenTextures(1, &swirl->texture);

	glBindTexture(GL_TEXTURE_2D, swirl->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	swirl->program = nemofx_glswirl_create_program(GLSWIRL_FRAGMENT_SHADER);
	if (swirl->program == 0)
		goto err1;

	swirl->utexture = glGetUniformLocation(swirl->program, "tex");
	swirl->uwidth = glGetUniformLocation(swirl->program, "width");
	swirl->uheight = glGetUniformLocation(swirl->program, "height");
	swirl->uradius = glGetUniformLocation(swirl->program, "radius");
	swirl->uangle = glGetUniformLocation(swirl->program, "angle");
	swirl->ucenter = glGetUniformLocation(swirl->program, "center");

	fbo_prepare_context(swirl->texture, width, height, &swirl->fbo, &swirl->dbo);

	swirl->width = width;
	swirl->height = height;

	return swirl;

err1:
	glDeleteTextures(1, &swirl->texture);

	free(swirl);

	return NULL;
}

void nemofx_glswirl_destroy(struct glswirl *swirl)
{
	glDeleteTextures(1, &swirl->texture);

	glDeleteFramebuffers(1, &swirl->fbo);
	glDeleteRenderbuffers(1, &swirl->dbo);

	glDeleteProgram(swirl->program);

	free(swirl);
}

void nemofx_glswirl_set_radius(struct glswirl *swirl, float radius)
{
	swirl->radius = radius / 2.0f;
}

void nemofx_glswirl_set_angle(struct glswirl *swirl, float angle)
{
	swirl->angle = angle;
}

void nemofx_glswirl_set_center(struct glswirl *swirl, float cx, float cy)
{
	swirl->center[0] = cx / 2.0f + 0.5f;
	swirl->center[1] = cy / 2.0f + 0.5f;
}

void nemofx_glswirl_resize(struct glswirl *swirl, int32_t width, int32_t height)
{
	if (swirl->width != width || swirl->height != height) {
		glBindTexture(GL_TEXTURE_2D, swirl->texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		glDeleteFramebuffers(1, &swirl->fbo);
		glDeleteRenderbuffers(1, &swirl->dbo);

		fbo_prepare_context(swirl->texture, width, height, &swirl->fbo, &swirl->dbo);

		swirl->width = width;
		swirl->height = height;
	}
}

void nemofx_glswirl_dispatch(struct glswirl *swirl, uint32_t texture)
{
	static GLfloat vertices[] = {
		-1.0f, -1.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 1.0f
	};

	glBindFramebuffer(GL_FRAMEBUFFER, swirl->fbo);

	glViewport(0, 0, swirl->width, swirl->height);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(swirl->program);
	glUniform1i(swirl->utexture, 0);
	glUniform1f(swirl->uwidth, swirl->width);
	glUniform1f(swirl->uheight, swirl->height);
	glUniform1f(swirl->uradius, swirl->radius);
	glUniform1f(swirl->uangle, swirl->angle);
	glUniform2fv(swirl->ucenter, 1, swirl->center);

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
}

float nemofx_glswirl_get_radius(struct glswirl *swirl)
{
	return swirl->radius;
}

float nemofx_glswirl_get_angle(struct glswirl *swirl)
{
	return swirl->angle;
}

float nemofx_glswirl_get_center_x(struct glswirl *swirl)
{
	return swirl->center[0];
}

float nemofx_glswirl_get_center_y(struct glswirl *swirl)
{
	return swirl->center[1];
}

uint32_t nemofx_glswirl_get_texture(struct glswirl *swirl)
{
	return swirl->texture;
}
