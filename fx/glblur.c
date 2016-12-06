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

#include <glblur.h>
#include <glhelper.h>
#include <oshelper.h>
#include <nemomisc.h>

struct glblur {
	GLuint texture[2];
	GLuint fbo[2], dbo[2];

	GLuint vshader;
	GLuint fshader;
	GLuint program;

	GLint utexture;
	GLint uwidth, uheight;
	GLint udirectx, udirecty;
	GLint uradius;

	int32_t width, height;

	int32_t rx, ry;
};

static const char GLBLUR_SIMPLE_VERTEX_SHADER[] =
"attribute vec2 position;\n"
"attribute vec2 texcoord;\n"
"varying vec2 vtexcoord;\n"
"void main()\n"
"{\n"
"  gl_Position = vec4(position, 0.0, 1.0);\n"
"  vtexcoord = texcoord;\n"
"}\n";

static const char GLBLUR_GAUSSIAN_9TAP_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D tex;\n"
"uniform float width;\n"
"uniform float height;\n"
"uniform float dx;\n"
"uniform float dy;\n"
"uniform float r;\n"
"void main()\n"
"{\n"
"  vec4 sum = vec4(0.0);\n"
"  sum += texture2D(tex, vec2(vtexcoord.x - 4.0 * r * dx, vtexcoord.y - 4.0 * r * dy)) * 0.0162162162;\n"
"  sum += texture2D(tex, vec2(vtexcoord.x - 3.0 * r * dx, vtexcoord.y - 3.0 * r * dy)) * 0.0540540541;\n"
"  sum += texture2D(tex, vec2(vtexcoord.x - 2.0 * r * dx, vtexcoord.y - 2.0 * r * dy)) * 0.1216216216;\n"
"  sum += texture2D(tex, vec2(vtexcoord.x - 1.0 * r * dx, vtexcoord.y - 1.0 * r * dy)) * 0.1945945946;\n"
"  sum += texture2D(tex, vec2(vtexcoord.x, vtexcoord.y)) * 0.2270270270;\n"
"  sum += texture2D(tex, vec2(vtexcoord.x + 1.0 * r * dx, vtexcoord.y + 1.0 * r * dy)) * 0.1945945946;\n"
"  sum += texture2D(tex, vec2(vtexcoord.x + 2.0 * r * dx, vtexcoord.y + 2.0 * r * dy)) * 0.1216216216;\n"
"  sum += texture2D(tex, vec2(vtexcoord.x + 3.0 * r * dx, vtexcoord.y + 3.0 * r * dy)) * 0.0540540541;\n"
"  sum += texture2D(tex, vec2(vtexcoord.x + 4.0 * r * dx, vtexcoord.y + 4.0 * r * dy)) * 0.0162162162;\n"
"  gl_FragColor = sum;\n"
"}\n";

struct glblur *nemofx_glblur_create(int32_t width, int32_t height)
{
	struct glblur *blur;

	blur = (struct glblur *)malloc(sizeof(struct glblur));
	if (blur == NULL)
		return NULL;
	memset(blur, 0, sizeof(struct glblur));

	blur->program = gl_compile_program(GLBLUR_SIMPLE_VERTEX_SHADER, GLBLUR_GAUSSIAN_9TAP_FRAGMENT_SHADER, &blur->vshader, &blur->fshader);
	if (blur->program == 0)
		goto err1;
	glUseProgram(blur->program);
	glBindAttribLocation(blur->program, 0, "position");
	glBindAttribLocation(blur->program, 1, "texcoord");

	blur->utexture = glGetUniformLocation(blur->program, "tex");
	blur->uwidth = glGetUniformLocation(blur->program, "width");
	blur->uheight = glGetUniformLocation(blur->program, "height");
	blur->udirectx = glGetUniformLocation(blur->program, "dx");
	blur->udirecty = glGetUniformLocation(blur->program, "dy");
	blur->uradius = glGetUniformLocation(blur->program, "r");

	blur->texture[0] = gl_create_texture(GL_LINEAR, GL_CLAMP_TO_EDGE, width, height);
	blur->texture[1] = gl_create_texture(GL_LINEAR, GL_CLAMP_TO_EDGE, width, height);

	gl_create_fbo(blur->texture[0], width, height, &blur->fbo[0], &blur->dbo[0]);
	gl_create_fbo(blur->texture[1], width, height, &blur->fbo[1], &blur->dbo[1]);

	blur->width = width;
	blur->height = height;

	return blur;

err1:
	free(blur);

	return NULL;
}

void nemofx_glblur_destroy(struct glblur *blur)
{
	glDeleteTextures(2, &blur->texture[0]);

	glDeleteFramebuffers(1, &blur->fbo[0]);
	glDeleteRenderbuffers(1, &blur->dbo[0]);
	glDeleteFramebuffers(1, &blur->fbo[1]);
	glDeleteRenderbuffers(1, &blur->dbo[1]);

	glDeleteShader(blur->vshader);
	glDeleteShader(blur->fshader);
	glDeleteProgram(blur->program);

	free(blur);
}

void nemofx_glblur_set_radius(struct glblur *blur, int32_t rx, int32_t ry)
{
	blur->rx = rx;
	blur->ry = ry;
}

void nemofx_glblur_resize(struct glblur *blur, int32_t width, int32_t height)
{
	if (blur->width != width || blur->height != height) {
		int i;

		for (i = 0; i < 2; i++) {
			glBindTexture(GL_TEXTURE_2D, blur->texture[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
			glBindTexture(GL_TEXTURE_2D, 0);

			glDeleteFramebuffers(1, &blur->fbo[i]);
			glDeleteRenderbuffers(1, &blur->dbo[i]);

			gl_create_fbo(blur->texture[i], width, height, &blur->fbo[i], &blur->dbo[i]);
		}

		blur->width = width;
		blur->height = height;
	}
}

uint32_t nemofx_glblur_dispatch(struct glblur *blur, uint32_t texture)
{
	static GLfloat vertices[] = {
		-1.0f, -1.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 1.0f
	};

	// first pass
	glBindFramebuffer(GL_FRAMEBUFFER, blur->fbo[0]);

	glViewport(0, 0, blur->width, blur->height);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glUseProgram(blur->program);
	glUniform1i(blur->utexture, 0);
	glUniform1f(blur->uwidth, blur->width);
	glUniform1f(blur->uheight, blur->height);
	glUniform1f(blur->udirectx, 1.0f);
	glUniform1f(blur->udirecty, 0.0f);
	glUniform1f(blur->uradius, (float)blur->rx / (float)blur->width);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[2]);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// second pass
	glBindFramebuffer(GL_FRAMEBUFFER, blur->fbo[1]);

	glViewport(0, 0, blur->width, blur->height);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindTexture(GL_TEXTURE_2D, blur->texture[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glUseProgram(blur->program);
	glUniform1i(blur->utexture, 0);
	glUniform1f(blur->uwidth, blur->width);
	glUniform1f(blur->uheight, blur->height);
	glUniform1f(blur->udirectx, 0.0f);
	glUniform1f(blur->udirecty, 1.0f);
	glUniform1f(blur->uradius, (float)blur->ry / (float)blur->height);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[2]);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return blur->texture[1];
}

int32_t nemofx_glblur_get_width(struct glblur *blur)
{
	return blur->width;
}

int32_t nemofx_glblur_get_height(struct glblur *blur)
{
	return blur->height;
}

uint32_t nemofx_glblur_get_texture(struct glblur *blur)
{
	return blur->texture[1];
}
