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
#define NEMOPIXS_MOVE_MINIMUM_DISTANCE						(0.01f)
#define NEMOPIXS_PIXEL_MINIMUM_DISTANCE						(1.0f)
#define NEMOPIXS_COLOR_MINIMUM_DISTANCE						(0.01f)

#define NEMOPIXS_GRAVITYWELL_INTENSITY		(3.0f)
#define NEMOPIXS_MOVE_INTENSITY						(256.0f)
#define NEMOPIXS_COLOR_INTENSITY					(64.0f)
#define NEMOPIXS_PIXEL_INTENSITY					(256.0f)

#define NEMOPIXS_MOVE_EPSILON							(0.025f)
#define NEMOPIXS_COLOR_EPSILON						(0.025f)
#define NEMOPIXS_PIXEL_EPSILON						(1.0f)

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
	}

	return one;
}

static void nemopixs_one_destroy(struct pixsone *one)
{
	free(one->vertices);
	free(one->velocities);
	free(one->diffuses);
	free(one->noises);
	free(one->vertices0);
	free(one->diffuses0);
	free(one->positions0);
	free(one->pixels0);

	free(one);
}

static inline void nemopixs_one_move(struct pixsone *one, int s, int d)
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

static inline void nemopixs_one_jitter(struct pixsone *one, float size)
{
	int i;

	for (i = 0; i < one->pixscount; i++) {
		one->positions0[i * 2 + 0] += random_get_double(-size / 2.0f, size / 2.0f);
		one->positions0[i * 2 + 1] += random_get_double(-size / 2.0f, size / 2.0f);
	}
}

static int nemopixs_one_set_position(struct pixsone *one, int action)
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
			one->vertices[i * 3 + 1] = 1.0f;

			one->vertices0[i * 3 + 1] = one->vertices[i * 3 + 1];
		}
	} else if (action == 3) {
		for (i = 0; i < one->pixscount; i++) {
			seed = random_get_double(0.0f, 1.0f);

			one->vertices[i * 3 + 0] = cos(seed * M_PI * 2.0f) * 2.0f;
			one->vertices[i * 3 + 1] = sin(seed * M_PI * 2.0f) * 2.0f;

			one->vertices0[i * 3 + 0] = one->vertices[i * 3 + 0];
			one->vertices0[i * 3 + 1] = one->vertices[i * 3 + 1];
		}
	} else if (action == 4) {
		for (i = 0; i < one->pixscount; i++) {
			one->vertices[i * 3 + 0] = random_get_double(-1.0f, 1.0f);
			one->vertices[i * 3 + 1] = random_get_double(-1.0f, 1.0f);

			one->vertices0[i * 3 + 0] = one->vertices[i * 3 + 0];
			one->vertices0[i * 3 + 1] = one->vertices[i * 3 + 1];
		}
	} else if (action == 5) {
		for (i = 0; i < one->pixscount; i++) {
			one->vertices[i * 3 + 0] = one->positions0[i * 2 + 0];
			one->vertices[i * 3 + 1] = -1.2f;

			one->vertices0[i * 3 + 0] = one->vertices[i * 3 + 0];
			one->vertices0[i * 3 + 1] = one->vertices[i * 3 + 1];
		}
	} else if (action == 6) {
		for (i = 0; i < one->pixscount; i++) {
			one->vertices[i * 3 + 0] = one->positions0[i * 2 + 0];
			one->vertices[i * 3 + 1] = 1.2f;

			one->vertices0[i * 3 + 0] = one->vertices[i * 3 + 0];
			one->vertices0[i * 3 + 1] = one->vertices[i * 3 + 1];
		}
	}

	return 0;
}

static int nemopixs_one_set_transform(struct pixsone *one, float x, float y, float w, float h)
{
	int i;

	for (i = 0; i < one->pixscount; i++) {
		one->vertices[i * 3 + 0] = one->positions0[i * 2 + 0] * w + x;
		one->vertices[i * 3 + 1] = one->positions0[i * 2 + 1] * h + y;

		one->vertices0[i * 3 + 0] = one->vertices[i * 3 + 0];
		one->vertices0[i * 3 + 1] = one->vertices[i * 3 + 1];
	}

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

	nemolog_message("PIXS", "[set_diffuse] columns(%d) rows(%d) pixels(%d)\n", one->columns, one->rows, one->pixscount);

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

	return 0;
}

static int nemopixs_one_set_pixel(struct pixsone *one, float r)
{
	int i;

	for (i = 0; i < one->pixscount; i++) {
		one->vertices[i * 3 + 2] = one->pixsize * r;

		one->pixels0[i] = one->pixsize * r;
	}

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

static int nemopixs_one_set_position_to(struct pixsone *one, int action)
{
	float seed;
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
			one->vertices0[i * 3 + 1] = 1.0f;
		}
	} else if (action == 3) {
		for (i = 0; i < one->pixscount; i++) {
			seed = random_get_double(0.0f, 1.0f);

			one->vertices0[i * 3 + 0] = cos(seed * M_PI * 2.0f) * 2.0f;
			one->vertices0[i * 3 + 1] = sin(seed * M_PI * 2.0f) * 2.0f;
		}
	} else if (action == 4) {
		for (i = 0; i < one->pixscount; i++) {
			one->vertices0[i * 3 + 0] = random_get_double(-1.0f, 1.0f);
			one->vertices0[i * 3 + 1] = random_get_double(-1.0f, 1.0f);
		}
	} else if (action == 5) {
		for (i = 0; i < one->pixscount; i++) {
			one->vertices0[i * 3 + 0] = one->positions0[i * 2 + 0];
			one->vertices0[i * 3 + 1] = -1.2f;
		}
	} else if (action == 6) {
		for (i = 0; i < one->pixscount; i++) {
			one->vertices0[i * 3 + 0] = one->positions0[i * 2 + 0];
			one->vertices0[i * 3 + 1] = 1.2f;
		}
	}

	one->is_vertices_dirty = 1;

	return 0;
}

static int nemopixs_one_set_transform_to(struct pixsone *one, float x, float y, float w, float h)
{
	int i;

	for (i = 0; i < one->pixscount; i++) {
		one->vertices0[i * 3 + 0] = one->positions0[i * 2 + 0] * w + x;
		one->vertices0[i * 3 + 1] = one->positions0[i * 2 + 1] * h + y;
	}

	one->is_vertices_dirty = 1;

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
			s = random_get_int(0, idx - 1);

			one->positions0[i * 2 + 0] = one->positions0[s * 2 + 0];
			one->positions0[i * 2 + 1] = one->positions0[s * 2 + 1];
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
	float x0, y0;
	float s0;
	float dx, dy, dd, ds;
	float dr, dg, db, da;
	float r0, g0, b0, a0;
	float f;
	float c;
	float mc;
	int i, n;

	if (tapcount > 0) {
		for (i = 0; i < one->pixscount; i++) {
			for (n = 0; n < tapcount; n++) {
				x0 = (nemoshow_event_get_x_on(&pixs->events, n) / (float)pixs->width) * 2.0f - 1.0f;
				y0 = (nemoshow_event_get_y_on(&pixs->events, n) / (float)pixs->height) * 2.0f - 1.0f;

				dx = x0 - one->vertices[i * 3 + 0];
				dy = y0 - one->vertices[i * 3 + 1];
				dd = dx * dx + dy * dy;
				ds = sqrtf(dd + NEMOPIXS_GRAVITYWELL_MINIMUM_DISTANCE);

				f = (NEMOPIXS_GRAVITYWELL_INTENSITY / tapcount) * dt / (ds * ds * ds) * one->noises[i];

				one->velocities[i * 2 + 0] += dx * f;
				one->velocities[i * 2 + 1] += dy * f;
			}

			one->vertices[i * 3 + 0] = CLIP(one->vertices[i * 3 + 0] + one->velocities[i * 2 + 0] * dt, -1.0f, 1.0f);
			one->vertices[i * 3 + 1] = CLIP(one->vertices[i * 3 + 1] + one->velocities[i * 2 + 1] * dt, -1.0f, 1.0f);
		}

		is_updated = 1;

		one->is_vertices_dirty = 1;
	}

	if (one->is_vertices_dirty != 0 && tapcount == 0) {
		int needs_feedback = 0;

		mc = random_get_double(0.015f, 0.075f);

		for (i = 0; i < one->pixscount; i++) {
			c = MAX(MAX(one->diffuses[i * 4 + 0], one->diffuses[i * 4 + 1]), MAX(one->diffuses[i * 4 + 2], mc));

			x0 = one->vertices0[i * 3 + 0];
			y0 = one->vertices0[i * 3 + 1];

			dx = x0 - one->vertices[i * 3 + 0];
			dy = y0 - one->vertices[i * 3 + 1];

			if (dx != 0.0f || dy != 0.0f) {
				dd = dx * dx + dy * dy;

				if (dd > NEMOPIXS_MOVE_EPSILON) {
					ds = sqrtf(dd + NEMOPIXS_MOVE_MINIMUM_DISTANCE);

					f = NEMOPIXS_MOVE_INTENSITY * dt / ds * c * one->noises[i];

					one->velocities[i * 2 + 0] = dx * f;
					one->velocities[i * 2 + 1] = dy * f;

					one->vertices[i * 3 + 0] = one->vertices[i * 3 + 0] + one->velocities[i * 2 + 0] * dt;
					one->vertices[i * 3 + 1] = one->vertices[i * 3 + 1] + one->velocities[i * 2 + 1] * dt;

					needs_feedback = 1;
				} else if (i >= one->pixscount0) {
					if (i < one->pixscount - 1)
						nemopixs_one_move(one, one->pixscount - 1, i);

					one->pixscount--;
				} else {
					one->vertices[i * 3 + 0] = x0;
					one->vertices[i * 3 + 1] = y0;

					one->velocities[i * 2 + 0] = 0.0f;
					one->velocities[i * 2 + 1] = 0.0f;
				}

				is_updated = 1;
			}
		}

		if (needs_feedback == 0)
			one->is_vertices_dirty = 0;
	}

	if (one->is_pixels_dirty != 0) {
		int needs_feedback = 0;

		for (i = 0; i < one->pixscount; i++) {
			s0 = one->pixels0[i];

			dd = s0 - one->vertices[i * 3 + 2];

			if (dd != 0.0f) {
				if (dd > NEMOPIXS_PIXEL_EPSILON) {
					ds = sqrtf(dd + NEMOPIXS_PIXEL_MINIMUM_DISTANCE);

					f = NEMOPIXS_PIXEL_INTENSITY * dt / ds * one->noises[i];

					one->vertices[i * 3 + 2] = one->vertices[i * 3 + 2] + dd * f * dt;

					needs_feedback = 1;
				} else {
					one->vertices[i * 3 + 2] = s0;
				}

				is_updated = 1;
			}
		}

		if (needs_feedback == 0)
			one->is_pixels_dirty = 0;
	}

	if (one->is_diffuses_dirty != 0) {
		int needs_feedback = 0;

		for (i = 0; i < one->pixscount; i++) {
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

					f = NEMOPIXS_COLOR_INTENSITY * dt / ds * one->noises[i];

					one->diffuses[i * 4 + 0] = one->diffuses[i * 4 + 0] + dr * f * dt;
					one->diffuses[i * 4 + 1] = one->diffuses[i * 4 + 1] + dg * f * dt;
					one->diffuses[i * 4 + 2] = one->diffuses[i * 4 + 2] + db * f * dt;
					one->diffuses[i * 4 + 3] = one->diffuses[i * 4 + 3] + da * f * dt;

					needs_feedback = 1;
				} else {
					one->diffuses[i * 4 + 0] = r0;
					one->diffuses[i * 4 + 1] = g0;
					one->diffuses[i * 4 + 2] = b0;
					one->diffuses[i * 4 + 3] = a0;
				}

				is_updated = 1;
			}
		}

		if (needs_feedback == 0)
			one->is_diffuses_dirty = 0;
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

	glUseProgram(pixs->program);

	glBindAttribLocation(pixs->program, 0, "position");
	glBindAttribLocation(pixs->program, 1, "diffuse");

	one = pixs->one;
	if (one != NULL) {
		if (nemopixs_update_one(pixs, one, dt) != 0)
			is_updated = 1;

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), &one->vertices[0]);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &one->diffuses[0]);
		glEnableVertexAttribArray(1);

		glDrawArrays(GL_POINTS, 0, one->pixscount);
	}

	one = pixs->one0;
	if (one != NULL) {
		int is_done = 0;

		if (nemopixs_update_one(pixs, one, dt) != 0)
			is_updated = 1;
		else
			is_done = 1;

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), &one->vertices[0]);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &one->diffuses[0]);
		glEnableVertexAttribArray(1);

		glDrawArrays(GL_POINTS, 0, one->pixscount);

		if (is_done != 0) {
			nemopixs_one_destroy(one);

			pixs->one0 = NULL;
		}
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (is_updated != 0) {
		nemoshow_one_dirty(canvas, NEMOSHOW_REDRAW_DIRTY);

		pixs->msecs = msecs;
	} else {
		pixs->msecs = 0;
	}
}

static void nemopixs_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, struct showevent *event)
{
	struct nemopixs *pixs = (struct nemopixs *)nemoshow_get_userdata(show);

	if (nemoshow_event_is_pointer_left_down(show, event)) {
		pixs->isprites = (pixs->isprites + 1) % pixs->nsprites;

		nemopixs_one_set_diffuse_to(pixs->one, pixs->sprites[pixs->isprites], 0.05f);
		nemopixs_one_shuffle(pixs->one);
		nemopixs_one_jitter(pixs->one, pixs->jitter);
		nemopixs_one_set_noise(pixs->one, 0.85f, 1.05f);
		nemopixs_one_set_position_to(pixs->one, 0);
	} else if (nemoshow_event_is_pointer_right_down(show, event)) {
		pixs->isprites = (pixs->isprites + pixs->nsprites - 1) % pixs->nsprites;

		nemopixs_one_set_diffuse_to(pixs->one, pixs->sprites[pixs->isprites], 0.05f);
		nemopixs_one_shuffle(pixs->one);
		nemopixs_one_jitter(pixs->one, pixs->jitter);
		nemopixs_one_set_noise(pixs->one, 0.85f, 1.05f);
		nemopixs_one_set_position_to(pixs->one, 0);
	}

	if (nemoshow_event_is_touch_down(show, event) || nemoshow_event_is_touch_up(show, event) || nemoshow_event_is_touch_motion(show, event)) {
		nemoshow_event_set_type(&pixs->events, NEMOSHOW_TOUCH_EVENT);

		nemoshow_event_update_taps(show, canvas, &pixs->events);
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

	nemotimer_set_timeout(pixs->timer, pixs->timeout);
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
	nemopixs_one_set_diffuse_to(pixs->one, pixs->sprites[pixs->isprites], 0.05f);
	nemopixs_one_jitter(pixs->one, pixs->jitter);
	nemopixs_one_set_noise(pixs->one, 0.85f, 1.05f);
	nemopixs_one_set_position_to(pixs->one, 0);

	nemoshow_one_dirty(pixs->canvas, NEMOSHOW_REDRAW_DIRTY);

	nemoshow_view_redraw(show);
}

static void nemopixs_dispatch_timer(struct nemotimer *timer, void *data)
{
	struct nemopixs *pixs = (struct nemopixs *)data;
	struct pixsone *one;
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

	one = nemopixs_one_create(width, height, columns, rows);
	nemopixs_one_set_diffuse(one, pixs->sprites[pixs->isprites], 0.05f);
	nemopixs_one_jitter(one, pixs->jitter);
	nemopixs_one_set_noise(one, 0.85f, 1.05f);
	nemopixs_one_set_position(one, 5);
	nemopixs_one_set_position_to(one, 0);

	nemopixs_one_set_noise(pixs->one, 0.95f, 1.25f);
	nemopixs_one_set_position_to(pixs->one, 6);

	pixs->one0 = pixs->one;
	pixs->one = one;

	nemoshow_one_dirty(pixs->canvas, NEMOSHOW_REDRAW_DIRTY);
	nemoshow_dispatch_frame(pixs->show);

	nemotimer_set_timeout(timer, pixs->timeout);
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
	static const char *fragmentshader =
		"precision mediump float;\n"
		"varying vec4 vdiffuse;\n"
		"void main()\n"
		"{\n"
		"  gl_FragColor = vdiffuse;\n"
		"}\n";

	fbo_prepare_context(
			nemoshow_canvas_get_texture(pixs->canvas),
			width, height,
			&pixs->fbo, &pixs->dbo);

	pixs->program = glshader_compile_program(vertexshader, fragmentshader, NULL, NULL);

	return 0;
}

static void nemopixs_finish_opengl(struct nemopixs *pixs)
{
	glDeleteFramebuffers(1, &pixs->fbo);
	glDeleteRenderbuffers(1, &pixs->dbo);

	glDeleteProgram(pixs->program);
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "width",					required_argument,			NULL,			'w' },
		{ "height",					required_argument,			NULL,			'h' },
		{ "framerate",			required_argument,			NULL,			'r' },
		{ "image",					required_argument,			NULL,			'i' },
		{ "pixels",					required_argument,			NULL,			'p' },
		{ "jitter",					required_argument,			NULL,			'j' },
		{ "timeout",				required_argument,			NULL,			't' },
		{ "fullscreen",			required_argument,			NULL,			'f' },
		{ 0 }
	};

	struct nemopixs *pixs;
	struct nemotool *tool;
	struct nemotimer *timer;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct showone *one;
	char *imagepath = NULL;
	char *fullscreen = NULL;
	float jitter = 0.0f;
	int timeout = 10000;
	int width = 800;
	int height = 800;
	int pixels = 128;
	int fps = 60;
	int opt;

	opterr = 0;

	while (opt = getopt_long(argc, argv, "w:h:r:i:p:j:t:f:", options, NULL)) {
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

			case 'p':
				pixels = strtoul(optarg, NULL, 10);
				break;

			case 'j':
				jitter = strtod(optarg, NULL);
				break;

			case 't':
				timeout = strtoul(optarg, NULL, 10);
				break;

			case 'f':
				fullscreen = strdup(optarg);
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
	pixs->timeout = timeout;

	pixs->tool = tool = nemotool_create();
	if (tool == NULL)
		goto err2;
	nemotool_connect_wayland(tool, NULL);

	pixs->show = show = nemoshow_create_view(tool, width, height);
	if (show == NULL)
		goto err3;
	nemoshow_set_dispatch_resize(show, nemopixs_dispatch_show_resize);
	nemoshow_set_userdata(show, pixs);

	nemoshow_view_set_framerate(show, fps);

	if (fullscreen != NULL)
		nemoshow_view_set_fullscreen(show, fullscreen);

	pixs->scene = scene = nemoshow_scene_create();
	nemoshow_scene_set_width(scene, width);
	nemoshow_scene_set_height(scene, height);
	nemoshow_set_scene(show, scene);

	pixs->back = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_BACK_TYPE);
	nemoshow_canvas_set_fill_color(canvas, 0.0f, 0.0f, 0.0f, 1.0f);
	nemoshow_one_attach(scene, canvas);

	pixs->canvas = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_OPENGL_TYPE);
	nemoshow_canvas_set_dispatch_redraw(canvas, nemopixs_dispatch_canvas_redraw);
	nemoshow_canvas_set_dispatch_event(canvas, nemopixs_dispatch_canvas_event);
	nemoshow_one_attach(scene, canvas);

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

			strcpy(filepath, imagepath);
			strcat(filepath, "/");
			strcat(filepath, filename);

			pixs->sprites[pixs->nsprites++] = canvas = nemoshow_canvas_create();
			nemoshow_canvas_set_width(canvas, pixels);
			nemoshow_canvas_set_height(canvas, pixels);
			nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
			nemoshow_attach_one(show, canvas);

			if (os_has_file_extension(filepath, "svg", NULL) != 0) {
				one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
				nemoshow_one_attach(canvas, one);
				nemoshow_item_set_x(one, 0.0f);
				nemoshow_item_set_y(one, 0.0f);
				nemoshow_item_set_width(one, pixels);
				nemoshow_item_set_height(one, pixels);
				nemoshow_item_set_fill_color(one, 0.0f, 255.0f, 255.0f, 255.0f);
				nemoshow_item_path_load_svg(one, filepath, 0.0f, 0.0f, pixels, pixels);
			} else if (os_has_file_extension(filepath, "png", "jpg", NULL) != 0) {
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

	nemopixs_prepare_opengl(pixs, width, height);

	pixs->one = nemopixs_one_create(width, height, pixels, pixels);
	nemopixs_one_set_diffuse(pixs->one, pixs->sprites[pixs->isprites], 0.05f);
	nemopixs_one_jitter(pixs->one, pixs->jitter);
	nemopixs_one_set_noise(pixs->one, 0.85f, 1.05f);
	nemopixs_one_set_position(pixs->one, 4);
	nemopixs_one_set_position_to(pixs->one, 0);

	pixs->timer = timer = nemotimer_create(tool);
	nemotimer_set_callback(timer, nemopixs_dispatch_timer);
	nemotimer_set_userdata(timer, pixs);
	nemotimer_set_timeout(timer, pixs->timeout);

	if (fullscreen == NULL)
		nemoshow_dispatch_frame(show);

	nemotool_run(tool);

	nemotimer_destroy(timer);

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
