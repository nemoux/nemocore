#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>
#include <ctype.h>
#include <math.h>
#include <dirent.h>

#include <nemopixs.h>
#include <nemoshow.h>
#include <showhelper.h>
#include <fbohelper.h>
#include <glshader.h>
#include <nemohelper.h>
#include <nemolog.h>
#include <nemomisc.h>

#define NEMOPIXS_GRAVITYWELL_MINIMUM_DISTANCE			(0.18f)
#define NEMOPIXS_ANTIGRAVITY_MINIMUM_DISTANCE			(0.18f)
#define NEMOPIXS_MOVE_MINIMUM_DISTANCE						(0.01f)
#define NEMOPIXS_PIXEL_MINIMUM_DISTANCE						(1.0f)
#define NEMOPIXS_COLOR_MINIMUM_DISTANCE						(0.01f)

#define NEMOPIXS_GRAVITYWELL_SCOPE				(0.5f)
#define NEMOPIXS_ANTIGRAVITY_SCOPE				(0.05f)

#define NEMOPIXS_GRAVITYWELL_INTENSITY		(3.0f)
#define NEMOPIXS_ANTIGRAVITY_INTENSITY		(3.0f)
#define NEMOPIXS_MOVE_INTENSITY						(128.0f)
#define NEMOPIXS_ANGULAR_INTENSITY				(3.0f)
#define NEMOPIXS_COLOR_CHANGE_INTENSITY		(32.0f)
#define NEMOPIXS_COLOR_FADEOUT_INTENSITY	(128.0f)
#define NEMOPIXS_PIXEL_INTENSITY					(128.0f)

#define NEMOPIXS_GRAVITYWELL_FRICTION			(0.5f)
#define NEMOPIXS_ANTIGRAVITY_FRICTION			(2.5f)

#define NEMOPIXS_MOVE_EPSILON							(0.0000001f)
#define NEMOPIXS_ANGULAR_EPSILON					(0.0000001f)
#define NEMOPIXS_COLOR_EPSILON						(0.0000001f)
#define NEMOPIXS_PIXEL_EPSILON						(1.0f)

#define NEMOPIXS_GRAVITYWELL_MINIMUM_COLOR_FORCE		(0.15f)
#define NEMOPIXS_ANTIGRAVITY_MINIMUM_COLOR_FORCE		(0.15f)
#define NEMOPIXS_ANGULAR_MINIMUM_COLOR_FORCE				(0.85f)
#define NEMOPIXS_MOVE_MINIMUM_COLOR_FORCE						(0.15f)

#define NEMOPIXS_FENCE_BOUNCE							(0.96f)

#define NEMOPIXS_PIXEL_TIMEOUT						(100)

#define NEMOPIXS_ACTION_TAPMAX						(3)

static struct pixsfence *nemopixs_fence_create(struct showone *canvas)
{
	struct pixsfence *fence;

	fence = (struct pixsfence *)malloc(sizeof(struct pixsfence));
	if (fence == NULL)
		return NULL;
	memset(fence, 0, sizeof(struct pixsfence));

	fence->canvas = canvas;

	fence->pixels = (uint8_t *)nemoshow_canvas_map(canvas);

	fence->width = nemoshow_canvas_get_viewport_width(canvas);
	fence->height = nemoshow_canvas_get_viewport_height(canvas);

	return fence;
}

static void nemopixs_fence_destroy(struct pixsfence *fence)
{
	nemoshow_canvas_unmap(fence->canvas);

	free(fence);
}

static int nemopixs_fence_contain_pixel(struct pixsfence *fence, float x, float y, float minimum_alpha)
{
	int ix = (x + 1.0f) / 2.0f * fence->width;
	int iy = (y + 1.0f) / 2.0f * fence->height;

	return fence->pixels[(iy * fence->width) * 4 + ix * 4 + 3] > minimum_alpha * 255;
}

static struct pixsone *nemopixs_one_create(int32_t width, int32_t height, int32_t columns, int32_t rows)
{
	struct pixsone *one;
	float pixsize = MIN((float)width / (float)columns, (float)height / (float)rows);
	int i;

	one = (struct pixsone *)malloc(sizeof(struct pixsone));
	if (one == NULL)
		return NULL;
	memset(one, 0, sizeof(struct pixsone));

	one->rows = rows;
	one->columns = columns;
	one->pixsize = pixsize;

	one->vertices = (float *)malloc(sizeof(float[3]) * rows * columns);
	one->velocities = (float *)malloc(sizeof(float[2]) * rows * columns);
	one->diffuses = (float *)malloc(sizeof(float[4]) * rows * columns);
	one->noises = (float *)malloc(sizeof(float) * rows * columns);
	one->sleeps = (float *)malloc(sizeof(float) * rows * columns);
	one->vertices0 = (float *)malloc(sizeof(float[3]) * rows * columns);
	one->diffuses0 = (float *)malloc(sizeof(float[4]) * rows * columns);
	one->positions0 = (float *)malloc(sizeof(float[2]) * rows * columns);
	one->pixels0 = (float *)malloc(sizeof(float) * rows * columns);

	for (i = 0; i < rows * columns; i++) {
		one->vertices[i * 3 + 0] = 0.0f;
		one->vertices[i * 3 + 1] = 0.0f;
		one->vertices[i * 3 + 2] = pixsize;

		one->velocities[i * 2 + 0] = 0.0f;
		one->velocities[i * 2 + 1] = 0.0f;

		one->diffuses[i * 4 + 0] = 0.0f;
		one->diffuses[i * 4 + 1] = 0.0f;
		one->diffuses[i * 4 + 2] = 0.0f;
		one->diffuses[i * 4 + 3] = 0.0f;

		one->noises[i] = 1.0f;
		one->sleeps[i] = 0.0f;
	}

	glGenVertexArrays(1, &one->varray);
	glGenBuffers(1, &one->vvertex);
	glGenBuffers(1, &one->vdiffuse);

	return one;
}

static void nemopixs_one_destroy(struct pixsone *one)
{
	glDeleteBuffers(1, &one->vvertex);
	glDeleteBuffers(1, &one->vdiffuse);
	glDeleteVertexArrays(1, &one->varray);

	free(one->vertices);
	free(one->velocities);
	free(one->diffuses);
	free(one->noises);
	free(one->sleeps);
	free(one->vertices0);
	free(one->diffuses0);
	free(one->positions0);
	free(one->pixels0);

	free(one);
}

static inline void nemopixs_one_copy(struct pixsone *one, int s, int d)
{
	one->vertices[d * 3 + 0] = one->vertices[s * 3 + 0];
	one->vertices[d * 3 + 1] = one->vertices[s * 3 + 1];
	one->vertices[d * 3 + 2] = one->vertices[s * 3 + 2];

	one->velocities[d * 2 + 0] = one->velocities[s * 2 + 0];
	one->velocities[d * 2 + 1] = one->velocities[s * 2 + 1];

	one->diffuses[d * 4 + 0] = one->diffuses[s * 4 + 0];
	one->diffuses[d * 4 + 1] = one->diffuses[s * 4 + 1];
	one->diffuses[d * 4 + 2] = one->diffuses[s * 4 + 2];
	one->diffuses[d * 4 + 3] = one->diffuses[s * 4 + 3];

	one->noises[d] = one->noises[s];
	one->sleeps[d] = one->sleeps[s];

	one->vertices0[d * 3 + 0] = one->vertices0[s * 3 + 0];
	one->vertices0[d * 3 + 1] = one->vertices0[s * 3 + 1];
	one->vertices0[d * 3 + 2] = one->vertices0[s * 3 + 2];

	one->diffuses0[d * 4 + 0] = one->diffuses0[s * 4 + 0];
	one->diffuses0[d * 4 + 1] = one->diffuses0[s * 4 + 1];
	one->diffuses0[d * 4 + 2] = one->diffuses0[s * 4 + 2];
	one->diffuses0[d * 4 + 3] = one->diffuses0[s * 4 + 3];

	one->positions0[d * 2 + 0] = one->positions0[s * 2 + 0];
	one->positions0[d * 2 + 1] = one->positions0[s * 2 + 1];

	one->pixels0[d] = one->pixels0[s];
}

static inline void nemopixs_one_shuffle(struct pixsone *one)
{
	float d[4];
	float p[2];
	int i, s;

	for (i = 0; i < one->pixscount0; i++) {
		s = random_get_int(0, one->pixscount0 - 1);

		d[0] = one->diffuses0[s * 4 + 0];
		d[1] = one->diffuses0[s * 4 + 1];
		d[2] = one->diffuses0[s * 4 + 2];
		d[3] = one->diffuses0[s * 4 + 3];
		p[0] = one->positions0[s * 2 + 0];
		p[1] = one->positions0[s * 2 + 1];

		one->diffuses0[s * 4 + 0] = one->diffuses0[i * 4 + 0];
		one->diffuses0[s * 4 + 1] = one->diffuses0[i * 4 + 1];
		one->diffuses0[s * 4 + 2] = one->diffuses0[i * 4 + 2];
		one->diffuses0[s * 4 + 3] = one->diffuses0[i * 4 + 3];
		one->positions0[s * 2 + 0] = one->positions0[i * 2 + 0];
		one->positions0[s * 2 + 1] = one->positions0[i * 2 + 1];

		one->diffuses0[i * 4 + 0] = d[0];
		one->diffuses0[i * 4 + 1] = d[1];
		one->diffuses0[i * 4 + 2] = d[2];
		one->diffuses0[i * 4 + 3] = d[3];
		one->positions0[i * 2 + 0] = p[0];
		one->positions0[i * 2 + 1] = p[1];
	}
}

static inline void nemopixs_one_swap(struct pixsone *one, int a, int b)
{
	float d[4];
	float p[2];

	d[0] = one->diffuses0[a * 4 + 0];
	d[1] = one->diffuses0[a * 4 + 1];
	d[2] = one->diffuses0[a * 4 + 2];
	d[3] = one->diffuses0[a * 4 + 3];
	p[0] = one->positions0[a * 2 + 0];
	p[1] = one->positions0[a * 2 + 1];

	one->diffuses0[a * 4 + 0] = one->diffuses0[b * 4 + 0];
	one->diffuses0[a * 4 + 1] = one->diffuses0[b * 4 + 1];
	one->diffuses0[a * 4 + 2] = one->diffuses0[b * 4 + 2];
	one->diffuses0[a * 4 + 3] = one->diffuses0[b * 4 + 3];
	one->positions0[a * 2 + 0] = one->positions0[b * 2 + 0];
	one->positions0[a * 2 + 1] = one->positions0[b * 2 + 1];

	one->diffuses0[b * 4 + 0] = d[0];
	one->diffuses0[b * 4 + 1] = d[1];
	one->diffuses0[b * 4 + 2] = d[2];
	one->diffuses0[b * 4 + 3] = d[3];
	one->positions0[b * 2 + 0] = p[0];
	one->positions0[b * 2 + 1] = p[1];

	one->vertices0[a * 3 + 0] = one->positions0[a * 2 + 0];
	one->vertices0[a * 3 + 1] = one->positions0[a * 2 + 1];

	one->vertices0[b * 3 + 0] = one->positions0[b * 2 + 0];
	one->vertices0[b * 3 + 1] = one->positions0[b * 2 + 1];

	one->is_vertices_dirty = 1;
	one->is_diffuses_dirty = 1;
}

static inline void nemopixs_one_transform(struct pixsone *one, float tx, float ty, float sx, float sy)
{
	int i;

	for (i = 0; i < one->pixscount; i++) {
		one->positions0[i * 2 + 0] = one->positions0[i * 2 + 0] * sx + tx;
		one->positions0[i * 2 + 1] = one->positions0[i * 2 + 1] * sy + ty;
	}
}

static inline void nemopixs_one_jitter(struct pixsone *one, float size)
{
	int i;

	for (i = 0; i < one->pixscount; i++) {
		one->positions0[i * 2 + 0] += random_get_double(-size / 2.0f, size / 2.0f);
		one->positions0[i * 2 + 1] += random_get_double(-size / 2.0f, size / 2.0f);
	}
}

static int nemopixs_one_set_position(struct pixsone *one, int action, int dirty)
{
	float seed;
	int i;

	if (action == 0) {
		for (i = 0; i < one->pixscount; i++) {
			one->vertices[i * 3 + 0] = one->positions0[i * 2 + 0];
			one->vertices[i * 3 + 1] = one->positions0[i * 2 + 1];

			one->vertices0[i * 3 + 0] = one->vertices[i * 3 + 0];
			one->vertices0[i * 3 + 1] = one->vertices[i * 3 + 1];
		}
	} else if (action == 1) {
		for (i = 0; i < one->pixscount; i++) {
			one->vertices[i * 3 + 0] = 0.0f;
			one->vertices[i * 3 + 1] = 0.0f;

			one->vertices0[i * 3 + 0] = one->vertices[i * 3 + 0];
			one->vertices0[i * 3 + 1] = one->vertices[i * 3 + 1];
		}
	} else if (action == 2) {
		for (i = 0; i < one->pixscount; i++) {
			one->vertices[i * 3 + 0] = 1.0f;

			one->vertices0[i * 3 + 0] = one->vertices[i * 3 + 0];
		}
	} else if (action == 3) {
		for (i = 0; i < one->pixscount; i++) {
			one->vertices[i * 3 + 1] = 1.0f;

			one->vertices0[i * 3 + 1] = one->vertices[i * 3 + 1];
		}
	} else if (action == 4) {
		for (i = 0; i < one->pixscount; i++) {
			seed = random_get_double(0.0f, 1.0f);

			one->vertices[i * 3 + 0] = cos(seed * M_PI * 2.0f) * 2.0f;
			one->vertices[i * 3 + 1] = sin(seed * M_PI * 2.0f) * 2.0f;

			one->vertices0[i * 3 + 0] = one->vertices[i * 3 + 0];
			one->vertices0[i * 3 + 1] = one->vertices[i * 3 + 1];
		}
	} else if (action == 5) {
		for (i = 0; i < one->pixscount; i++) {
			one->vertices[i * 3 + 0] = random_get_double(-1.0f, 1.0f);
			one->vertices[i * 3 + 1] = random_get_double(-1.0f, 1.0f);

			one->vertices0[i * 3 + 0] = one->vertices[i * 3 + 0];
			one->vertices0[i * 3 + 1] = one->vertices[i * 3 + 1];
		}
	}

	one->is_vertices_dirty = dirty;

	return 0;
}

static int nemopixs_one_set_diffuse(struct pixsone *one, struct showone *canvas, float minimum_alpha)
{
	uint8_t *pixels;
	int idx = 0;
	int x, y;

	pixels = (uint8_t *)nemoshow_canvas_map(canvas);

	for (y = 0; y < one->rows; y++) {
		for (x = 0; x < one->columns; x++) {
			float a = (float)pixels[(y * one->columns) * 4 + x * 4 + 3] / 255.0f;

			if (a >= minimum_alpha) {
				float r = (float)pixels[(y * one->columns) * 4 + x * 4 + 2] / 255.0f;
				float g = (float)pixels[(y * one->columns) * 4 + x * 4 + 1] / 255.0f;
				float b = (float)pixels[(y * one->columns) * 4 + x * 4 + 0] / 255.0f;

				one->diffuses[idx * 4 + 0] = r;
				one->diffuses[idx * 4 + 1] = g;
				one->diffuses[idx * 4 + 2] = b;
				one->diffuses[idx * 4 + 3] = a;

				one->diffuses0[idx * 4 + 0] = r;
				one->diffuses0[idx * 4 + 1] = g;
				one->diffuses0[idx * 4 + 2] = b;
				one->diffuses0[idx * 4 + 3] = a;

				one->positions0[idx * 2 + 0] = ((float)x / (float)one->columns) * 2.0f - 1.0f;
				one->positions0[idx * 2 + 1] = ((float)y / (float)one->rows) * 2.0f - 1.0f;

				idx++;
			}
		}
	}

	nemoshow_canvas_unmap(canvas);

	one->pixscount = idx;
	one->pixscount0 = one->pixscount;

	one->is_diffuses_dirty = 1;

	nemolog_message("PIXS", "[set_diffuse] columns(%d) rows(%d) pixels(%d)\n", one->columns, one->rows, one->pixscount);

	return 0;
}

static int nemopixs_one_set_texture(struct pixsone *one, struct showone *canvas)
{
	int x, y;

	for (y = 0; y < one->rows; y++) {
		for (x = 0; x < one->columns; x++) {
			one->diffuses[(y * one->columns) * 4 + x * 4 + 0] = (float)x / (float)one->columns;
			one->diffuses[(y * one->columns) * 4 + x * 4 + 1] = (float)y / (float)one->rows;

			one->positions0[(y * one->columns) * 2 + x * 2 + 0] = ((float)x / (float)one->columns) * 2.0f - 1.0f;
			one->positions0[(y * one->columns) * 2 + x * 2 + 1] = ((float)y / (float)one->rows) * 2.0f - 1.0f;
		}
	}

	one->pixscount = one->rows * one->columns;
	one->pixscount0 = one->pixscount;

	one->texture = canvas;

	one->is_texcoords_dirty = 1;

	return 0;
}

static int nemopixs_one_set_color(struct pixsone *one, float r, float g, float b, float a)
{
	int x, y;

	one->pixscount = 0;

	for (y = 0; y < one->rows; y++) {
		for (x = 0; x < one->columns; x++) {
			one->diffuses[(y * one->columns) * 4 + x * 4 + 0] = r;
			one->diffuses[(y * one->columns) * 4 + x * 4 + 1] = g;
			one->diffuses[(y * one->columns) * 4 + x * 4 + 2] = b;
			one->diffuses[(y * one->columns) * 4 + x * 4 + 3] = a;

			one->diffuses0[(y * one->columns) * 4 + x * 4 + 0] = r;
			one->diffuses0[(y * one->columns) * 4 + x * 4 + 1] = g;
			one->diffuses0[(y * one->columns) * 4 + x * 4 + 2] = b;
			one->diffuses0[(y * one->columns) * 4 + x * 4 + 3] = a;

			one->positions0[(y * one->columns) * 2 + x * 2 + 0] = ((float)x / (float)one->columns) * 2.0f - 1.0f;
			one->positions0[(y * one->columns) * 2 + x * 2 + 1] = ((float)y / (float)one->rows) * 2.0f - 1.0f;
		}
	}

	one->pixscount = one->rows * one->columns;
	one->pixscount0 = one->pixscount;

	one->is_diffuses_dirty = 1;

	return 0;
}

static int nemopixs_one_set_pixel(struct pixsone *one, float r)
{
	int i;

	for (i = 0; i < one->pixscount; i++) {
		one->vertices[i * 3 + 2] = one->pixsize * r;

		one->pixels0[i] = one->pixsize * r;
	}

	one->is_pixels_dirty = 1;

	return 0;
}

static int nemopixs_one_set_noise(struct pixsone *one, float min, float max)
{
	int i;

	for (i = 0; i < one->pixscount; i++) {
		one->noises[i] = random_get_double(min, max);
	}

	return 0;
}

static int nemopixs_one_set_sleep(struct pixsone *one, float interval)
{
	int i;

	for (i = 0; i < one->pixscount; i++) {
		one->sleeps[i] = i * interval;
	}

	return 0;
}

static int nemopixs_one_set_position_to(struct pixsone *one, int action, int dirty)
{
	float seed;
	float r;
	int i;

	if (action == 0) {
		for (i = 0; i < one->pixscount; i++) {
			one->vertices0[i * 3 + 0] = one->positions0[i * 2 + 0];
			one->vertices0[i * 3 + 1] = one->positions0[i * 2 + 1];
		}
	} else if (action == 1) {
		for (i = 0; i < one->pixscount; i++) {
			one->vertices0[i * 3 + 0] = 0.0f;
			one->vertices0[i * 3 + 1] = 0.0f;
		}
	} else if (action == 2) {
		for (i = 0; i < one->pixscount; i++) {
			one->vertices0[i * 3 + 0] = 1.0f;
		}
	} else if (action == 3) {
		for (i = 0; i < one->pixscount; i++) {
			one->vertices0[i * 3 + 1] = 1.0f;
		}
	} else if (action == 4) {
		for (i = 0; i < one->pixscount; i++) {
			r = M_PI / 2.0f - atan2(one->vertices[i * 3 + 0], one->vertices[i * 3 + 1]);

			one->vertices0[i * 3 + 0] = cos(r) * 2.0f;
			one->vertices0[i * 3 + 1] = sin(r) * 2.0f;
		}
	} else if (action == 5) {
		for (i = 0; i < one->pixscount; i++) {
			one->vertices0[i * 3 + 0] = random_get_double(-1.0f, 1.0f);
			one->vertices0[i * 3 + 1] = random_get_double(-1.0f, 1.0f);
		}
	}

	one->is_vertices_dirty = dirty;

	return 0;
}

static int nemopixs_one_set_diffuse_to(struct pixsone *one, struct showone *canvas, float minimum_alpha)
{
	uint8_t *pixels;
	int idx = 0;
	int x, y;

	pixels = (uint8_t *)nemoshow_canvas_map(canvas);

	for (y = 0; y < one->rows; y++) {
		for (x = 0; x < one->columns; x++) {
			float a = (float)pixels[(y * one->columns) * 4 + x * 4 + 3] / 255.0f;

			if (a >= minimum_alpha) {
				float r = (float)pixels[(y * one->columns) * 4 + x * 4 + 2] / 255.0f;
				float g = (float)pixels[(y * one->columns) * 4 + x * 4 + 1] / 255.0f;
				float b = (float)pixels[(y * one->columns) * 4 + x * 4 + 0] / 255.0f;

				one->diffuses0[idx * 4 + 0] = r;
				one->diffuses0[idx * 4 + 1] = g;
				one->diffuses0[idx * 4 + 2] = b;
				one->diffuses0[idx * 4 + 3] = a;

				one->positions0[idx * 2 + 0] = ((float)x / (float)one->columns) * 2.0f - 1.0f;
				one->positions0[idx * 2 + 1] = ((float)y / (float)one->rows) * 2.0f - 1.0f;

				idx++;
			}
		}
	}

	if (idx < one->pixscount) {
		int i, s;

		for (i = idx; i < one->pixscount; i++) {
			s = random_get_int(0, one->pixscount - 1);

			one->diffuses0[i * 4 + 0] = 0.0f;
			one->diffuses0[i * 4 + 1] = 0.0f;
			one->diffuses0[i * 4 + 2] = 0.0f;
			one->diffuses0[i * 4 + 3] = 0.0f;

			one->positions0[i * 2 + 0] = one->positions0[s * 2 + 0] * one->noises[i];
			one->positions0[i * 2 + 1] = one->positions0[s * 2 + 1] * one->noises[i];
		}
	} else if (idx > one->pixscount) {
		int i, s;

		for (i = one->pixscount; i < idx; i++) {
			s = random_get_int(0, one->pixscount - 1);

			one->vertices[i * 3 + 0] = one->vertices[s * 3 + 0];
			one->vertices[i * 3 + 1] = one->vertices[s * 3 + 1];
			one->vertices[i * 3 + 2] = one->vertices[s * 3 + 2];

			one->diffuses[i * 4 + 0] = one->diffuses[s * 4 + 0];
			one->diffuses[i * 4 + 1] = one->diffuses[s * 4 + 1];
			one->diffuses[i * 4 + 2] = one->diffuses[s * 4 + 2];
			one->diffuses[i * 4 + 3] = one->diffuses[s * 4 + 3];

			one->vertices0[i * 3 + 0] = one->vertices0[s * 3 + 0];
			one->vertices0[i * 3 + 1] = one->vertices0[s * 3 + 1];
			one->vertices0[i * 3 + 2] = one->vertices0[s * 3 + 2];
		}
	}

	nemoshow_canvas_unmap(canvas);

	one->pixscount = MAX(one->pixscount, idx);
	one->pixscount0 = idx;

	one->is_diffuses_dirty = 1;

	nemolog_message("PIXS", "[set_diffuse_to] columns(%d) rows(%d) pixels(%d)\n", one->columns, one->rows, one->pixscount);

	return 0;
}

static int nemopixs_one_set_color_to(struct pixsone *one, float r, float g, float b, float a)
{
	int x, y;

	for (y = 0; y < one->rows; y++) {
		for (x = 0; x < one->columns; x++) {
			one->diffuses0[(y * one->columns) * 4 + x * 4 + 0] = r;
			one->diffuses0[(y * one->columns) * 4 + x * 4 + 1] = g;
			one->diffuses0[(y * one->columns) * 4 + x * 4 + 2] = b;
			one->diffuses0[(y * one->columns) * 4 + x * 4 + 3] = a;

			one->positions0[(y * one->columns) * 2 + x * 2 + 0] = ((float)x / (float)one->columns) * 2.0f - 1.0f;
			one->positions0[(y * one->columns) * 2 + x * 2 + 1] = ((float)y / (float)one->rows) * 2.0f - 1.0f;
		}
	}

	one->pixscount = one->rows * one->columns;
	one->pixscount0 = one->rows * one->columns;

	one->is_diffuses_dirty = 1;

	return 0;
}

static int nemopixs_one_set_alpha_to(struct pixsone *one, float a)
{
	int i;

	for (i = 0; i < one->pixscount; i++) {
		one->diffuses0[i * 4 + 0] = one->diffuses0[i * 4 + 0] * a;
		one->diffuses0[i * 4 + 1] = one->diffuses0[i * 4 + 1] * a;
		one->diffuses0[i * 4 + 2] = one->diffuses0[i * 4 + 2] * a;
		one->diffuses0[i * 4 + 3] = a;
	}

	one->is_diffuses_dirty = 1;

	return 0;
}

static int nemopixs_one_set_pixel_to(struct pixsone *one, float r)
{
	int i;

	for (i = 0; i < one->pixscount; i++) {
		one->pixels0[i] = one->pixsize * r;
	}

	one->is_pixels_dirty = 1;

	return 0;
}

static int nemopixs_update_one(struct nemopixs *pixs, struct pixsone *one, float dt)
{
	int tapcount = nemoshow_event_get_tapcount(&pixs->events);
	int is_updated = 0;

	if (tapcount > 0 && one->is_hidden == 0) {
		struct pixsfence *fence = NULL;

		if (pixs->over != NULL)
			fence = nemopixs_fence_create(pixs->over);

		if (pixs->iactions == 0) {
			int tapmax = MIN(tapcount, pixs->tapmax);
			float intensity = NEMOPIXS_GRAVITYWELL_INTENSITY / (float)tapmax;
			float x, y;
			float x0, y0;
			float dx, dy, dd, ds;
			float f, v, c;
			int i, j, n;

			for (i = 0; i < one->pixscount; i++) {
				for (j = 0; j < tapmax; j++) {
					n = pixs->tapidx = (pixs->tapidx + 1) % tapcount;

					x0 = (nemoshow_event_get_x_on(&pixs->events, n) / (float)pixs->width) * 2.0f - 1.0f;
					y0 = (nemoshow_event_get_y_on(&pixs->events, n) / (float)pixs->height) * 2.0f - 1.0f;

					dx = x0 - one->vertices[i * 3 + 0];
					dy = y0 - one->vertices[i * 3 + 1];
					dd = dx * dx + dy * dy;
					ds = sqrtf(dd + NEMOPIXS_GRAVITYWELL_MINIMUM_DISTANCE);

					if (dd < NEMOPIXS_GRAVITYWELL_SCOPE * one->noises[i]) {
						c = MAX3(one->diffuses[i * 4 + 0], one->diffuses[i * 4 + 1], one->diffuses[i * 4 + 2]);
						c = c * (1.0f - NEMOPIXS_GRAVITYWELL_MINIMUM_COLOR_FORCE) + NEMOPIXS_GRAVITYWELL_MINIMUM_COLOR_FORCE;

						f = intensity * dt / (ds * ds * ds) * c * one->noises[i];

						one->velocities[i * 2 + 0] += dx * f;
						one->velocities[i * 2 + 1] += dy * f;
					}
				}

				one->velocities[i * 2 + 0] -= one->velocities[i * 2 + 0] * NEMOPIXS_GRAVITYWELL_FRICTION * dt;
				one->velocities[i * 2 + 1] -= one->velocities[i * 2 + 1] * NEMOPIXS_GRAVITYWELL_FRICTION * dt;

				x = CLAMP(one->vertices[i * 3 + 0] + one->velocities[i * 2 + 0] * dt, -1.0f, 1.0f);
				y = CLAMP(one->vertices[i * 3 + 1] + one->velocities[i * 2 + 1] * dt, -1.0f, 1.0f);

				if (fence == NULL || nemopixs_fence_contain_pixel(fence, x, y, 0.05f) == 0) {
					one->vertices[i * 3 + 0] = x;
					one->vertices[i * 3 + 1] = y;
				} else {
					v = sqrtf(one->velocities[i * 2 + 0] * one->velocities[i * 2 + 0] + one->velocities[i * 2 + 1] * one->velocities[i * 2 + 1]) * NEMOPIXS_FENCE_BOUNCE;

					dx = one->vertices[i * 3 + 0] - x;
					dy = one->vertices[i * 3 + 1] - y;
					ds = sqrtf(dx * dx + dy * dy);

					one->velocities[i * 2 + 0] = dx / ds * v;
					one->velocities[i * 2 + 1] = dy / ds * v;
				}
			}
		} else if (pixs->iactions == 1) {
			int tapmax = MIN(tapcount, pixs->tapmax);
			float intensity = NEMOPIXS_ANTIGRAVITY_INTENSITY * (float)tapcount / (float)tapmax;
			float x, y;
			float x0, y0;
			float dx, dy, dd, ds;
			float f, v, c;
			int i, j, n;

			for (i = 0; i < one->pixscount; i++) {
				for (j = 0; j < tapmax; j++) {
					n = pixs->tapidx = (pixs->tapidx + 1) % tapcount;

					x0 = (nemoshow_event_get_x_on(&pixs->events, n) / (float)pixs->width) * 2.0f - 1.0f;
					y0 = (nemoshow_event_get_y_on(&pixs->events, n) / (float)pixs->height) * 2.0f - 1.0f;

					dx = x0 - one->vertices[i * 3 + 0];
					dy = y0 - one->vertices[i * 3 + 1];
					dd = dx * dx + dy * dy;
					ds = sqrtf(dd + NEMOPIXS_ANTIGRAVITY_MINIMUM_DISTANCE);

					if (dd < NEMOPIXS_ANTIGRAVITY_SCOPE * one->noises[i]) {
						c = MAX3(one->diffuses[i * 4 + 0], one->diffuses[i * 4 + 1], one->diffuses[i * 4 + 2]);
						c = c * (1.0f - NEMOPIXS_ANTIGRAVITY_MINIMUM_COLOR_FORCE) + NEMOPIXS_ANTIGRAVITY_MINIMUM_COLOR_FORCE;

						f = -intensity * dt / (ds * ds * ds) * c * one->noises[i];

						one->velocities[i * 2 + 0] += dx * f;
						one->velocities[i * 2 + 1] += dy * f;
					}
				}

				one->velocities[i * 2 + 0] -= one->velocities[i * 2 + 0] * NEMOPIXS_ANTIGRAVITY_FRICTION * dt;
				one->velocities[i * 2 + 1] -= one->velocities[i * 2 + 1] * NEMOPIXS_ANTIGRAVITY_FRICTION * dt;

				x = CLAMP(one->vertices[i * 3 + 0] + one->velocities[i * 2 + 0] * dt, -1.0f, 1.0f);
				y = CLAMP(one->vertices[i * 3 + 1] + one->velocities[i * 2 + 1] * dt, -1.0f, 1.0f);

				if (fence == NULL || nemopixs_fence_contain_pixel(fence, x, y, 0.05f) == 0) {
					one->vertices[i * 3 + 0] = x;
					one->vertices[i * 3 + 1] = y;
				} else {
					v = sqrtf(one->velocities[i * 2 + 0] * one->velocities[i * 2 + 0] + one->velocities[i * 2 + 1] * one->velocities[i * 2 + 1]) * NEMOPIXS_FENCE_BOUNCE;

					dx = one->vertices[i * 3 + 0] - x;
					dy = one->vertices[i * 3 + 1] - y;
					ds = sqrtf(dx * dx + dy * dy);

					one->velocities[i * 2 + 0] = dx / ds * v;
					one->velocities[i * 2 + 1] = dy / ds * v;
				}
			}
		}

		one->is_vertices_dirty = 1;

		is_updated = 1;

		if (fence != NULL)
			nemopixs_fence_destroy(fence);
	} else {
		if (one->is_vertices_dirty == 1) {
			int needs_feedback = 0;
			float x0, y0;
			float dx, dy, dd, ds;
			float f, c;
			int i;

			for (i = 0; i < one->pixscount; i++) {
				if (one->sleeps[i] > 0.0f) {
					one->sleeps[i] -= dt;
					needs_feedback = 1;
					continue;
				}

				x0 = one->vertices0[i * 3 + 0];
				y0 = one->vertices0[i * 3 + 1];

				dx = x0 - one->vertices[i * 3 + 0];
				dy = y0 - one->vertices[i * 3 + 1];

				if (dx != 0.0f || dy != 0.0f) {
					dd = dx * dx + dy * dy;

					if (dd > NEMOPIXS_MOVE_EPSILON) {
						c = MAX3(one->diffuses[i * 4 + 0], one->diffuses[i * 4 + 1], one->diffuses[i * 4 + 2]);
						c = c * (1.0f - NEMOPIXS_MOVE_MINIMUM_COLOR_FORCE) + NEMOPIXS_MOVE_MINIMUM_COLOR_FORCE;

						ds = sqrtf(dd + NEMOPIXS_MOVE_MINIMUM_DISTANCE);

						f = NEMOPIXS_MOVE_INTENSITY * dt / ds * c * one->noises[i];

						one->velocities[i * 2 + 0] = dx * f;
						one->velocities[i * 2 + 1] = dy * f;

						one->vertices[i * 3 + 0] = one->vertices[i * 3 + 0] + one->velocities[i * 2 + 0] * dt;
						one->vertices[i * 3 + 1] = one->vertices[i * 3 + 1] + one->velocities[i * 2 + 1] * dt;
					} else {
						one->vertices[i * 3 + 0] = x0;
						one->vertices[i * 3 + 1] = y0;

						one->velocities[i * 2 + 0] = 0.0f;
						one->velocities[i * 2 + 1] = 0.0f;
					}

					needs_feedback = 1;
				}
			}

			if (needs_feedback == 0)
				one->is_vertices_dirty = 0;
			else
				is_updated = 1;
		} else if (one->is_vertices_dirty == 2) {
			int needs_feedback = 0;
			float x0, y0;
			float x1, y1;
			float dx, dy;
			float a0, a1;
			float a, r;
			float f, c;
			int i;

			for (i = 0; i < one->pixscount; i++) {
				if (one->sleeps[i] > 0.0f) {
					one->sleeps[i] -= dt;
					needs_feedback = 1;
					continue;
				}

				x0 = one->vertices[i * 3 + 0];
				y0 = one->vertices[i * 3 + 1];
				x1 = one->vertices0[i * 3 + 0];
				y1 = one->vertices0[i * 3 + 1];

				dx = x1 - x0;
				dy = y1 - y0;

				if (dx != 0.0f || dy != 0.0f) {
					if (dx * dx + dy * dy > NEMOPIXS_ANGULAR_EPSILON) {
						c = MAX3(one->diffuses[i * 4 + 0], one->diffuses[i * 4 + 1], one->diffuses[i * 4 + 2]);
						c = c * (1.0f - NEMOPIXS_ANGULAR_MINIMUM_COLOR_FORCE) + NEMOPIXS_ANGULAR_MINIMUM_COLOR_FORCE;

						a0 = M_PI / 2.0f - atan2(x0, y0);
						a1 = M_PI / 2.0f - atan2(x1, y1);

						f = NEMOPIXS_ANGULAR_INTENSITY * c * one->noises[i];
						r = sqrtf(x1 * x1 + y1 * y1);
						a = a0 + (a1 - a0) * f * dt;

						one->vertices[i * 3 + 0] = cos(a) * r;
						one->vertices[i * 3 + 1] = sin(a) * r;
					} else {
						one->vertices[i * 3 + 0] = x1;
						one->vertices[i * 3 + 1] = y1;

						one->velocities[i * 2 + 0] = 0.0f;
						one->velocities[i * 2 + 1] = 0.0f;
					}

					needs_feedback = 1;
				}
			}

			if (needs_feedback == 0)
				one->is_vertices_dirty = 0;
			else
				is_updated = 1;
		}
	}

	if (one->is_pixels_dirty != 0) {
		int needs_feedback = 0;
		float s0;
		float dd, ds;
		float f;
		int i;

		for (i = 0; i < one->pixscount; i++) {
			if (one->sleeps[i] > 0.0f) {
				one->sleeps[i] -= dt;
				needs_feedback = 1;
				continue;
			}

			s0 = one->pixels0[i];

			dd = s0 - one->vertices[i * 3 + 2];

			if (dd != 0.0f) {
				if (dd > NEMOPIXS_PIXEL_EPSILON) {
					ds = sqrtf(dd + NEMOPIXS_PIXEL_MINIMUM_DISTANCE);

					f = NEMOPIXS_PIXEL_INTENSITY * dt / ds * one->noises[i];

					one->vertices[i * 3 + 2] = one->vertices[i * 3 + 2] + dd * f * dt;
				} else {
					one->vertices[i * 3 + 2] = s0;
				}

				needs_feedback = 1;
			}
		}

		if (needs_feedback == 0)
			one->is_pixels_dirty = 0;
		else
			is_updated = 1;
	}

	if (one->texture == NULL && one->is_diffuses_dirty != 0) {
		int needs_feedback = 0;
		float dd, ds;
		float dr, dg, db, da;
		float r0, g0, b0, a0;
		float f;
		int i;

		for (i = 0; i < one->pixscount0; i++) {
			if (one->sleeps[i] > 0.0f) {
				one->sleeps[i] -= dt;
				needs_feedback = 1;
				continue;
			}

			r0 = one->diffuses0[i * 4 + 0];
			g0 = one->diffuses0[i * 4 + 1];
			b0 = one->diffuses0[i * 4 + 2];
			a0 = one->diffuses0[i * 4 + 3];

			dr = r0 - one->diffuses[i * 4 + 0];
			dg = g0 - one->diffuses[i * 4 + 1];
			db = b0 - one->diffuses[i * 4 + 2];
			da = a0 - one->diffuses[i * 4 + 3];

			if (dr != 0.0f || dg != 0.0f || db != 0.0f || da != 0.0f) {
				dd = dr * dr + dg * dg + db * db + da * da;

				if (dd > NEMOPIXS_COLOR_EPSILON) {
					ds = sqrtf(dd + NEMOPIXS_COLOR_MINIMUM_DISTANCE);

					f = NEMOPIXS_COLOR_CHANGE_INTENSITY * dt / ds * one->noises[i];

					one->diffuses[i * 4 + 0] = one->diffuses[i * 4 + 0] + dr * f * dt;
					one->diffuses[i * 4 + 1] = one->diffuses[i * 4 + 1] + dg * f * dt;
					one->diffuses[i * 4 + 2] = one->diffuses[i * 4 + 2] + db * f * dt;
					one->diffuses[i * 4 + 3] = one->diffuses[i * 4 + 3] + da * f * dt;
				} else {
					one->diffuses[i * 4 + 0] = r0;
					one->diffuses[i * 4 + 1] = g0;
					one->diffuses[i * 4 + 2] = b0;
					one->diffuses[i * 4 + 3] = a0;
				}

				needs_feedback = 1;
			}
		}

		for (i = one->pixscount0; i < one->pixscount; i++) {
			if (one->sleeps[i] > 0.0f) {
				one->sleeps[i] -= dt;
				needs_feedback = 1;
				continue;
			}

			r0 = one->diffuses0[i * 4 + 0];
			g0 = one->diffuses0[i * 4 + 1];
			b0 = one->diffuses0[i * 4 + 2];
			a0 = one->diffuses0[i * 4 + 3];

			dr = r0 - one->diffuses[i * 4 + 0];
			dg = g0 - one->diffuses[i * 4 + 1];
			db = b0 - one->diffuses[i * 4 + 2];
			da = a0 - one->diffuses[i * 4 + 3];

			if (dr != 0.0f || dg != 0.0f || db != 0.0f || da != 0.0f) {
				dd = dr * dr + dg * dg + db * db + da * da;

				if (dd > NEMOPIXS_COLOR_EPSILON) {
					ds = sqrtf(dd + NEMOPIXS_COLOR_MINIMUM_DISTANCE);

					f = NEMOPIXS_COLOR_FADEOUT_INTENSITY * dt / ds * one->noises[i];

					one->diffuses[i * 4 + 0] = one->diffuses[i * 4 + 0] + dr * f * dt;
					one->diffuses[i * 4 + 1] = one->diffuses[i * 4 + 1] + dg * f * dt;
					one->diffuses[i * 4 + 2] = one->diffuses[i * 4 + 2] + db * f * dt;
					one->diffuses[i * 4 + 3] = one->diffuses[i * 4 + 3] + da * f * dt;
				} else {
					one->diffuses[i * 4 + 0] = r0;
					one->diffuses[i * 4 + 1] = g0;
					one->diffuses[i * 4 + 2] = b0;
					one->diffuses[i * 4 + 3] = a0;
				}

				needs_feedback = 1;
			} else {
				if (i < one->pixscount - 1)
					nemopixs_one_copy(one, one->pixscount - 1, i);

				one->pixscount--;
				i--;
			}
		}

		if (needs_feedback == 0)
			one->is_diffuses_dirty = 0;
		else
			is_updated = 1;
	}

	return is_updated;
}

static void nemopixs_dispatch_canvas_redraw(struct nemoshow *show, struct showone *canvas)
{
	struct nemopixs *pixs = (struct nemopixs *)nemoshow_get_userdata(show);
	struct pixsone *one;
	uint32_t msecs = time_current_msecs();
	int is_updated = 0;
	float dt;

	if (pixs->msecs == 0)
		pixs->msecs = msecs;

	dt = (float)(msecs - pixs->msecs) / 1000.0f;

	glBindFramebuffer(GL_FRAMEBUFFER, pixs->fbo);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_POINT_SMOOTH);
	glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

	glViewport(0, 0,
			nemoshow_canvas_get_viewport_width(canvas),
			nemoshow_canvas_get_viewport_height(canvas));

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	one = pixs->one;
	if (one != NULL) {
		if (one->texture == NULL) {
			if (pixs->pointsprite == NULL) {
				glUseProgram(pixs->programs[0]);

				glBindAttribLocation(pixs->programs[0], 0, "position");
				glBindAttribLocation(pixs->programs[0], 1, "diffuse");
			} else {
				glUseProgram(pixs->programs[1]);

				glBindAttribLocation(pixs->programs[1], 0, "position");
				glBindAttribLocation(pixs->programs[1], 1, "diffuse");

				glUniform1i(pixs->usprite1, 0);

				glBindTexture(GL_TEXTURE_2D, nemoshow_canvas_get_texture(pixs->pointsprite));
			}
		} else {
			if (pixs->pointsprite == NULL) {
				glUseProgram(pixs->programs[2]);

				glBindAttribLocation(pixs->programs[2], 0, "position");
				glBindAttribLocation(pixs->programs[2], 1, "diffuse");

				glBindTexture(GL_TEXTURE_2D, nemoshow_canvas_get_texture(one->texture));
			} else {
				glUseProgram(pixs->programs[3]);

				glBindAttribLocation(pixs->programs[3], 0, "position");
				glBindAttribLocation(pixs->programs[3], 1, "diffuse");

				glUniform1i(pixs->utexture3, 0);
				glUniform1i(pixs->usprite3, 1);

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, nemoshow_canvas_get_texture(pixs->pointsprite));
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, nemoshow_canvas_get_texture(one->texture));
			}
		}

		if (nemopixs_update_one(pixs, one, dt) != 0) {
			glBindVertexArray(one->varray);

			if (one->is_vertices_dirty != 0 || one->is_pixels_dirty != 0) {
				glBindBuffer(GL_ARRAY_BUFFER, one->vvertex);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void *)0);
				glEnableVertexAttribArray(0);
				glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat[3]) * one->pixscount, &one->vertices[0], GL_STATIC_DRAW);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}

			if (one->is_diffuses_dirty != 0) {
				glBindBuffer(GL_ARRAY_BUFFER, one->vdiffuse);
				glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *)0);
				glEnableVertexAttribArray(1);
				glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat[4]) * one->pixscount, &one->diffuses[0], GL_STATIC_DRAW);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}

			if (one->is_texcoords_dirty != 0) {
				glBindBuffer(GL_ARRAY_BUFFER, one->vdiffuse);
				glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *)0);
				glEnableVertexAttribArray(1);
				glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat[4]) * one->pixscount, &one->diffuses[0], GL_STATIC_DRAW);
				glBindBuffer(GL_ARRAY_BUFFER, 0);

				one->is_texcoords_dirty = 0;
			}

			glBindVertexArray(0);

			is_updated = 1;
		}

		glBindVertexArray(one->varray);
		glDrawArrays(GL_POINTS, 0, one->pixscount);
		glBindVertexArray(0);
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (is_updated != 0) {
		nemoshow_one_dirty(canvas, NEMOSHOW_REDRAW_DIRTY);

		pixs->msecs = msecs;
	} else {
		nemotimer_set_timeout(pixs->ptimer, NEMOPIXS_PIXEL_TIMEOUT);

		pixs->msecs = 0;
	}
}

static void nemopixs_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, struct showevent *event)
{
	struct nemopixs *pixs = (struct nemopixs *)nemoshow_get_userdata(show);

	if (nemoshow_event_is_pointer_left_down(show, event)) {
		nemopixs_one_set_position_to(pixs->one, random_get_int(1, 5), 1);
		nemopixs_one_set_alpha_to(pixs->one, 0.0f);

		pixs->one->is_hidden = 1;
	} else if (nemoshow_event_is_pointer_right_down(show, event)) {
		if (pixs->one->texture == NULL) {
			pixs->isprites = (pixs->isprites + 1) % pixs->nsprites;
			pixs->iactions = (pixs->iactions + 1) % 2;

			nemopixs_one_set_diffuse_to(pixs->one, pixs->sprites[pixs->isprites], 0.05f);
		}

		nemopixs_one_jitter(pixs->one, pixs->jitter);
		nemopixs_one_set_noise(pixs->one, 0.85f, 1.05f);
		nemopixs_one_set_pixel(pixs->one, pixs->pixsize);
		nemopixs_one_set_position_to(pixs->one, 0, 1);

		pixs->one->is_hidden = 0;
	}

	if (nemoshow_event_is_touch_down(show, event) || nemoshow_event_is_touch_up(show, event) || nemoshow_event_is_touch_motion(show, event)) {
		nemoshow_event_set_type(&pixs->events, NEMOSHOW_TOUCH_EVENT);

		nemoshow_event_update_taps(show, canvas, &pixs->events);

		if (nemoshow_event_is_touch_down(show, event) && nemoshow_event_get_tapcount(&pixs->events) == 1) {
			pixs->has_taps = 1;
		} else if (nemoshow_event_is_touch_up(show, event) && nemoshow_event_get_tapcount(&pixs->events) == 0) {
			pixs->has_taps = 0;

			if (pixs->motion != NULL)
				nemofx_glmotion_clear(pixs->motion);
		}

#if 0
		if (pixs->pointsprite != NULL) {
			if (nemoshow_event_is_touch_down(show, event) && nemoshow_event_get_tapcount(&pixs->events) == 1) {
				nemoshow_transition_dispatch_easy(pixs->show, pixs->pointone, NEMOSHOW_CUBIC_OUT_EASE, 450, 0, 1, "r", 64.0f * 0.45f, 64.0f * 0.25f);
			} else if (nemoshow_event_is_touch_up(show, event) && nemoshow_event_get_tapcount(&pixs->events) == 0) {
				nemoshow_transition_dispatch_easy(pixs->show, pixs->pointone, NEMOSHOW_CUBIC_OUT_EASE, 450, 0, 1, "r", 64.0f * 0.25f, 64.0f * 0.45f);
			}
		}
#endif
	}

	if (nemoshow_event_is_touch_down(show, event) || nemoshow_event_is_touch_up(show, event)) {
		nemoshow_event_update_taps(show, canvas, event);

		if (nemoshow_event_is_more_taps(show, event, 8)) {
			nemoshow_view_pick_distant(show, event, NEMOSHOW_VIEW_PICK_ALL_TYPE);

			nemoshow_event_set_cancel(event);

			nemoshow_dispatch_grab_all(show, event);
		}
	}

	nemoshow_one_dirty(pixs->canvas, NEMOSHOW_REDRAW_DIRTY);
	nemoshow_dispatch_frame(show);

	nemotimer_set_timeout(pixs->stimer, pixs->timeout);
	nemotimer_set_timeout(pixs->ptimer, 0);
}

static void nemopixs_dispatch_show_resize(struct nemoshow *show, int32_t width, int32_t height)
{
	struct nemopixs *pixs = (struct nemopixs *)nemoshow_get_userdata(show);
	int columns, rows;
	int i;

	if (width == height) {
		columns = pixs->pixels;
		rows = pixs->pixels;
	} else if (width > height) {
		columns = pixs->pixels;
		rows = pixs->pixels * (float)height / (float)width;
	} else if (width < height) {
		columns = pixs->pixels * (float)width / (float)height;
		rows = pixs->pixels;
	}

	nemoshow_view_resize(show, width, height);

	glDeleteFramebuffers(1, &pixs->fbo);
	glDeleteRenderbuffers(1, &pixs->dbo);

	fbo_prepare_context(
			nemoshow_canvas_get_texture(pixs->canvas),
			width, height,
			&pixs->fbo, &pixs->dbo);

	for (i = 0; i < pixs->nsprites; i++) {
		nemoshow_canvas_set_viewport(pixs->sprites[i], columns, rows);
		nemoshow_canvas_render(show, pixs->sprites[i]);
	}

	nemopixs_one_destroy(pixs->one);

	pixs->one = nemopixs_one_create(width, height, columns, rows);
	if (pixs->video == NULL)
		nemopixs_one_set_diffuse(pixs->one, pixs->sprites[pixs->isprites], 0.05f);
	else
		nemopixs_one_set_texture(pixs->one, pixs->video);
	nemopixs_one_jitter(pixs->one, pixs->jitter);
	nemopixs_one_set_noise(pixs->one, 0.85f, 1.05f);
	nemopixs_one_set_pixel(pixs->one, pixs->pixsize);
	nemopixs_one_set_position_to(pixs->one, 0, 1);

	nemoshow_one_dirty(pixs->canvas, NEMOSHOW_REDRAW_DIRTY);

	nemoshow_view_redraw(show);
}

static void nemopixs_dispatch_scene_timer(struct nemotimer *timer, void *data)
{
	struct nemopixs *pixs = (struct nemopixs *)data;
	struct pixsone *one = pixs->one;

	if (one->texture == NULL) {
		int width = nemoshow_canvas_get_viewport_width(pixs->canvas);
		int height = nemoshow_canvas_get_viewport_height(pixs->canvas);
		int columns, rows;

		if (width == height) {
			columns = pixs->pixels;
			rows = pixs->pixels;
		} else if (width > height) {
			columns = pixs->pixels;
			rows = pixs->pixels * (float)height / (float)width;
		} else if (width < height) {
			columns = pixs->pixels * (float)width / (float)height;
			rows = pixs->pixels;
		}

		pixs->isprites = (pixs->isprites + 1) % pixs->nsprites;
		pixs->iactions = (pixs->iactions + 1) % 2;

		nemopixs_one_set_diffuse_to(one, pixs->sprites[pixs->isprites], 0.05f);
		nemopixs_one_shuffle(one);
		nemopixs_one_jitter(one, pixs->jitter);
		nemopixs_one_set_noise(one, 0.85f, 1.05f);
		nemopixs_one_set_pixel(one, pixs->pixsize);
		nemopixs_one_set_position_to(one, 0, 2);

		nemoshow_one_dirty(pixs->canvas, NEMOSHOW_REDRAW_DIRTY);
		nemoshow_dispatch_frame(pixs->show);

		nemotimer_set_timeout(pixs->stimer, pixs->timeout);
		nemotimer_set_timeout(pixs->ptimer, 0);
	}
}

static void nemopixs_dispatch_pixel_timer(struct nemotimer *timer, void *data)
{
	struct nemopixs *pixs = (struct nemopixs *)data;
	struct pixsone *one = pixs->one;

	if (one->texture == NULL || one->is_hidden == 0) {
		nemopixs_one_swap(one,
				random_get_int(0, one->pixscount - 1),
				random_get_int(0, one->pixscount - 1));

		nemoshow_one_dirty(pixs->canvas, NEMOSHOW_REDRAW_DIRTY);
		nemoshow_dispatch_frame(pixs->show);

		nemotimer_set_timeout(pixs->ptimer, NEMOPIXS_PIXEL_TIMEOUT);
	}
}

static GLuint nemopixs_dispatch_tale_effect(struct talenode *node, void *data)
{
	struct nemopixs *pixs = (struct nemopixs *)data;
	GLuint texture = nemotale_node_get_texture(node);

	if (pixs->blur != NULL) {
		nemofx_glblur_dispatch(pixs->blur, texture);

		texture = nemofx_glblur_get_texture(pixs->blur);
	}

	if (pixs->motion != NULL && pixs->has_taps != 0) {
		nemofx_glmotion_dispatch(pixs->motion, texture);

		texture = nemofx_glmotion_get_texture(pixs->motion);
	}

	return texture;
}

static int nemopixs_prepare_opengl(struct nemopixs *pixs, int32_t width, int32_t height)
{
	static const char *vertexshader =
		"attribute vec3 position;\n"
		"attribute vec4 diffuse;\n"
		"varying vec4 vdiffuse;\n"
		"void main()\n"
		"{\n"
		"  gl_Position = vec4(position.xy, 0.0, 1.0);\n"
		"  gl_PointSize = position.z;\n"
		"  vdiffuse = diffuse;\n"
		"}\n";
	static const char *vertexshader_texture =
		"attribute vec3 position;\n"
		"attribute vec4 diffuse;\n"
		"varying vec2 vtexcoord;\n"
		"void main()\n"
		"{\n"
		"  gl_Position = vec4(position.xy, 0.0, 1.0);\n"
		"  gl_PointSize = position.z;\n"
		"  vtexcoord = diffuse.xy;\n"
		"}\n";
	static const char *fragmentshader =
		"precision mediump float;\n"
		"varying vec4 vdiffuse;\n"
		"void main()\n"
		"{\n"
		"  gl_FragColor = vdiffuse;\n"
		"}\n";
	static const char *fragmentshader_pointsprite =
		"precision mediump float;\n"
		"uniform sampler2D sprite;\n"
		"varying vec4 vdiffuse;\n"
		"void main()\n"
		"{\n"
		"  gl_FragColor = vdiffuse * texture2D(sprite, gl_PointCoord);\n"
		"}\n";
	static const char *fragmentshader_texture =
		"precision mediump float;\n"
		"uniform sampler2D texture;\n"
		"varying vec2 vtexcoord;\n"
		"void main()\n"
		"{\n"
		"  gl_FragColor = texture2D(texture, vtexcoord);\n"
		"}\n";
	static const char *fragmentshader_texture_pointsprite =
		"precision mediump float;\n"
		"uniform sampler2D texture;\n"
		"uniform sampler2D sprite;\n"
		"varying vec2 vtexcoord;\n"
		"void main()\n"
		"{\n"
		"  gl_FragColor = texture2D(texture, vtexcoord) * texture2D(sprite, gl_PointCoord);\n"
		"}\n";

	fbo_prepare_context(
			nemoshow_canvas_get_texture(pixs->canvas),
			width, height,
			&pixs->fbo, &pixs->dbo);

	pixs->programs[0] = glshader_compile_program(vertexshader, fragmentshader, NULL, NULL);
	pixs->programs[1] = glshader_compile_program(vertexshader, fragmentshader_pointsprite, NULL, NULL);
	pixs->programs[2] = glshader_compile_program(vertexshader_texture, fragmentshader_texture, NULL, NULL);
	pixs->programs[3] = glshader_compile_program(vertexshader_texture, fragmentshader_texture_pointsprite, NULL, NULL);

	pixs->usprite1 = glGetUniformLocation(pixs->programs[1], "sprite");
	pixs->usprite3 = glGetUniformLocation(pixs->programs[3], "sprite");
	pixs->utexture3 = glGetUniformLocation(pixs->programs[3], "texture");

	return 0;
}

static void nemopixs_finish_opengl(struct nemopixs *pixs)
{
	glDeleteFramebuffers(1, &pixs->fbo);
	glDeleteRenderbuffers(1, &pixs->dbo);

	glDeleteProgram(pixs->programs[0]);
	glDeleteProgram(pixs->programs[1]);
	glDeleteProgram(pixs->programs[2]);
	glDeleteProgram(pixs->programs[3]);
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "width",					required_argument,			NULL,			'w' },
		{ "height",					required_argument,			NULL,			'h' },
		{ "framerate",			required_argument,			NULL,			'r' },
		{ "image",					required_argument,			NULL,			'i' },
		{ "video",					required_argument,			NULL,			'v' },
		{ "pixels",					required_argument,			NULL,			'p' },
		{ "jitter",					required_argument,			NULL,			'j' },
		{ "timeout",				required_argument,			NULL,			't' },
		{ "pointsprite",		required_argument,			NULL,			's' },
		{ "background",			required_argument,			NULL,			'b' },
		{ "overlay",				required_argument,			NULL,			'o' },
		{ "fullscreen",			required_argument,			NULL,			'f' },
		{ "pixsize",				required_argument,			NULL,			'x' },
		{ "blursize",				required_argument,			NULL,			'u' },
		{ "motionblur",			required_argument,			NULL,			'm' },
		{ 0 }
	};

	struct nemopixs *pixs;
	struct nemotool *tool;
	struct nemotimer *timer;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct showone *one;
	struct showone *blur;
	struct talenode *node;
	char *imagepath = NULL;
	char *videopath = NULL;
	char *fullscreen = NULL;
	char *pointsprite = NULL;
	char *background = NULL;
	char *overlay = NULL;
	float jitter = 0.0f;
	float pixsize = 1.0f;
	float blursize = 0.0f;
	float motionblur = 0.0f;
	int timeout = 10000;
	int width = 800;
	int height = 800;
	int pixels = 128;
	int fps = 60;
	int opt;

	opterr = 0;

	while (opt = getopt_long(argc, argv, "w:h:r:i:v:p:j:t:s:b:o:f:x:u:m:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'w':
				width = strtoul(optarg, NULL, 10);
				break;

			case 'h':
				height = strtoul(optarg, NULL, 10);
				break;

			case 'r':
				fps = strtoul(optarg, NULL, 10);
				break;

			case 'i':
				imagepath = strdup(optarg);
				break;

			case 'v':
				videopath = strdup(optarg);
				break;

			case 'p':
				pixels = strtoul(optarg, NULL, 10);
				break;

			case 'j':
				jitter = strtod(optarg, NULL);
				break;

			case 't':
				timeout = strtoul(optarg, NULL, 10);
				break;

			case 's':
				pointsprite = strdup(optarg);
				break;

			case 'b':
				background = strdup(optarg);
				break;

			case 'o':
				overlay = strdup(optarg);
				break;

			case 'f':
				fullscreen = strdup(optarg);
				break;

			case 'x':
				pixsize = strtod(optarg, NULL);
				break;

			case 'u':
				blursize = strtod(optarg, NULL);
				break;

			case 'm':
				motionblur = strtod(optarg, NULL);
				break;

			default:
				break;
		}
	}

	pixs = (struct nemopixs *)malloc(sizeof(struct nemopixs));
	if (pixs == NULL)
		goto err1;
	memset(pixs, 0, sizeof(struct nemopixs));

	pixs->width = width;
	pixs->height = height;
	pixs->pixels = pixels;
	pixs->jitter = jitter;
	pixs->pixsize = pixsize;
	pixs->timeout = timeout;
	pixs->tapmax = NEMOPIXS_ACTION_TAPMAX;

	pixs->tool = tool = nemotool_create();
	if (tool == NULL)
		goto err2;
	nemotool_connect_wayland(tool, NULL);

	pixs->show = show = nemoshow_create_view(tool, width, height);
	if (show == NULL)
		goto err3;
	nemoshow_set_filtering_quality(show, NEMOSHOW_FILTER_HIGH_QUALITY);
	nemoshow_set_dispatch_resize(show, nemopixs_dispatch_show_resize);
	nemoshow_set_userdata(show, pixs);

	nemoshow_view_set_framerate(show, fps);

	if (fullscreen != NULL)
		nemoshow_view_set_fullscreen(show, fullscreen);

	pixs->scene = scene = nemoshow_scene_create();
	nemoshow_scene_set_width(scene, width);
	nemoshow_scene_set_height(scene, height);
	nemoshow_set_scene(show, scene);

	if (background != NULL) {
		pixs->back = canvas = nemoshow_canvas_create();
		nemoshow_canvas_set_width(canvas, width);
		nemoshow_canvas_set_height(canvas, height);
		nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
		nemoshow_canvas_set_opaque(canvas, 1);
		nemoshow_one_attach(scene, canvas);

		one = nemoshow_item_create(NEMOSHOW_IMAGE_ITEM);
		nemoshow_one_attach(canvas, one);
		nemoshow_item_set_x(one, 0.0f);
		nemoshow_item_set_y(one, 0.0f);
		nemoshow_item_set_width(one, width);
		nemoshow_item_set_height(one, height);
		nemoshow_item_set_uri(one, background);
	} else {
		pixs->back = canvas = nemoshow_canvas_create();
		nemoshow_canvas_set_width(canvas, width);
		nemoshow_canvas_set_height(canvas, height);
		nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_BACK_TYPE);
		nemoshow_canvas_set_fill_color(canvas, 0.0f, 0.0f, 0.0f, 1.0f);
		nemoshow_one_attach(scene, canvas);
	}

	pixs->canvas = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_OPENGL_TYPE);
	nemoshow_canvas_set_dispatch_redraw(canvas, nemopixs_dispatch_canvas_redraw);
	nemoshow_canvas_set_dispatch_event(canvas, nemopixs_dispatch_canvas_event);
	nemoshow_one_attach(scene, canvas);

	node = nemoshow_canvas_get_node(canvas);
	nemotale_node_set_dispatch_effect(node, nemopixs_dispatch_tale_effect, pixs);

	if (blursize > 0.0f) {
		pixs->blur = nemofx_glblur_create(width, height);
		nemofx_glblur_set_radius(pixs->blur, blursize, blursize);
	}

	if (motionblur > 0.0f) {
		pixs->motion = nemofx_glmotion_create(width, height);
		nemofx_glmotion_set_step(pixs->motion, motionblur);
	}

	if (overlay != NULL) {
		pixs->over = canvas = nemoshow_canvas_create();
		nemoshow_canvas_set_width(canvas, width);
		nemoshow_canvas_set_height(canvas, height);
		nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
		nemoshow_one_attach(scene, canvas);

		if (os_has_file_extension(overlay, "svg", NULL) != 0) {
			one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
			nemoshow_one_attach(canvas, one);
			nemoshow_item_set_x(one, 0.0f);
			nemoshow_item_set_y(one, 0.0f);
			nemoshow_item_set_width(one, width);
			nemoshow_item_set_height(one, height);
			nemoshow_item_set_fill_color(one, 0.0f, 255.0f, 255.0f, 255.0f);
			nemoshow_item_path_load_svg(one, overlay, 0.0f, 0.0f, width, height);
		} else {
			one = nemoshow_item_create(NEMOSHOW_IMAGE_ITEM);
			nemoshow_one_attach(canvas, one);
			nemoshow_item_set_x(one, 0.0f);
			nemoshow_item_set_y(one, 0.0f);
			nemoshow_item_set_width(one, width);
			nemoshow_item_set_height(one, height);
			nemoshow_item_set_uri(one, overlay);
		}
	}

	if (pointsprite != NULL) {
		pixs->pointsprite = canvas = nemoshow_canvas_create();
		nemoshow_canvas_set_width(canvas, 64);
		nemoshow_canvas_set_height(canvas, 64);
		nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
		nemoshow_attach_one(show, canvas);

		if (pointsprite[0] == '@') {
			blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
			nemoshow_filter_set_blur(blur, pointsprite + 1, 64.0f * 0.08f);

			pixs->pointone = one = nemoshow_item_create(NEMOSHOW_CIRCLE_ITEM);
			nemoshow_one_attach(canvas, one);
			nemoshow_item_set_cx(one, 64.0f * 0.5f);
			nemoshow_item_set_cy(one, 64.0f * 0.5f);
			nemoshow_item_set_r(one, 64.0f * 0.42f);
			nemoshow_item_set_fill_color(one, 255.0f, 255.0f, 255.0f, 255.0f);
			nemoshow_item_set_filter(one, blur);
		} else if (os_has_file_extension(pointsprite, "svg", NULL) != 0) {
			blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
			nemoshow_filter_set_blur(blur, "solid", 8.0f);

			pixs->pointone = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
			nemoshow_one_attach(canvas, one);
			nemoshow_item_set_x(one, 0.0f);
			nemoshow_item_set_y(one, 0.0f);
			nemoshow_item_set_width(one, 64.0f);
			nemoshow_item_set_height(one, 64.0f);
			nemoshow_item_set_fill_color(one, 0.0f, 255.0f, 255.0f, 255.0f);
			nemoshow_item_set_filter(one, blur);
			nemoshow_item_path_load_svg(one, pointsprite, 0.0f, 0.0f, 64.0f, 64.0f);
		} else {
			pixs->pointone = one = nemoshow_item_create(NEMOSHOW_IMAGE_ITEM);
			nemoshow_one_attach(canvas, one);
			nemoshow_item_set_x(one, 0.0f);
			nemoshow_item_set_y(one, 0.0f);
			nemoshow_item_set_width(one, 64.0f);
			nemoshow_item_set_height(one, 64.0f);
			nemoshow_item_set_uri(one, pointsprite);
		}
	}

	if (os_check_path_is_directory(imagepath) != 0) {
		struct dirent **entries;
		const char *filename;
		char filepath[128];
		int i, count;

		count = scandir(imagepath, &entries, NULL, alphasort);

		for (i = 0; i < count; i++) {
			filename = entries[i]->d_name;
			if (filename[0] == '.')
				continue;
			if (os_has_file_extension(filename, "svg", "png", "jpg", NULL) == 0)
				continue;

			strcpy(filepath, imagepath);
			strcat(filepath, "/");
			strcat(filepath, filename);

			pixs->sprites[pixs->nsprites++] = canvas = nemoshow_canvas_create();
			nemoshow_canvas_set_width(canvas, pixels);
			nemoshow_canvas_set_height(canvas, pixels);
			nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
			nemoshow_attach_one(show, canvas);

			if (os_has_file_extension(filepath, "svg", NULL) != 0) {
				srand(time_current_msecs());

				one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
				nemoshow_one_attach(canvas, one);
				nemoshow_item_set_x(one, 0.0f);
				nemoshow_item_set_y(one, 0.0f);
				nemoshow_item_set_width(one, pixels);
				nemoshow_item_set_height(one, pixels);
				nemoshow_item_set_fill_color(one,
						random_get_double(0.0f, 128.0f),
						random_get_double(0.0f, 128.0f),
						random_get_double(0.0f, 128.0f),
						255.0f);
				nemoshow_item_set_stroke_color(one,
						random_get_double(128.0f, 255.0f),
						random_get_double(128.0f, 255.0f),
						random_get_double(128.0f, 255.0f),
						255.0f);
				nemoshow_item_set_stroke_width(one, pixels / 128);
				nemoshow_item_path_load_svg(one, filepath, 0.0f, 0.0f, pixels, pixels);
			} else {
				one = nemoshow_item_create(NEMOSHOW_IMAGE_ITEM);
				nemoshow_one_attach(canvas, one);
				nemoshow_item_set_x(one, 0.0f);
				nemoshow_item_set_y(one, 0.0f);
				nemoshow_item_set_width(one, pixels);
				nemoshow_item_set_height(one, pixels);
				nemoshow_item_set_uri(one, filepath);
			}

			nemoshow_update_one(show);
			nemoshow_canvas_render(show, canvas);
		}

		free(entries);
	} else {
		pixs->sprites[0] = canvas = nemoshow_canvas_create();
		nemoshow_canvas_set_width(canvas, pixels);
		nemoshow_canvas_set_height(canvas, pixels);
		nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
		nemoshow_attach_one(show, canvas);

		pixs->nsprites = 1;
		pixs->isprites = 0;

		if (imagepath == NULL) {
			one = nemoshow_item_create(NEMOSHOW_CIRCLE_ITEM);
			nemoshow_one_attach(canvas, one);
			nemoshow_item_set_cx(one, pixels / 2.0f);
			nemoshow_item_set_cy(one, pixels / 2.0f);
			nemoshow_item_set_r(one, pixels / 3.0f);
			nemoshow_item_set_fill_color(one, 0.0f, 255.0f, 255.0f, 255.0f);
		} else if (os_has_file_extension(imagepath, "svg", NULL) != 0) {
			one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
			nemoshow_one_attach(canvas, one);
			nemoshow_item_set_x(one, 0.0f);
			nemoshow_item_set_y(one, 0.0f);
			nemoshow_item_set_width(one, pixels);
			nemoshow_item_set_height(one, pixels);
			nemoshow_item_set_fill_color(one, 0.0f, 255.0f, 255.0f, 255.0f);
			nemoshow_item_path_load_svg(one, imagepath, 0.0f, 0.0f, pixels, pixels);
		} else if (os_has_file_extension(imagepath, "png", "jpg", NULL) != 0) {
			one = nemoshow_item_create(NEMOSHOW_IMAGE_ITEM);
			nemoshow_one_attach(canvas, one);
			nemoshow_item_set_x(one, 0.0f);
			nemoshow_item_set_y(one, 0.0f);
			nemoshow_item_set_width(one, pixels);
			nemoshow_item_set_height(one, pixels);
			nemoshow_item_set_uri(one, imagepath);
		}

		nemoshow_update_one(show);
		nemoshow_canvas_render(show, canvas);
	}

	if (videopath != NULL) {
		pixs->video = canvas = nemoshow_canvas_create();
		nemoshow_canvas_set_width(canvas, pixels);
		nemoshow_canvas_set_height(canvas, pixels);
		nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
		nemoshow_attach_one(show, canvas);

		one = nemoshow_item_create(NEMOSHOW_IMAGE_ITEM);
		nemoshow_one_attach(canvas, one);
		nemoshow_item_set_x(one, 0.0f);
		nemoshow_item_set_y(one, 0.0f);
		nemoshow_item_set_width(one, pixels);
		nemoshow_item_set_height(one, pixels);
		nemoshow_item_set_uri(one, videopath);
	}

	nemopixs_prepare_opengl(pixs, width, height);

	pixs->one = nemopixs_one_create(width, height, pixels, pixels);
	if (pixs->video == NULL)
		nemopixs_one_set_diffuse(pixs->one, pixs->sprites[pixs->isprites], 0.05f);
	else
		nemopixs_one_set_texture(pixs->one, pixs->video);
	nemopixs_one_jitter(pixs->one, pixs->jitter);
	nemopixs_one_set_noise(pixs->one, 0.85f, 1.05f);
	nemopixs_one_set_pixel(pixs->one, pixs->pixsize);
	nemopixs_one_set_position(pixs->one, 4, 1);
	nemopixs_one_set_position_to(pixs->one, 0, 1);

	pixs->stimer = timer = nemotimer_create(tool);
	nemotimer_set_callback(timer, nemopixs_dispatch_scene_timer);
	nemotimer_set_userdata(timer, pixs);
	nemotimer_set_timeout(timer, pixs->timeout);

	pixs->ptimer = timer = nemotimer_create(tool);
	nemotimer_set_callback(timer, nemopixs_dispatch_pixel_timer);
	nemotimer_set_userdata(timer, pixs);
	nemotimer_set_timeout(timer, NEMOPIXS_PIXEL_TIMEOUT);

	if (fullscreen == NULL)
		nemoshow_dispatch_frame(show);

	nemotool_run(tool);

	nemotimer_destroy(pixs->stimer);
	nemotimer_destroy(pixs->ptimer);

	nemopixs_one_destroy(pixs->one);

	nemopixs_finish_opengl(pixs);

	nemoshow_destroy_view(show);

err3:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

err2:
	free(pixs);

err1:
	return 0;
}
