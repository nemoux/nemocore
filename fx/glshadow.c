#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <math.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <glshadow.h>
#include <glhelper.h>
#include <nemomatrix.h>
#include <nemomisc.h>

#define GLSHADOW_MAP_SIZE			(256)

struct glshadow {
	GLuint texture;
	GLuint fbo, dbo;

	GLuint occluder;
	GLuint ofbo, odbo;
	GLuint shadow;
	GLuint sfbo, sdbo;

	GLuint vshader0;
	GLuint fshader0;
	GLuint program0;
	GLint utexture0;

	GLuint vshader1;
	GLuint fshader1;
	GLuint program1;
	GLint utexture1;
	GLint uprojection1;

	GLuint vshader2;
	GLuint fshader2;
	GLuint program2;
	GLint utexture2;
	GLint uwidth2;
	GLint uheight2;

	GLuint vshader3;
	GLuint fshader3;
	GLuint program3;
	GLint ushadow3;
	GLint uprojection3;
	GLint uwidth3;
	GLint uheight3;
	GLint ucolor3;
	GLint usize3;

	int32_t width, height;
	int32_t lightscope;

	struct {
		float position[3];
		float color[3];
		float size;
	} pointlights[GLSHADOW_POINTLIGHTS_MAX];
};

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

struct glshadow *nemofx_glshadow_create(int32_t width, int32_t height, int32_t lightscope)
{
	struct glshadow *shadow;

	shadow = (struct glshadow *)malloc(sizeof(struct glshadow));
	if (shadow == NULL)
		return NULL;
	memset(shadow, 0, sizeof(struct glshadow));

	shadow->program0 = gl_compile_program(GLSHADOW_COVER_VERTEX_SHADER, GLSHADOW_COVER_FRAGMENT_SHADER, &shadow->vshader0, &shadow->fshader0);
	if (shadow->program0 == 0)
		goto err1;
	shadow->program1 = gl_compile_program(GLSHADOW_OCCLUDE_VERTEX_SHADER, GLSHADOW_OCCLUDE_FRAGMENT_SHADER, &shadow->vshader1, &shadow->fshader1);
	if (shadow->program1 == 0)
		goto err2;
	shadow->program2 = gl_compile_program(GLSHADOW_MAP_VERTEX_SHADER, GLSHADOW_MAP_FRAGMENT_SHADER, &shadow->vshader2, &shadow->fshader2);
	if (shadow->program2 == 0)
		goto err3;
	shadow->program3 = gl_compile_program(GLSHADOW_RENDER_VERTEX_SHADER, GLSHADOW_RENDER_FRAGMENT_SHADER, &shadow->vshader3, &shadow->fshader3);
	if (shadow->program3 == 0)
		goto err4;

	glUseProgram(shadow->program0);
	glBindAttribLocation(shadow->program0, 0, "position");
	glBindAttribLocation(shadow->program0, 1, "texcoord");
	glUseProgram(shadow->program1);
	glBindAttribLocation(shadow->program1, 0, "position");
	glBindAttribLocation(shadow->program1, 1, "texcoord");
	glUseProgram(shadow->program2);
	glBindAttribLocation(shadow->program2, 0, "position");
	glBindAttribLocation(shadow->program2, 1, "texcoord");
	glUseProgram(shadow->program3);
	glBindAttribLocation(shadow->program3, 0, "position");
	glBindAttribLocation(shadow->program3, 1, "texcoord");

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

	shadow->occluder = gl_create_texture(GL_LINEAR, GL_CLAMP_TO_EDGE, lightscope, lightscope);
	shadow->shadow = gl_create_texture(GL_LINEAR, GL_CLAMP_TO_EDGE, GLSHADOW_MAP_SIZE, 1);

	gl_create_fbo(shadow->occluder, lightscope, lightscope, &shadow->ofbo, &shadow->odbo);
	gl_create_fbo(shadow->shadow, GLSHADOW_MAP_SIZE, 1, &shadow->sfbo, &shadow->sdbo);

	shadow->width = width;
	shadow->height = height;
	shadow->lightscope = lightscope;

	return shadow;

err4:
	glDeleteShader(shadow->vshader2);
	glDeleteShader(shadow->fshader2);
	glDeleteProgram(shadow->program2);

err3:
	glDeleteShader(shadow->vshader1);
	glDeleteShader(shadow->fshader1);
	glDeleteProgram(shadow->program1);

err2:
	glDeleteShader(shadow->vshader0);
	glDeleteShader(shadow->fshader0);
	glDeleteProgram(shadow->program0);

err1:
	free(shadow);

	return NULL;
}

void nemofx_glshadow_destroy(struct glshadow *shadow)
{
	if (shadow->texture > 0)
		glDeleteTextures(1, &shadow->texture);
	if (shadow->fbo > 0)
		glDeleteFramebuffers(1, &shadow->fbo);
	if (shadow->dbo > 0)
		glDeleteRenderbuffers(1, &shadow->dbo);

	glDeleteTextures(1, &shadow->shadow);
	glDeleteFramebuffers(1, &shadow->sfbo);
	glDeleteRenderbuffers(1, &shadow->sdbo);

	glDeleteTextures(1, &shadow->occluder);
	glDeleteFramebuffers(1, &shadow->ofbo);
	glDeleteRenderbuffers(1, &shadow->odbo);

	glDeleteShader(shadow->vshader0);
	glDeleteShader(shadow->fshader0);
	glDeleteProgram(shadow->program0);
	glDeleteShader(shadow->vshader1);
	glDeleteShader(shadow->fshader1);
	glDeleteProgram(shadow->program1);
	glDeleteShader(shadow->vshader2);
	glDeleteShader(shadow->fshader2);
	glDeleteProgram(shadow->program2);
	glDeleteShader(shadow->vshader3);
	glDeleteShader(shadow->fshader3);
	glDeleteProgram(shadow->program3);

	free(shadow);
}

void nemofx_glshadow_use_fbo(struct glshadow *shadow)
{
	shadow->texture = gl_create_texture(GL_LINEAR, GL_CLAMP_TO_EDGE, shadow->width, shadow->height);

	gl_create_fbo(shadow->texture, shadow->width, shadow->height, &shadow->fbo, &shadow->dbo);
}

void nemofx_glshadow_set_pointlight_position(struct glshadow *shadow, int index, float x, float y)
{
	shadow->pointlights[index].position[0] = x * 2.0f - 1.0f;
	shadow->pointlights[index].position[1] = y * 2.0f - 1.0f;
	shadow->pointlights[index].position[2] = 0.0f;
}

void nemofx_glshadow_set_pointlight_color(struct glshadow *shadow, int index, float r, float g, float b)
{
	shadow->pointlights[index].color[0] = r;
	shadow->pointlights[index].color[1] = g;
	shadow->pointlights[index].color[2] = b;
}

void nemofx_glshadow_set_pointlight_size(struct glshadow *shadow, int index, float size)
{
	shadow->pointlights[index].size = size;
}

void nemofx_glshadow_clear_pointlights(struct glshadow *shadow)
{
	int i;

	for (i = 0; i < GLSHADOW_POINTLIGHTS_MAX; i++) {
		shadow->pointlights[i].size = 0.0f;
	}
}

void nemofx_glshadow_resize(struct glshadow *shadow, int32_t width, int32_t height)
{
	if (shadow->width == width && shadow->height == height)
		return;

	if (shadow->texture > 0) {
		glBindTexture(GL_TEXTURE_2D, shadow->texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		glDeleteFramebuffers(1, &shadow->fbo);
		glDeleteRenderbuffers(1, &shadow->dbo);

		gl_create_fbo(shadow->texture, width, height, &shadow->fbo, &shadow->dbo);
	}

	shadow->width = width;
	shadow->height = height;
}

uint32_t nemofx_glshadow_dispatch(struct glshadow *shadow, uint32_t texture)
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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
			nemomatrix_scale(&matrix,
					(float)shadow->width / (float)shadow->lightscope,
					(float)shadow->height / (float)shadow->lightscope);

			glUseProgram(shadow->program1);
			glUniformMatrix4fv(shadow->uprojection1, 1, GL_FALSE, nemomatrix_get_array(&matrix));
			glUniform1i(shadow->utexture1, 0);

			glBindTexture(GL_TEXTURE_2D, texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[0]);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[2]);
			glEnableVertexAttribArray(1);

			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

			glBindFramebuffer(GL_FRAMEBUFFER, shadow->fbo);

			glViewport(0, 0, shadow->width, shadow->height);

			nemomatrix_init_identity(&matrix);
			nemomatrix_scale(&matrix,
					(float)shadow->lightscope / (float)shadow->width,
					(float)shadow->lightscope / (float)shadow->height);
			nemomatrix_translate(&matrix,
					shadow->pointlights[i].position[0],
					shadow->pointlights[i].position[1]);

			glUseProgram(shadow->program3);
			glUniformMatrix4fv(shadow->uprojection3, 1, GL_FALSE, nemomatrix_get_array(&matrix));
			glUniform1i(shadow->ushadow3, 0);
			glUniform1i(shadow->uwidth3, shadow->lightscope);
			glUniform1i(shadow->uheight3, shadow->lightscope);
			glUniform3fv(shadow->ucolor3, 1, shadow->pointlights[i].color);
			glUniform1f(shadow->usize3, shadow->pointlights[i].size);

			glBindTexture(GL_TEXTURE_2D, shadow->shadow);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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

	return shadow->texture;
}

int32_t nemofx_glshadow_get_width(struct glshadow *shadow)
{
	return shadow->width;
}

int32_t nemofx_glshadow_get_height(struct glshadow *shadow)
{
	return shadow->height;
}

uint32_t nemofx_glshadow_get_texture(struct glshadow *shadow)
{
	return shadow->texture;
}
