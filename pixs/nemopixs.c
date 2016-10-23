#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>
#include <ctype.h>
#include <math.h>

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

	for (i = 0; i < rows * columns; i++) {
		pixs->vertices[i * 3 + 0] = random_get_double(-1.0f, 1.0f);
		pixs->vertices[i * 3 + 1] = random_get_double(-1.0f, 1.0f);
		pixs->vertices[i * 3 + 2] = pixsize;

		pixs->velocities[i * 2 + 0] = 0.0f;
		pixs->velocities[i * 2 + 1] = 0.0f;

		pixs->diffuses[i * 4 + 0] = 0.0f;
		pixs->diffuses[i * 4 + 1] = 0.0f;
		pixs->diffuses[i * 4 + 2] = 0.0f;
		pixs->diffuses[i * 4 + 3] = 0.0f;
	}

	return 0;
}

static void nemopixs_finish_pixels(struct nemopixs *pixs)
{
	free(pixs->vertices);
	free(pixs->velocities);
	free(pixs->diffuses);
}

static int nemopixs_set_diffuse(struct nemopixs *pixs, struct showone *canvas)
{
	uint8_t *pixels;
	int x, y;

	pixels = (uint8_t *)nemoshow_canvas_map(canvas);

	for (y = 0; y < pixs->rows; y++) {
		for (x = 0; x < pixs->columns; x++) {
			pixs->diffuses[(y * pixs->columns) * 4 + x * 4 + 0] = (float)pixels[(y * pixs->columns) * 4 + x * 4 + 2] / 255.0f;
			pixs->diffuses[(y * pixs->columns) * 4 + x * 4 + 1] = (float)pixels[(y * pixs->columns) * 4 + x * 4 + 1] / 255.0f;
			pixs->diffuses[(y * pixs->columns) * 4 + x * 4 + 2] = (float)pixels[(y * pixs->columns) * 4 + x * 4 + 0] / 255.0f;
			pixs->diffuses[(y * pixs->columns) * 4 + x * 4 + 3] = (float)pixels[(y * pixs->columns) * 4 + x * 4 + 3] / 255.0f;
		}
	}

	nemoshow_canvas_unmap(canvas);

	return 0;
}

static void nemopixs_dispatch_canvas_update(void *userdata, uint32_t msecs, double t)
{
	struct nemopixs *pixs = (struct nemopixs *)userdata;
	float x0, y0;
	float dt;
	float dx, dy, dd, ds;
	float f;
	int i, n;

	nemopixs_set_diffuse(pixs, pixs->sprite);

	if (msecs == 0 || pixs->msecs == 0)
		pixs->msecs = msecs;

	dt = (float)(msecs - pixs->msecs) / 1000.0f;

	if (pixs->ntaps > 0) {
		for (i = 0; i < pixs->rows * pixs->columns; i++) {
			for (n = 0; n < pixs->ntaps; n++) {
				x0 = (pixs->taps[n * 2 + 0] / (float)pixs->width) * 2.0f - 1.0f;
				y0 = (pixs->taps[n * 2 + 1] / (float)pixs->height) * 2.0f - 1.0f;

				dx = x0 - pixs->vertices[i * 3 + 0];
				dy = y0 - pixs->vertices[i * 3 + 1];
				dd = dx * dx + dy * dy;
				ds = sqrtf(dd + 0.1f);

				f = 3.0f * dt / (ds * ds * ds);

				pixs->velocities[i * 2 + 0] += dx * f;
				pixs->velocities[i * 2 + 1] += dy * f;
			}

			pixs->vertices[i * 3 + 0] = pixs->vertices[i * 3 + 0] + pixs->velocities[i * 2 + 0] * dt;
			pixs->vertices[i * 3 + 1] = pixs->vertices[i * 3 + 1] + pixs->velocities[i * 2 + 1] * dt;
		}
	} else {
		for (i = 0; i < pixs->rows * pixs->columns; i++) {
			x0 = ((float)(i % pixs->columns) / (float)pixs->columns) * 2.0f - 1.0f;
			y0 = ((float)(i / pixs->columns) / (float)pixs->rows) * 2.0f - 1.0f;

			dx = x0 - pixs->vertices[i * 3 + 0];
			dy = y0 - pixs->vertices[i * 3 + 1];
			dd = dx * dx + dy * dy;
			ds = sqrtf(dd + 0.1f);

			f = 512.0f * dt / ds * MAX(MAX(pixs->diffuses[i * 4 + 0], pixs->diffuses[i * 4 + 1]), pixs->diffuses[i * 4 + 2]);

			pixs->velocities[i * 2 + 0] = dx * f;
			pixs->velocities[i * 2 + 1] = dy * f;

			pixs->vertices[i * 3 + 0] = pixs->vertices[i * 3 + 0] + pixs->velocities[i * 2 + 0] * dt;
			pixs->vertices[i * 3 + 1] = pixs->vertices[i * 3 + 1] + pixs->velocities[i * 2 + 1] * dt;
		}
	}

	pixs->msecs = msecs;
}

static void nemopixs_dispatch_canvas_redraw(struct nemoshow *show, struct showone *canvas)
{
	struct nemopixs *pixs = (struct nemopixs *)nemoshow_get_userdata(show);

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

	glDrawArrays(GL_POINTS, 0, pixs->rows * pixs->columns);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void nemopixs_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, struct showevent *event)
{
	struct nemopixs *pixs = (struct nemopixs *)nemoshow_get_userdata(show);
	int i;

	nemoshow_event_update_taps(show, canvas, event);

	pixs->ntaps = nemoshow_event_get_tapcount(event);

	for (i = 0; i < pixs->ntaps; i++) {
		nemoshow_transform_to_viewport(show,
				nemoshow_event_get_x_on(event, i),
				nemoshow_event_get_y_on(event, i),
				&pixs->taps[i * 2 + 0],
				&pixs->taps[i * 2 + 1]);
	}

	if (nemoshow_event_is_touch_down(show, event) || nemoshow_event_is_touch_up(show, event)) {
		if (nemoshow_event_is_more_taps(show, event, 3)) {
			nemoshow_view_pick_distant(show, event, NEMOSHOW_VIEW_PICK_ALL_TYPE);

			nemoshow_event_set_cancel(event);

			nemoshow_dispatch_grab_all(show, event);
		}
	}
}

static void nemopixs_dispatch_show_resize(struct nemoshow *show, int32_t width, int32_t height)
{
	struct nemopixs *pixs = (struct nemopixs *)nemoshow_get_userdata(show);

	pixs->width = width;
	pixs->height = height;

	nemoshow_view_resize(pixs->show, width, height);

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

	nemoshow_canvas_set_viewport(pixs->sprite, pixs->columns, pixs->rows);

	nemoshow_view_redraw(pixs->show);
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
		{ "fullscreen",			required_argument,			NULL,			'f' },
		{ 0 }
	};

	struct nemopixs *pixs;
	struct nemotool *tool;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct showone *one;
	struct showtransition *trans;
	char *imagepath = NULL;
	char *fullscreen = NULL;
	int width = 800;
	int height = 800;
	int pixels = 128;
	int fps = 60;
	int opt;

	opterr = 0;

	while (opt = getopt_long(argc, argv, "r:i:p:f:", options, NULL)) {
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

	pixs->sprite = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, pixels);
	nemoshow_canvas_set_height(canvas, pixels);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
	nemoshow_attach_one(show, canvas);

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
	} else if (os_has_file_extension(imagepath, "png", NULL) != 0) {
		one = nemoshow_item_create(NEMOSHOW_IMAGE_ITEM);
		nemoshow_one_attach(canvas, one);
		nemoshow_item_set_x(one, 0.0f);
		nemoshow_item_set_y(one, 0.0f);
		nemoshow_item_set_width(one, pixels);
		nemoshow_item_set_height(one, pixels);
		nemoshow_item_set_uri(one, imagepath);
	}

	trans = nemoshow_transition_create(NEMOSHOW_LINEAR_EASE, 18000, 0);
	nemoshow_transition_dirty_one(trans, pixs->canvas, NEMOSHOW_REDRAW_DIRTY);
	nemoshow_transition_set_repeat(trans, 0);
	nemoshow_transition_set_dispatch_frame(trans, nemopixs_dispatch_canvas_update);
	nemoshow_transition_set_userdata(trans, pixs);
	nemoshow_attach_transition(show, trans);

	nemopixs_prepare_opengl(pixs, width, height);
	nemopixs_prepare_pixels(pixs, width, height, pixels, pixels);

	nemoshow_dispatch_frame(show);

	nemotool_run(tool);

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
