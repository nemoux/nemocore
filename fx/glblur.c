#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <glblur.h>
#include <glhelper.h>
#include <fbohelper.h>
#include <oshelper.h>
#include <nemomisc.h>

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
"  gl_FragColor = vec4(sum.rgb, 1.0);\n"
"}\n";

static GLuint nemofx_glblur_create_program(const char *shader)
{
	const char *vertexshader = GLBLUR_SIMPLE_VERTEX_SHADER;
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

struct glblur *nemofx_glblur_create(int32_t width, int32_t height)
{
	struct glblur *blur;
	int size;

	blur = (struct glblur *)malloc(sizeof(struct glblur));
	if (blur == NULL)
		return NULL;
	memset(blur, 0, sizeof(struct glblur));

	glGenTextures(2, &blur->texture[0]);

	glBindTexture(GL_TEXTURE_2D, blur->texture[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindTexture(GL_TEXTURE_2D, blur->texture[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	blur->program = nemofx_glblur_create_program(GLBLUR_GAUSSIAN_9TAP_FRAGMENT_SHADER);
	if (blur->program == 0)
		goto err1;

	blur->utexture = glGetUniformLocation(blur->program, "tex");
	blur->uwidth = glGetUniformLocation(blur->program, "width");
	blur->uheight = glGetUniformLocation(blur->program, "height");
	blur->udirectx = glGetUniformLocation(blur->program, "dx");
	blur->udirecty = glGetUniformLocation(blur->program, "dy");
	blur->uradius = glGetUniformLocation(blur->program, "r");

	fbo_prepare_context(blur->texture[0], width, height, &blur->fbo[0], &blur->dbo[0]);
	fbo_prepare_context(blur->texture[1], width, height, &blur->fbo[1], &blur->dbo[1]);

	blur->width = width;
	blur->height = height;

	return blur;

err1:
	glDeleteTextures(2, &blur->texture[0]);

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

			fbo_prepare_context(blur->texture[i], width, height, &blur->fbo[i], &blur->dbo[i]);
		}

		blur->width = width;
		blur->height = height;
	}
}

void nemofx_glblur_dispatch(struct glblur *blur, GLuint texture)
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
}
