#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <math.h>

#include <glshadow.h>
#include <glhelper.h>
#include <fbohelper.h>
#include <oshelper.h>
#include <nemomatrix.h>
#include <nemomisc.h>

#define GLSHADOW_MAP_SIZE			(256)

static const char GLSHADOW_COVER_VERTEX_SHADER[] =
"attribute vec2 position;\n"
"attribute vec2 texcoord;\n"
"varying vec2 vtexcoord;\n"
"void main()\n"
"{\n"
"  gl_Position = vec4(position, 0.0, 1.0);\n"
"  vtexcoord = texcoord;\n"
"}\n";

static const char GLSHADOW_COVER_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D texture;\n"
"void main()\n"
"{\n"
"  gl_FragColor = texture2D(texture, vtexcoord);\n"
"}\n";

static const char GLSHADOW_OCCLUDE_VERTEX_SHADER[] =
"uniform mat4 projection;\n"
"attribute vec2 position;\n"
"attribute vec2 texcoord;\n"
"varying vec2 vtexcoord;\n"
"void main()\n"
"{\n"
"  gl_Position = projection * vec4(position, 0.0, 1.0);\n"
"  vtexcoord = texcoord;\n"
"}\n";

static const char GLSHADOW_OCCLUDE_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D texture;\n"
"void main()\n"
"{\n"
"  gl_FragColor = texture2D(texture, vtexcoord);\n"
"}\n";

static const char GLSHADOW_MAP_VERTEX_SHADER[] =
"attribute vec2 position;\n"
"attribute vec2 texcoord;\n"
"varying vec2 vtexcoord;\n"
"void main()\n"
"{\n"
"  gl_Position = vec4(position, 0.0, 1.0);\n"
"  vtexcoord = texcoord;\n"
"}\n";

static const char GLSHADOW_MAP_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D texture;\n"
"uniform int width;\n"
"uniform int height;\n"
"void main()\n"
"{\n"
"  float distance = 1.0;\n"
"  for (int y = 0; y < height; y += 1) {\n"
"    vec2 norm = vec2(vtexcoord.x, float(y) / float(height)) * 2.0 - 1.0;\n"
"    float theta = 3.14 * 1.5 + norm.x * 3.14;\n"
"    float r = (1.0 + norm.y) * 0.5;\n"
"    vec2 coord = vec2(-r * sin(theta), r * cos(theta)) / 2.0 + 0.5;\n"
"    vec4 color = texture2D(texture, coord);\n"
"    if (color.a > 0.75)\n"
"      distance = min(distance, float(y) / float(height));\n"
"  }\n"
"  gl_FragColor = vec4(vec3(distance), 1.0);\n"
"}\n";

static const char GLSHADOW_RENDER_VERTEX_SHADER[] =
"uniform mat4 projection;\n"
"attribute vec2 position;\n"
"attribute vec2 texcoord;\n"
"varying vec2 vtexcoord;\n"
"void main()\n"
"{\n"
"  gl_Position = projection * vec4(position, 0.0, 1.0);\n"
"  vtexcoord = texcoord;\n"
"}\n";

static const char GLSHADOW_RENDER_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D tshadow;\n"
"uniform int width;\n"
"uniform int height;\n"
"uniform vec3 lcolor;\n"
"uniform float lsize;\n"
"void main()\n"
"{\n"
"  vec2 norm = vtexcoord * 2.0 - 1.0;\n"
"  float theta = atan(norm.y, norm.x);\n"
"  float r = length(norm);\n"
"  float coord = (theta + 3.14) / (2.0 * 3.14);\n"
"  vec2 tc = vec2(coord, 0.0);\n"
"  float center = step(r, texture2D(tshadow, tc).r);\n"
"  float blur = (1.0 / float(width)) * smoothstep(0.0, 1.0, r);\n"
"  float sum = 0.0;\n"
"  sum += step(r, texture2D(tshadow, vec2(tc.x - 4.0 * blur, tc.y)).r) * 0.05;\n"
"  sum += step(r, texture2D(tshadow, vec2(tc.x - 3.0 * blur, tc.y)).r) * 0.09;\n"
"  sum += step(r, texture2D(tshadow, vec2(tc.x - 2.0 * blur, tc.y)).r) * 0.12;\n"
"  sum += step(r, texture2D(tshadow, vec2(tc.x - 1.0 * blur, tc.y)).r) * 0.15;\n"
"  sum += center * 0.16;\n"
"  sum += step(r, texture2D(tshadow, vec2(tc.x + 1.0 * blur, tc.y)).r) * 0.15;\n"
"  sum += step(r, texture2D(tshadow, vec2(tc.x + 2.0 * blur, tc.y)).r) * 0.12;\n"
"  sum += step(r, texture2D(tshadow, vec2(tc.x + 3.0 * blur, tc.y)).r) * 0.09;\n"
"  sum += step(r, texture2D(tshadow, vec2(tc.x + 4.0 * blur, tc.y)).r) * 0.05;\n"
"  float lit = mix(center, sum, 1.0) * smoothstep(1.0, 0.0, r);\n"
"  gl_FragColor = vec4(lcolor * lit, lit);\n"
"}\n";

static GLuint glshadow_create_program(const char *vshader, const char *fshader)
{
	GLuint frag, vert;
	GLuint program;
	GLint status;

	frag = glshader_compile(GL_FRAGMENT_SHADER, 1, &fshader);
	vert = glshader_compile(GL_VERTEX_SHADER, 1, &vshader);

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

struct glshadow *glshadow_create(int32_t width, int32_t height, int32_t lightscope)
{
	struct glshadow *shadow;

	shadow = (struct glshadow *)malloc(sizeof(struct glshadow));
	if (shadow == NULL)
		return NULL;
	memset(shadow, 0, sizeof(struct glshadow));

	glGenTextures(1, &shadow->texture);
	glBindTexture(GL_TEXTURE_2D, shadow->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &shadow->occluder);
	glBindTexture(GL_TEXTURE_2D, shadow->occluder);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, lightscope, lightscope, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &shadow->shadow);
	glBindTexture(GL_TEXTURE_2D, shadow->shadow);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, GLSHADOW_MAP_SIZE, 1, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	shadow->program0 = glshadow_create_program(GLSHADOW_COVER_VERTEX_SHADER, GLSHADOW_COVER_FRAGMENT_SHADER);
	if (shadow->program0 == 0)
		goto err1;
	shadow->program1 = glshadow_create_program(GLSHADOW_OCCLUDE_VERTEX_SHADER, GLSHADOW_OCCLUDE_FRAGMENT_SHADER);
	if (shadow->program1 == 0)
		goto err2;
	shadow->program2 = glshadow_create_program(GLSHADOW_MAP_VERTEX_SHADER, GLSHADOW_MAP_FRAGMENT_SHADER);
	if (shadow->program2 == 0)
		goto err3;
	shadow->program3 = glshadow_create_program(GLSHADOW_RENDER_VERTEX_SHADER, GLSHADOW_RENDER_FRAGMENT_SHADER);
	if (shadow->program3 == 0)
		goto err4;

	shadow->utexture0 = glGetUniformLocation(shadow->program0, "texture");

	shadow->utexture1 = glGetUniformLocation(shadow->program1, "texture");
	shadow->uprojection1 = glGetUniformLocation(shadow->program1, "projection");

	shadow->utexture2 = glGetUniformLocation(shadow->program2, "texture");
	shadow->uwidth2 = glGetUniformLocation(shadow->program2, "width");
	shadow->uheight2 = glGetUniformLocation(shadow->program2, "height");

	shadow->ushadow3 = glGetUniformLocation(shadow->program3, "tshadow");
	shadow->uwidth3 = glGetUniformLocation(shadow->program3, "width");
	shadow->uheight3 = glGetUniformLocation(shadow->program3, "height");
	shadow->uprojection3 = glGetUniformLocation(shadow->program3, "projection");
	shadow->ucolor3 = glGetUniformLocation(shadow->program3, "lcolor");
	shadow->usize3 = glGetUniformLocation(shadow->program3, "lsize");

	fbo_prepare_context(shadow->occluder, lightscope, lightscope, &shadow->ofbo, &shadow->odbo);
	fbo_prepare_context(shadow->shadow, GLSHADOW_MAP_SIZE, 1, &shadow->sfbo, &shadow->sdbo);
	fbo_prepare_context(shadow->texture, width, height, &shadow->fbo, &shadow->dbo);

	shadow->width = width;
	shadow->height = height;
	shadow->lightscope = lightscope;

	return shadow;

err4:
	glDeleteProgram(shadow->program2);

err3:
	glDeleteProgram(shadow->program1);

err2:
	glDeleteProgram(shadow->program0);

err1:
	glDeleteTextures(1, &shadow->texture);
	glDeleteTextures(1, &shadow->shadow);

	free(shadow);

	return NULL;
}

void glshadow_destroy(struct glshadow *shadow)
{
	glDeleteTextures(1, &shadow->texture);
	glDeleteFramebuffers(1, &shadow->fbo);
	glDeleteRenderbuffers(1, &shadow->dbo);

	glDeleteTextures(1, &shadow->shadow);
	glDeleteFramebuffers(1, &shadow->sfbo);
	glDeleteRenderbuffers(1, &shadow->sdbo);

	glDeleteTextures(1, &shadow->occluder);
	glDeleteFramebuffers(1, &shadow->ofbo);
	glDeleteRenderbuffers(1, &shadow->odbo);

	glDeleteProgram(shadow->program0);
	glDeleteProgram(shadow->program1);
	glDeleteProgram(shadow->program2);
	glDeleteProgram(shadow->program3);

	free(shadow);
}

void glshadow_set_pointlight_position(struct glshadow *shadow, int index, float x, float y)
{
	shadow->pointlights[index].position[0] = x * 2.0f - 1.0f;
	shadow->pointlights[index].position[1] = y * 2.0f - 1.0f;
	shadow->pointlights[index].position[2] = 0.0f;
}

void glshadow_set_pointlight_color(struct glshadow *shadow, int index, float r, float g, float b)
{
	shadow->pointlights[index].color[0] = r;
	shadow->pointlights[index].color[1] = g;
	shadow->pointlights[index].color[2] = b;
}

void glshadow_set_pointlight_size(struct glshadow *shadow, int index, float size)
{
	shadow->pointlights[index].size = size;
}

void glshadow_clear_pointlights(struct glshadow *shadow)
{
	int i;

	for (i = 0; i < GLSHADOW_POINTLIGHTS_MAX; i++) {
		shadow->pointlights[i].size = 0.0f;
	}
}

void glshadow_resize(struct glshadow *shadow, int32_t width, int32_t height)
{
	if (shadow->width != width || shadow->height != height) {
		glBindTexture(GL_TEXTURE_2D, shadow->texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		glDeleteFramebuffers(1, &shadow->fbo);
		glDeleteRenderbuffers(1, &shadow->dbo);

		fbo_prepare_context(shadow->texture, width, height, &shadow->fbo, &shadow->dbo);

		shadow->width = width;
		shadow->height = height;
	}
}

void glshadow_dispatch(struct glshadow *shadow, GLuint texture)
{
	static GLfloat vertices[] = {
		-1.0f, -1.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 1.0f
	};
	struct nemomatrix matrix;
	int i;

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	glBindFramebuffer(GL_FRAMEBUFFER, shadow->fbo);

	glViewport(0, 0, shadow->width, shadow->height);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shadow->program0);
	glUniform1i(shadow->utexture0, 0);

	glBindTexture(GL_TEXTURE_2D, texture);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[2]);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	for (i = 0; i < GLSHADOW_POINTLIGHTS_MAX; i++) {
		if (shadow->pointlights[i].size > 0.0f) {
			glBindFramebuffer(GL_FRAMEBUFFER, shadow->ofbo);

			glViewport(0, 0, shadow->lightscope, shadow->lightscope);

			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClearDepth(0.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			nemomatrix_init_identity(&matrix);
			nemomatrix_translate(&matrix,
					-shadow->pointlights[i].position[0],
					-shadow->pointlights[i].position[1]);

			glUseProgram(shadow->program1);
			glUniformMatrix4fv(shadow->uprojection1, 1, GL_FALSE, matrix.d);
			glUniform1i(shadow->utexture1, 0);

			glBindTexture(GL_TEXTURE_2D, texture);

			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[0]);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[2]);
			glEnableVertexAttribArray(1);

			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

			glBindFramebuffer(GL_FRAMEBUFFER, shadow->sfbo);

			glViewport(0, 0, GLSHADOW_MAP_SIZE, 1);

			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClearDepth(0.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glUseProgram(shadow->program2);
			glUniform1i(shadow->utexture2, 0);
			glUniform1i(shadow->uwidth2, shadow->lightscope);
			glUniform1i(shadow->uheight2, shadow->lightscope);

			glBindTexture(GL_TEXTURE_2D, shadow->occluder);

			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[0]);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[2]);
			glEnableVertexAttribArray(1);

			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

			glBindFramebuffer(GL_FRAMEBUFFER, shadow->fbo);

			glViewport(0, 0, shadow->width, shadow->height);

			nemomatrix_init_identity(&matrix);
			nemomatrix_translate(&matrix,
					shadow->pointlights[i].position[0],
					shadow->pointlights[i].position[1]);

			glUseProgram(shadow->program3);
			glUniformMatrix4fv(shadow->uprojection3, 1, GL_FALSE, matrix.d);
			glUniform1i(shadow->ushadow3, 0);
			glUniform1i(shadow->uwidth3, shadow->lightscope);
			glUniform1i(shadow->uheight3, shadow->lightscope);
			glUniform3fv(shadow->ucolor3, 1, shadow->pointlights[i].color);
			glUniform1f(shadow->usize3, shadow->pointlights[i].size);

			glBindTexture(GL_TEXTURE_2D, shadow->shadow);

			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[0]);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[2]);
			glEnableVertexAttribArray(1);

			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		}
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDisable(GL_BLEND);
}
