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

static int nemopixs_prepare_pixels(struct nemopixs *pixs, int32_t width, int32_t height, int32_t columns, int32_t rows)
{
	float pixsize = MIN((float)width / (float)columns, (float)height / (float)rows);
	int i;

	pixs->rows = rows;
	pixs->columns = columns;

	pixs->vertices = (float *)malloc(sizeof(float[3]) * rows * columns);
	pixs->velocities = (float *)malloc(sizeof(float[2]) * rows * columns);
	pixs->diffuses = (float *)malloc(sizeof(float[4]) * rows * columns);
	pixs->noises = (float *)malloc(sizeof(float) * rows * columns);
	pixs->vertices0 = (float *)malloc(sizeof(float[3]) * rows * columns);
	pixs->diffuses0 = (float *)malloc(sizeof(float[4]) * rows * columns);
	pixs->positions0 = (float *)malloc(sizeof(float[2]) * rows * columns);

	for (i = 0; i < rows * columns; i++) {
		pixs->vertices[i * 3 + 0] = 0.0f;
		pixs->vertices[i * 3 + 1] = 0.0f;
		pixs->vertices[i * 3 + 2] = pixsize;

		pixs->velocities[i * 2 + 0] = 0.0f;
		pixs->velocities[i * 2 + 1] = 0.0f;

		pixs->diffuses[i * 4 + 0] = 0.0f;
		pixs->diffuses[i * 4 + 1] = 0.0f;
		pixs->diffuses[i * 4 + 2] = 0.0f;
		pixs->diffuses[i * 4 + 3] = 0.0f;

		pixs->noises[i] = 1.0f;
	}

	return 0;
}

static void nemopixs_finish_pixels(struct nemopixs *pixs)
{
	free(pixs->vertices);
	free(pixs->velocities);
	free(pixs->diffuses);
	free(pixs->noises);
	free(pixs->vertices0);
	free(pixs->diffuses0);
	free(pixs->positions0);
}

static int nemopixs_set_position(struct nemopixs *pixs, int action)
{
	float seed;
	int i;

	if (action == 0) {
		for (i = 0; i < pixs->pixscount; i++) {
			pixs->vertices[i * 3 + 0] = pixs->positions0[i * 2 + 0];
			pixs->vertices[i * 3 + 1] = pixs->positions0[i * 2 + 1];

			pixs->vertices0[i * 3 + 0] = pixs->vertices[i * 3 + 0];
			pixs->vertices0[i * 3 + 1] = pixs->vertices[i * 3 + 1];
		}
	} else if (action == 1) {
		for (i = 0; i < pixs->pixscount; i++) {
			pixs->vertices[i * 3 + 0] = 0.0f;
			pixs->vertices[i * 3 + 1] = 0.0f;

			pixs->vertices0[i * 3 + 0] = pixs->vertices[i * 3 + 0];
			pixs->vertices0[i * 3 + 1] = pixs->vertices[i * 3 + 1];
		}
	} else if (action == 2) {
		for (i = 0; i < pixs->pixscount; i++) {
			pixs->vertices[i * 3 + 1] = 1.0f;

			pixs->vertices0[i * 3 + 1] = pixs->vertices[i * 3 + 1];
		}
	} else if (action == 3) {
		for (i = 0; i < pixs->pixscount; i++) {
			seed = random_get_double(0.0f, 1.0f);

			pixs->vertices[i * 3 + 0] = cos(seed * M_PI * 2.0f) * 2.0f;
			pixs->vertices[i * 3 + 1] = sin(seed * M_PI * 2.0f) * 2.0f;

			pixs->vertices0[i * 3 + 0] = pixs->vertices[i * 3 + 0];
			pixs->vertices0[i * 3 + 1] = pixs->vertices[i * 3 + 1];
		}
	} else if (action == 4) {
		for (i = 0; i < pixs->pixscount; i++) {
			pixs->vertices[i * 3 + 0] = random_get_double(-1.0f, 1.0f);
			pixs->vertices[i * 3 + 1] = random_get_double(-1.0f, 1.0f);

			pixs->vertices0[i * 3 + 0] = pixs->vertices[i * 3 + 0];
			pixs->vertices0[i * 3 + 1] = pixs->vertices[i * 3 + 1];
		}
	}

	pixs->force_redraw = 1;

	return 0;
}

static int nemopixs_set_diffuse(struct nemopixs *pixs, struct showone *canvas, float minimum_alpha)
{
	uint8_t *pixels;
	int x, y;

	pixs->pixscount = 0;

	pixels = (uint8_t *)nemoshow_canvas_map(canvas);

	for (y = 0; y < pixs->rows; y++) {
		for (x = 0; x < pixs->columns; x++) {
			float a = (float)pixels[(y * pixs->columns) * 4 + x * 4 + 3] / 255.0f;

			if (a >= minimum_alpha) {
				float r = (float)pixels[(y * pixs->columns) * 4 + x * 4 + 2] / 255.0f;
				float g = (float)pixels[(y * pixs->columns) * 4 + x * 4 + 1] / 255.0f;
				float b = (float)pixels[(y * pixs->columns) * 4 + x * 4 + 0] / 255.0f;

				pixs->diffuses[(y * pixs->columns) * 4 + x * 4 + 0] = r;
				pixs->diffuses[(y * pixs->columns) * 4 + x * 4 + 1] = g;
				pixs->diffuses[(y * pixs->columns) * 4 + x * 4 + 2] = b;
				pixs->diffuses[(y * pixs->columns) * 4 + x * 4 + 3] = a;

				pixs->diffuses0[(y * pixs->columns) * 4 + x * 4 + 0] = r;
				pixs->diffuses0[(y * pixs->columns) * 4 + x * 4 + 1] = g;
				pixs->diffuses0[(y * pixs->columns) * 4 + x * 4 + 2] = b;
				pixs->diffuses0[(y * pixs->columns) * 4 + x * 4 + 3] = a;

				pixs->positions0[(y * pixs->columns) * 2 + x * 2 + 0] = ((float)x / (float)pixs->columns) * 2.0f - 1.0f;
				pixs->positions0[(y * pixs->columns) * 2 + x * 2 + 1] = ((float)y / (float)pixs->rows) * 2.0f - 1.0f;

				pixs->pixscount++;
			}
		}
	}

	nemoshow_canvas_unmap(canvas);

	pixs->force_redraw = 1;

	nemolog_message("PIXS", "[set_diffuse] columns(%d) rows(%d) pixels(%d)\n", pixs->columns, pixs->rows, pixs->pixscount);

	return 0;
}

static int nemopixs_set_noise(struct nemopixs *pixs, float min, float max)
{
	int i;

	for (i = 0; i < pixs->pixscount; i++) {
		pixs->noises[i] = random_get_double(min, max);
	}

	return 0;
}

static int nemopixs_set_position_to(struct nemopixs *pixs, int action)
{
	float seed;
	int i;

	if (action == 0) {
		for (i = 0; i < pixs->pixscount; i++) {
			pixs->vertices0[i * 3 + 0] = pixs->positions0[i * 2 + 0];
			pixs->vertices0[i * 3 + 1] = pixs->positions0[i * 2 + 1];
		}
	} else if (action == 1) {
		for (i = 0; i < pixs->pixscount; i++) {
			pixs->vertices0[i * 3 + 0] = 0.0f;
			pixs->vertices0[i * 3 + 1] = 0.0f;
		}
	} else if (action == 2) {
		for (i = 0; i < pixs->pixscount; i++) {
			pixs->vertices0[i * 3 + 1] = 1.0f;
		}
	} else if (action == 3) {
		for (i = 0; i < pixs->pixscount; i++) {
			seed = random_get_double(0.0f, 1.0f);

			pixs->vertices0[i * 3 + 0] = cos(seed * M_PI * 2.0f) * 2.0f;
			pixs->vertices0[i * 3 + 1] = sin(seed * M_PI * 2.0f) * 2.0f;
		}
	} else if (action == 4) {
		for (i = 0; i < pixs->pixscount; i++) {
			pixs->vertices0[i * 3 + 0] = random_get_double(-1.0f, 1.0f);
			pixs->vertices0[i * 3 + 1] = random_get_double(-1.0f, 1.0f);
		}
	}

	pixs->is_vertices_dirty = 1;

	return 0;
}

static int nemopixs_set_diffuse_to(struct nemopixs *pixs, struct showone *canvas, float minimum_alpha)
{
	uint8_t *pixels;
	int x, y;

	pixs->pixscount = 0;

	pixels = (uint8_t *)nemoshow_canvas_map(canvas);

	for (y = 0; y < pixs->rows; y++) {
		for (x = 0; x < pixs->columns; x++) {
			float a = (float)pixels[(y * pixs->columns) * 4 + x * 4 + 3] / 255.0f;

			if (a >= minimum_alpha) {
				float r = (float)pixels[(y * pixs->columns) * 4 + x * 4 + 2] / 255.0f;
				float g = (float)pixels[(y * pixs->columns) * 4 + x * 4 + 1] / 255.0f;
				float b = (float)pixels[(y * pixs->columns) * 4 + x * 4 + 0] / 255.0f;

				pixs->diffuses0[(y * pixs->columns) * 4 + x * 4 + 0] = r;
				pixs->diffuses0[(y * pixs->columns) * 4 + x * 4 + 1] = g;
				pixs->diffuses0[(y * pixs->columns) * 4 + x * 4 + 2] = b;
				pixs->diffuses0[(y * pixs->columns) * 4 + x * 4 + 3] = a;

				pixs->positions0[(y * pixs->columns) * 2 + x * 2 + 0] = ((float)x / (float)pixs->columns) * 2.0f - 1.0f;
				pixs->positions0[(y * pixs->columns) * 2 + x * 2 + 1] = ((float)y / (float)pixs->rows) * 2.0f - 1.0f;

				pixs->pixscount++;
			}
		}
	}

	nemoshow_canvas_unmap(canvas);

	pixs->is_diffuses_dirty = 1;

	nemolog_message("PIXS", "[set_diffuse_to] columns(%d) rows(%d) pixels(%d)\n", pixs->columns, pixs->rows, pixs->pixscount);

	return 0;
}

static int nemopixs_update_pixels(struct nemopixs *pixs, uint32_t msecs)
{
	int tapcount = nemoshow_event_get_tapcount(&pixs->events);
	int is_updated = 0;
	float x0, y0;
	float dt;
	float dx, dy, dd, ds;
	float dr, dg, db, da;
	float r0, g0, b0, a0;
	float f;
	float c;
	float mc;
	int i, n;

	if (pixs->msecs == 0)
		pixs->msecs = msecs;

	dt = (float)(msecs - pixs->msecs) / 1000.0f;

	if (tapcount > 0) {
		for (i = 0; i < pixs->pixscount; i++) {
			for (n = 0; n < tapcount; n++) {
				x0 = (nemoshow_event_get_x_on(&pixs->events, n) / (float)pixs->width) * 2.0f - 1.0f;
				y0 = (nemoshow_event_get_y_on(&pixs->events, n) / (float)pixs->height) * 2.0f - 1.0f;

				dx = x0 - pixs->vertices[i * 3 + 0];
				dy = y0 - pixs->vertices[i * 3 + 1];
				dd = dx * dx + dy * dy;
				ds = sqrtf(dd + pixs->gravitywell_minimum_distance);

				f = (3.0f / tapcount) * dt / (ds * ds * ds);

				pixs->velocities[i * 2 + 0] += dx * f;
				pixs->velocities[i * 2 + 1] += dy * f;
			}

			pixs->vertices[i * 3 + 0] = CLIP(pixs->vertices[i * 3 + 0] + pixs->velocities[i * 2 + 0] * dt, -1.0f, 1.0f);
			pixs->vertices[i * 3 + 1] = CLIP(pixs->vertices[i * 3 + 1] + pixs->velocities[i * 2 + 1] * dt, -1.0f, 1.0f);
		}

		is_updated = 1;

		pixs->is_vertices_dirty = 1;
	}

	if (pixs->is_vertices_dirty != 0 && tapcount == 0) {
		int needs_feedback = 0;

		mc = random_get_double(0.015f, 0.075f);

		for (i = 0; i < pixs->pixscount; i++) {
			c = MAX(MAX(pixs->diffuses[i * 4 + 0], pixs->diffuses[i * 4 + 1]), MAX(pixs->diffuses[i * 4 + 2], mc));

			x0 = pixs->vertices0[i * 3 + 0];
			y0 = pixs->vertices0[i * 3 + 1];

			dx = x0 - pixs->vertices[i * 3 + 0];
			dy = y0 - pixs->vertices[i * 3 + 1];

			if (dx != 0.0f || dy != 0.0f) {
				dd = dx * dx + dy * dy;

				if (dd > 0.0001f) {
					ds = sqrtf(dd + pixs->move_minimum_distance);

					f = 512.0f * dt / ds * c * pixs->noises[i];

					pixs->velocities[i * 2 + 0] = dx * f;
					pixs->velocities[i * 2 + 1] = dy * f;

					pixs->vertices[i * 3 + 0] = pixs->vertices[i * 3 + 0] + pixs->velocities[i * 2 + 0] * dt;
					pixs->vertices[i * 3 + 1] = pixs->vertices[i * 3 + 1] + pixs->velocities[i * 2 + 1] * dt;

					needs_feedback = 1;
				} else {
					pixs->vertices[i * 3 + 0] = x0;
					pixs->vertices[i * 3 + 1] = y0;
				}

				is_updated = 1;
			}
		}

		if (needs_feedback == 0)
			pixs->is_vertices_dirty = 0;
	}

	if (pixs->is_diffuses_dirty != 0) {
		int needs_feedback = 0;

		for (i = 0; i < pixs->pixscount; i++) {
			r0 = pixs->diffuses0[i * 4 + 0];
			g0 = pixs->diffuses0[i * 4 + 1];
			b0 = pixs->diffuses0[i * 4 + 2];
			a0 = pixs->diffuses0[i * 4 + 3];

			dr = r0 - pixs->diffuses[i * 4 + 0];
			dg = g0 - pixs->diffuses[i * 4 + 1];
			db = b0 - pixs->diffuses[i * 4 + 2];
			da = a0 - pixs->diffuses[i * 4 + 3];

			if (dr != 0.0f || dg != 0.0f || db != 0.0f || da != 0.0f) {
				dd = dr * dr + dg * dg + db * db + da * da;

				if (dd > 0.0001f) {
					ds = sqrtf(dd + pixs->move_minimum_distance);

					f = 64.0f * dt / ds * pixs->noises[i];

					pixs->diffuses[i * 4 + 0] = pixs->diffuses[i * 4 + 0] + dr * f * dt;
					pixs->diffuses[i * 4 + 1] = pixs->diffuses[i * 4 + 1] + dg * f * dt;
					pixs->diffuses[i * 4 + 2] = pixs->diffuses[i * 4 + 2] + db * f * dt;
					pixs->diffuses[i * 4 + 3] = pixs->diffuses[i * 4 + 3] + da * f * dt;

					needs_feedback = 1;
				} else {
					pixs->diffuses[i * 4 + 0] = r0;
					pixs->diffuses[i * 4 + 1] = g0;
					pixs->diffuses[i * 4 + 2] = b0;
					pixs->diffuses[i * 4 + 3] = a0;
				}

				is_updated = 1;
			}
		}

		if (needs_feedback == 0)
			pixs->is_diffuses_dirty = 0;
	}

	pixs->msecs = msecs;

	return is_updated;
}

static void nemopixs_redraw_pixels(struct nemopixs *pixs, struct showone *canvas)
{
	glBindFramebuffer(GL_FRAMEBUFFER, pixs->fbo);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_POINT_SMOOTH);
	glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

	glViewport(0, 0,
			nemoshow_canvas_get_viewport_width(canvas),
			nemoshow_canvas_get_viewport_height(canvas));

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(pixs->program);

	glBindAttribLocation(pixs->program, 0, "position");
	glBindAttribLocation(pixs->program, 1, "diffuse");

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), &pixs->vertices[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &pixs->diffuses[0]);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_POINTS, 0, pixs->pixscount);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void nemopixs_dispatch_canvas_redraw(struct nemoshow *show, struct showone *canvas)
{
	struct nemopixs *pixs = (struct nemopixs *)nemoshow_get_userdata(show);

	if (nemopixs_update_pixels(pixs, time_current_msecs()) != 0 || pixs->force_redraw != 0) {
		nemopixs_redraw_pixels(pixs, canvas);

		nemoshow_one_dirty(canvas, NEMOSHOW_REDRAW_DIRTY);

		pixs->force_redraw = 0;
	} else {
		pixs->msecs = 0;
	}
}

static void nemopixs_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, struct showevent *event)
{
	struct nemopixs *pixs = (struct nemopixs *)nemoshow_get_userdata(show);

	if (nemoshow_event_is_pointer_left_down(show, event)) {
		nemopixs_set_noise(pixs, 0.75f, 1.25f);
		nemopixs_set_position_to(pixs, 0);
	} else if (nemoshow_event_is_pointer_right_down(show, event)) {
		nemopixs_set_noise(pixs, 0.75f, 1.25f);
		nemopixs_set_position_to(pixs, 2);
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
}

static void nemopixs_dispatch_show_resize(struct nemoshow *show, int32_t width, int32_t height)
{
	struct nemopixs *pixs = (struct nemopixs *)nemoshow_get_userdata(show);
	int i;

	nemoshow_view_resize(show, width, height);

	glDeleteFramebuffers(1, &pixs->fbo);
	glDeleteRenderbuffers(1, &pixs->dbo);

	fbo_prepare_context(
			nemoshow_canvas_get_texture(pixs->canvas),
			width, height,
			&pixs->fbo, &pixs->dbo);

	nemopixs_finish_pixels(pixs);

	if (width == height) {
		nemopixs_prepare_pixels(pixs, width, height, pixs->pixels, pixs->pixels);
	} else if (width > height) {
		nemopixs_prepare_pixels(pixs, width, height, pixs->pixels, pixs->pixels * (float)height / (float)width);
	} else if (width < height) {
		nemopixs_prepare_pixels(pixs, width, height, pixs->pixels * (float)width / (float)height, pixs->pixels);
	}

	for (i = 0; i < pixs->nsprites; i++) {
		nemoshow_canvas_set_viewport(pixs->sprites[i], pixs->columns, pixs->rows);
		nemoshow_canvas_render(show, pixs->sprites[i]);
	}

	nemopixs_set_diffuse_to(pixs, pixs->sprites[pixs->isprites], 0.05f);
	nemopixs_set_position_to(pixs, 0);

	nemoshow_one_dirty(pixs->canvas, NEMOSHOW_REDRAW_DIRTY);

	nemoshow_view_redraw(show);
}

static void nemopixs_dispatch_timer(struct nemotimer *timer, void *data)
{
	struct nemopixs *pixs = (struct nemopixs *)data;

	pixs->isprites = (pixs->isprites + 1) % pixs->nsprites;

	nemopixs_set_noise(pixs, 0.85f, 1.15f);
	nemopixs_set_diffuse_to(pixs, pixs->sprites[pixs->isprites], 0.05f);

	nemoshow_one_dirty(pixs->canvas, NEMOSHOW_REDRAW_DIRTY);
	nemoshow_dispatch_frame(pixs->show);

	nemotimer_set_timeout(pixs->timer, pixs->timeout);
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
		{ "framerate",			required_argument,			NULL,			'r' },
		{ "image",					required_argument,			NULL,			'i' },
		{ "pixels",					required_argument,			NULL,			'p' },
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
	int timeout = 15000;
	int width = 800;
	int height = 800;
	int pixels = 128;
	int fps = 60;
	int opt;

	opterr = 0;

	while (opt = getopt_long(argc, argv, "r:i:p:t:f:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'r':
				fps = strtoul(optarg, NULL, 10);
				break;

			case 'i':
				imagepath = strdup(optarg);
				break;

			case 'p':
				pixels = strtoul(optarg, NULL, 10);
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
	pixs->timeout = timeout;

	pixs->gravitywell_minimum_distance = 0.18f;
	pixs->move_minimum_distance = 0.01f;

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
	nemoshow_canvas_set_fill_color(canvas, 1.0f, 1.0f, 1.0f, 1.0f);
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
	nemopixs_prepare_pixels(pixs, width, height, pixels, pixels);

	nemopixs_set_diffuse(pixs, pixs->sprites[pixs->isprites], 0.05f);
	nemopixs_set_position(pixs, 0);

	pixs->timer = timer = nemotimer_create(tool);
	nemotimer_set_callback(timer, nemopixs_dispatch_timer);
	nemotimer_set_userdata(timer, pixs);
	nemotimer_set_timeout(pixs->timer, pixs->timeout);

	nemoshow_dispatch_frame(show);

	nemotool_run(tool);

	nemotimer_destroy(timer);

	nemopixs_finish_pixels(pixs);
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
