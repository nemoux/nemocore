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

#include <glripple.h>
#include <glhelper.h>
#include <fbohelper.h>
#include <nemomisc.h>
#include <nemolist.h>

struct rippleone {
	int gx, gy;
	int delta;
	int duration;
	int step;

	struct nemolist link;
};

struct glripple {
	GLuint texture;
	GLuint fbo, dbo;

	GLuint vshader;
	GLuint fshader;
	GLuint program;

	GLuint varray;
	GLuint vvertex;
	GLuint vtexcoord;
	GLuint vindex;

	GLint utexture;

	GLfloat *vertices;
	GLfloat *vertices0;
	GLfloat *texcoords;
	GLuint *indices;

	int32_t width, height;

	float *vectors;
	float *vectors_;
	int32_t rows, columns;
	int32_t elements;

	float *amplitudes;
	float *amplitudes_;
	int32_t length;

	struct nemolist list;
};

static const char GLRIPPLE_SIMPLE_VERTEX_SHADER[] =
"attribute vec3 position;\n"
"attribute vec2 texcoord;\n"
"varying vec2 vtexcoord;\n"
"void main()\n"
"{\n"
"  gl_Position = vec4(position, 1.0);\n"
"  vtexcoord = texcoord;\n"
"}\n";

static const char GLRIPPLE_SIMPLE_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D tex;\n"
"void main()\n"
"{\n"
"  gl_FragColor = texture2D(tex, vtexcoord);\n"
"}\n";

struct glripple *nemofx_glripple_create(int32_t width, int32_t height)
{
	struct glripple *ripple;

	ripple = (struct glripple *)malloc(sizeof(struct glripple));
	if (ripple == NULL)
		return NULL;
	memset(ripple, 0, sizeof(struct glripple));

	ripple->program = gl_compile_program(GLRIPPLE_SIMPLE_VERTEX_SHADER, GLRIPPLE_SIMPLE_FRAGMENT_SHADER, &ripple->vshader, &ripple->fshader);
	if (ripple->program == 0)
		goto err1;
	glUseProgram(ripple->program);
	glBindAttribLocation(ripple->program, 0, "position");
	glBindAttribLocation(ripple->program, 1, "texcoord");

	ripple->utexture = glGetUniformLocation(ripple->program, "tex");

	glGenTextures(1, &ripple->texture);
	glBindTexture(GL_TEXTURE_2D, ripple->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenVertexArrays(1, &ripple->varray);
	glGenBuffers(1, &ripple->vvertex);
	glGenBuffers(1, &ripple->vtexcoord);
	glGenBuffers(1, &ripple->vindex);

	fbo_prepare_context(ripple->texture, width, height, &ripple->fbo, &ripple->dbo);

	ripple->width = width;
	ripple->height = height;

	nemolist_init(&ripple->list);

	return ripple;

err1:
	free(ripple);

	return NULL;
}

void nemofx_glripple_destroy(struct glripple *ripple)
{
	nemolist_remove(&ripple->list);

	glDeleteTextures(1, &ripple->texture);

	glDeleteBuffers(1, &ripple->vvertex);
	glDeleteBuffers(1, &ripple->vtexcoord);
	glDeleteBuffers(1, &ripple->vindex);
	glDeleteVertexArrays(1, &ripple->varray);

	glDeleteFramebuffers(1, &ripple->fbo);
	glDeleteRenderbuffers(1, &ripple->dbo);

	glDeleteShader(ripple->vshader);
	glDeleteShader(ripple->fshader);
	glDeleteProgram(ripple->program);

	if (ripple->vectors_ != NULL)
		free(ripple->vectors_);
	if (ripple->amplitudes_ != NULL)
		free(ripple->amplitudes_);

	if (ripple->vertices != NULL)
		free(ripple->vertices);
	if (ripple->vertices0 != NULL)
		free(ripple->vertices0);
	if (ripple->texcoords != NULL)
		free(ripple->texcoords);
	if (ripple->indices != NULL)
		free(ripple->indices);

	free(ripple);
}

void nemofx_glripple_use_vectors(struct glripple *ripple, float *vectors, int rows, int columns, int width, int height)
{
	if (vectors != NULL) {
		ripple->vectors = vectors;
	} else {
		ripple->vectors = ripple->vectors_ = (float *)malloc(sizeof(float[3]) * rows * columns);

		nemofx_glripple_build_vectors(ripple->vectors, rows, columns, width, height);
	}

	ripple->rows = rows;
	ripple->columns = columns;
	ripple->elements = rows * (columns + 1) * 2;
}

void nemofx_glripple_use_amplitudes(struct glripple *ripple, float *amplitudes, int length, int cycles, float amplitude)
{
	if (amplitudes != NULL) {
		ripple->amplitudes = amplitudes;
	} else {
		ripple->amplitudes = ripple->amplitudes_ = (float *)malloc(sizeof(float) * length);

		nemofx_glripple_build_amplitudes(ripple->amplitudes, length, cycles, amplitude);
	}

	ripple->length = length;
}

void nemofx_glripple_layout(struct glripple *ripple, int32_t rows, int32_t columns, int32_t length)
{
	GLfloat *vertices;
	GLfloat *vertices0;
	GLfloat *texcoords;
	GLuint *indices;
	GLfloat ox = -1.0f;
	GLfloat oy = -1.0f;
	GLfloat px = 2.0f / columns;
	GLfloat py = 2.0f / rows;
	int idx;
	int x, y;

	vertices = (GLfloat *)malloc(sizeof(GLfloat[3]) * (rows + 1) * (columns + 1));
	vertices0 = (GLfloat *)malloc(sizeof(GLfloat[3]) * (rows + 1) * (columns + 1));
	texcoords = (GLfloat *)malloc(sizeof(GLfloat[2]) * (rows + 1) * (columns + 1));
	indices = (GLuint *)malloc(sizeof(GLuint[2]) * rows * (columns + 1));

	for (y = 0; y <= rows; y++) {
		for (x = 0; x <= columns; x++) {
			idx = y * (columns + 1) + x;

			vertices0[idx * 3 + 0] = vertices[idx * 3 + 0] = ox + (float)x * px;
			vertices0[idx * 3 + 1] = vertices[idx * 3 + 1] = oy + (float)y * py;
			vertices0[idx * 3 + 2] = vertices[idx * 3 + 2] = 0.0f;

			texcoords[idx * 2 + 0] = (float)x / (float)columns;
			texcoords[idx * 2 + 1] = (float)y / (float)rows;
		}
	}

	for (y = 0, idx = 0; y < rows; y++) {
		for (x = 0; x <= columns; x++) {
			indices[idx++] = y * (columns + 1) + x;
			indices[idx++] = (y + 1) * (columns + 1) + x;
		}
	}

	glBindVertexArray(ripple->varray);

	glBindBuffer(GL_ARRAY_BUFFER, ripple->vvertex);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void *)0);
	glEnableVertexAttribArray(0);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat[3]) * (rows + 1) * (columns + 1), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, ripple->vtexcoord);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void *)0);
	glEnableVertexAttribArray(1);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat[2]) * (rows + 1) * (columns + 1), texcoords, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ripple->vindex);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint[2]) * rows * (columns + 1), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	ripple->vertices = vertices;
	ripple->vertices0 = vertices0;
	ripple->texcoords = texcoords;
	ripple->indices = indices;
}

void nemofx_glripple_resize(struct glripple *ripple, int32_t width, int32_t height)
{
	if (ripple->width != width || ripple->height != height) {
		glBindTexture(GL_TEXTURE_2D, ripple->texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		glDeleteFramebuffers(1, &ripple->fbo);
		glDeleteRenderbuffers(1, &ripple->dbo);

		fbo_prepare_context(ripple->texture, width, height, &ripple->fbo, &ripple->dbo);

		ripple->width = width;
		ripple->height = height;
	}
}

void nemofx_glripple_update(struct glripple *ripple)
{
	struct rippleone *one, *next;
	int offset;
	int x, y;

	nemolist_for_each_safe(one, next, &ripple->list, link) {
		if (one->delta > one->duration) {
			nemolist_remove(&one->link);

			free(one);
		} else {
			one->delta += one->step;
		}
	}

	for (y = 1; y < ripple->rows; y++) {
		for (x = 1; x < ripple->columns; x++) {
			offset = y * (ripple->columns + 1) + x;

			ripple->vertices[offset * 3 + 0] = ripple->vertices0[offset * 3 + 0];
			ripple->vertices[offset * 3 + 1] = ripple->vertices0[offset * 3 + 1];

			nemolist_for_each(one, &ripple->list, link) {
				int mx = x - one->gx;
				int my = y - one->gy;
				float sx = 1.0f;
				float sy = 1.0f;
				float amp;
				int r;

				if (mx < 0) {
					mx *= -1;
					sx *= -1;
				}

				if (my < 0) {
					my *= -1;
					sy *= -1;
				}

				r = MINMAX(one->delta - ripple->vectors[(mx * ripple->rows + my) * 3 + 2], 0, ripple->length - 1);
				amp = SQUARE(1.0f - (float)one->delta / (float)ripple->length);

				ripple->vertices[offset * 3 + 0] += ripple->vectors[(mx * ripple->rows + my) * 3 + 0] * sx * ripple->amplitudes[r] * amp;
				ripple->vertices[offset * 3 + 1] += ripple->vectors[(mx * ripple->rows + my) * 3 + 1] * sy * ripple->amplitudes[r] * amp;
			}
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, ripple->vvertex);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat[3]) * (ripple->rows + 1) * (ripple->columns + 1), ripple->vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void nemofx_glripple_dispatch(struct glripple *ripple, uint32_t texture)
{
	glBindFramebuffer(GL_FRAMEBUFFER, ripple->fbo);

	glViewport(0, 0, ripple->width, ripple->height);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(ripple->program);
	glUniform1i(ripple->program, 0);

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindVertexArray(ripple->varray);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ripple->vindex);

	glDrawElements(GL_TRIANGLE_STRIP, ripple->elements, GL_UNSIGNED_INT, (void *)0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glBindTexture(GL_TEXTURE_2D, 0);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void nemofx_glripple_shoot(struct glripple *ripple, float x, float y, int step)
{
	struct rippleone *one;

	one = (struct rippleone *)malloc(sizeof(struct rippleone));
	one->gx = x * ripple->columns;
	one->gy = y * ripple->rows;
	one->delta = 0;
	one->duration = sqrtf(ripple->width * ripple->width + ripple->height * ripple->height) + ripple->length;
	one->step = step;

	nemolist_insert(&ripple->list, &one->link);
}

void nemofx_glripple_build_vectors(float *vectors, int rows, int columns, int width, int height)
{
	float x, y, l;
	int i, j;

	for (i = 0; i < columns; i++) {
		for (j = 0; j < rows; j++) {
			x = (float)i / (float)(columns - 1);
			y = (float)j / (float)(rows - 1);

			l = (float)sqrt(x * x + y * y);

			if (l == 0.0f) {
				x = 0.0f;
				y = 0.0f;
			} else {
				x /= l;
				y /= l;
			}

			vectors[(i * rows + j) * 3 + 0] = x;
			vectors[(i * rows + j) * 3 + 1] = y;
			vectors[(i * rows + j) * 3 + 2] = l * width * 2;
		}
	}
}

void nemofx_glripple_build_amplitudes(float *amplitudes, int length, int cycles, float amplitude)
{
	double t, a;
	int i;

	for (i = 0; i < length; i++) {
		t = 1.0f - i / (length - 1.0f);
		a = (-cos(t * 2.0f * 3.1428571f * cycles) * 0.5f + 0.5f) * amplitude * t * t * t * t * t * t * t * t;
		if (i == 0)
			a = 0.0f;

		amplitudes[i] = a;
	}
}

int32_t nemofx_glripple_get_width(struct glripple *ripple)
{
	return ripple->width;
}

int32_t nemofx_glripple_get_height(struct glripple *ripple)
{
	return ripple->height;
}

int32_t nemofx_glripple_get_rows(struct glripple *ripple)
{
	return ripple->rows;
}

int32_t nemofx_glripple_get_columns(struct glripple *ripple)
{
	return ripple->columns;
}

uint32_t nemofx_glripple_get_texture(struct glripple *ripple)
{
	return ripple->texture;
}
