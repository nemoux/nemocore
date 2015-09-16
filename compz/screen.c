#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <screen.h>
#include <compz.h>
#include <canvas.h>
#include <view.h>
#include <nemomatrix.h>
#include <nemoitem.h>
#include <nemolog.h>
#include <nemomisc.h>

static int nemoscreen_repaint_frame(struct nemoscreen *screen)
{
	struct nemocompz *compz = screen->compz;
	struct nemoframe_callback *cb, *cnext;
	struct nemocanvas *canvas;
	struct wl_list frame_callback_list;
	int r = 0;

	wl_list_init(&frame_callback_list);

	if (compz->dirty != 0) {
		wl_list_for_each(canvas, &compz->canvas_list, link) {
			wl_list_insert_list(&frame_callback_list, &canvas->frame_callback_list);
			wl_list_init(&canvas->frame_callback_list);
		}

		nemocompz_update_transform(compz);
		nemocompz_acculumate_damage(compz);
		nemocompz_flush_damage(compz);

		compz->dirty = 0;
	}

	if (pixman_region32_not_empty(&screen->damage)) {
		r = screen->repaint_frame(screen, &screen->damage);
	}

	wl_list_for_each_safe(cb, cnext, &frame_callback_list, link) {
		wl_callback_send_done(cb->resource, screen->frame_msecs);
		wl_resource_destroy(cb->resource);
	}

	screen->repaint_needed = 0;

	return r;
}

void nemoscreen_finish_frame(struct nemoscreen *screen, uint32_t secs, uint32_t usecs, uint32_t psf_flags)
{
	struct nemocompz *compz = screen->compz;

	if (!wl_list_empty(&compz->feedback_list)) {
		struct nemocanvas *canvas, *tmp;
		struct wl_list feedback_list;

		wl_list_init(&feedback_list);

		wl_list_for_each_safe(canvas, tmp, &compz->feedback_list, feedback_link) {
			if (canvas->base.screen_mask & (1 << screen->id)) {
				if (!wl_list_empty(&canvas->feedback_list)) {
					struct nemoview *view;
					struct nemofeedback *feedback;
					uint32_t flags = 0xffffffff;

					wl_list_for_each(view, &canvas->view_list, link) {
						flags &= view->psf_flags;
					}

					wl_list_for_each(feedback, &canvas->feedback_list, link) {
						feedback->psf_flags = flags;
					}

					wl_list_insert_list(&feedback_list, &canvas->feedback_list);
					wl_list_init(&canvas->feedback_list);
				}

				wl_list_remove(&canvas->feedback_link);
				wl_list_init(&canvas->feedback_link);
			}
		}

		nemopresentation_present_feedback_list(&feedback_list, screen,
				1000000000000LL / screen->current_mode->refresh,
				secs, usecs * 1000, screen->msc, psf_flags);
	}

	screen->frame_msecs = secs * 1000 + usecs / 1000;

	if (screen->repaint_needed != 0) {
		if (nemoscreen_repaint_frame(screen) < 0)
			return;
	}

	screen->repaint_scheduled = 0;
}

static void nemoscreen_dispatch_repaint(void *data)
{
	struct nemoscreen *screen = (struct nemoscreen *)data;

	screen->repaint(screen);
}

void nemoscreen_schedule_repaint(struct nemoscreen *screen)
{
	screen->compz->dirty = 1;

	screen->repaint_needed = 1;
	if (screen->repaint_scheduled != 0)
		return;

	wl_event_loop_add_idle(screen->compz->loop, nemoscreen_dispatch_repaint, screen);
	screen->repaint_scheduled = 1;
}

int nemoscreen_read_pixels(struct nemoscreen *screen, pixman_format_code_t format, void *pixels, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	return screen->read_pixels(screen, format, pixels, x, y, width, height);
}

static void nemoscreen_unbind_output(struct wl_resource *resource)
{
	wl_list_remove(wl_resource_get_link(resource));
}

static void nemoscreen_bind_output(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct nemoscreen *screen = (struct nemoscreen *)data;
	struct nemomode *mode;
	struct wl_resource *resource;

	resource = wl_resource_create(client, &wl_output_interface, MIN(version, 2), id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_list_insert(&screen->resource_list, wl_resource_get_link(resource));
	wl_resource_set_implementation(resource, NULL, screen, nemoscreen_unbind_output);

	wl_output_send_geometry(resource,
			screen->x,
			screen->y,
			screen->mmwidth,
			screen->mmheight,
			screen->subpixel,
			screen->make,
			screen->model,
			WL_OUTPUT_TRANSFORM_NORMAL);
	if (version >= 2)
		wl_output_send_scale(resource, 1);

	wl_list_for_each(mode, &screen->mode_list, link) {
		wl_output_send_mode(resource,
				mode->flags,
				mode->width,
				mode->height,
				mode->refresh);
	}

	if (version >= 2)
		wl_output_send_done(resource);
}

void nemoscreen_prepare(struct nemoscreen *screen, int32_t x, int32_t y, int32_t width, int32_t height, int32_t mmwidth, int32_t mmheight, int32_t r, int32_t pwidth, int32_t pheight, int32_t diagonal)
{
	struct nemocompz *compz = screen->compz;

	screen->x = x;
	screen->y = y;
	screen->width = width;
	screen->height = height;
	screen->mmwidth = mmwidth;
	screen->mmheight = mmheight;
	screen->r = r;
	screen->pwidth = pwidth;
	screen->pheight = pheight;
	screen->diagonal = diagonal;

	screen->snddev = NULL;

	wl_signal_init(&screen->destroy_signal);
	wl_list_init(&screen->resource_list);

	pixman_region32_init(&screen->region);
	pixman_region32_init(&screen->damage);

	screen->id = ffs(~compz->screen_idpool) - 1;
	compz->screen_idpool |= 1 << screen->id;

	screen->global = wl_global_create(compz->display, &wl_output_interface, 2, screen, nemoscreen_bind_output);
	if (screen->global == NULL)
		goto err1;

	return;

err1:
	nemoscreen_finish(screen);
}

void nemoscreen_finish(struct nemoscreen *screen)
{
	screen->compz->screen_idpool &= ~(1 << screen->id);

	wl_signal_emit(&screen->destroy_signal, screen);

	pixman_region32_fini(&screen->region);
	pixman_region32_fini(&screen->damage);

	if (screen->snddev != NULL)
		free(screen->snddev);
}

static void nemoscreen_update_transform_matrix(struct nemoscreen *screen)
{
	pixman_region32_fini(&screen->region);

	if (screen->r == 0) {
		pixman_region32_init_rect(&screen->region,
				screen->x, screen->y, screen->width, screen->height);
	} else {
		struct nemomatrix *matrix = &screen->transform.matrix;
		struct nemomatrix *inverse = &screen->transform.inverse;
		double radian = -screen->r / 180.0f * M_PI;
		float cx = screen->width / 2.0f;
		float cy = screen->height / 2.0f;
		float minx = HUGE_VALF, miny = HUGE_VALF;
		float maxx = -HUGE_VALF, maxy = -HUGE_VALF;
		int32_t s[4][2] = {
			{ 0, 0 },
			{ 0, screen->height },
			{ screen->width, 0 },
			{ screen->width, screen->height }
		};
		float tx, ty;
		float sx, sy, ex, ey;
		int i;

		nemomatrix_init_identity(matrix);

		nemomatrix_translate(matrix, -cx, -cy);
		nemomatrix_rotate(matrix, cos(radian), sin(radian));
		nemomatrix_translate(matrix, cx, cy);

		nemomatrix_translate(matrix, screen->x, screen->y);

		if (nemomatrix_invert(inverse, matrix) < 0) {
			pixman_region32_init(&screen->region);
			nemolog_warning("SCREEN", "failed to invert transform matrix\n");
			return;
		}

		for (i = 0; i < 4; i++) {
			struct nemovector v = { s[i][0], s[i][1], 0.0f, 1.0f };

			nemomatrix_transform(matrix, &v);

			if (fabsf(v.f[3]) < 1e-6) {
				tx = 0.0f;
				ty = 0.0f;
			} else {
				tx = v.f[0] / v.f[3];
				ty = v.f[1] / v.f[3];
			}

			if (tx < minx)
				minx = tx;
			if (tx > maxx)
				maxx = tx;
			if (ty < miny)
				miny = ty;
			if (ty > maxy)
				maxy = ty;
		}

		sx = floorf(minx);
		sy = floorf(miny);
		ex = ceilf(maxx);
		ey = ceilf(maxy);

		pixman_region32_init_rect(&screen->region, sx, sy, ex - sx, ey - sy);
	}
}

static void nemoscreen_update_render_matrix(struct nemoscreen *screen)
{
	nemomatrix_init_identity(&screen->render.matrix);

	nemomatrix_translate(&screen->render.matrix,
			-screen->x,
			-screen->y);

	if (screen->width != screen->pwidth || screen->height != screen->pheight) {
		nemomatrix_scale(&screen->render.matrix,
				(double)screen->pwidth / (double)screen->width,
				(double)screen->pheight / (double)screen->height);
	}

	nemomatrix_translate(&screen->render.matrix,
			-screen->pwidth / 2.0f,
			-screen->pheight / 2.0f);

	if (screen->r != 0) {
		nemomatrix_rotate(&screen->render.matrix,
				cos(screen->r / 180.0f * M_PI),
				sin(screen->r / 180.0f * M_PI));
	}

	nemomatrix_scale(&screen->render.matrix,
			2.0f / screen->pwidth,
			-2.0f / screen->pheight);
}

void nemoscreen_update_geometry(struct nemoscreen *screen)
{
	nemoscreen_update_render_matrix(screen);
	nemoscreen_update_transform_matrix(screen);

	nemocompz_update_scene(screen->compz);
}

void nemoscreen_transform_to_global(struct nemoscreen *screen, float dx, float dy, float *x, float *y)
{
	if (screen->r != 0) {
		struct nemovector v = { dx, dy, 0.0f, 1.0f };

		nemomatrix_transform(&screen->transform.matrix, &v);

		if (fabsf(v.f[3]) < 1e-6) {
			*x = 0.0f;
			*y = 0.0f;
			return;
		}

		*x = v.f[0] / v.f[3];
		*y = v.f[1] / v.f[3];
	} else {
		if (screen->width != screen->pwidth || screen->height != screen->pheight) {
			*x = dx * (double)screen->width / (double)screen->pwidth + screen->x;
			*y = dy * (double)screen->height / (double)screen->pheight + screen->y;
		} else {
			*x = dx + screen->x;
			*y = dy + screen->y;
		}
	}
}

void nemoscreen_transform_from_global(struct nemoscreen *screen, float x, float y, float *dx, float *dy)
{
	if (screen->r != 0) {
		struct nemovector v = { x, y, 0.0f, 1.0f };

		nemomatrix_transform(&screen->transform.inverse, &v);

		if (fabsf(v.f[3]) < 1e-6) {
			*dx = 0.0f;
			*dy = 0.0f;
			return;
		}

		*dx = v.f[0] / v.f[3];
		*dy = v.f[1] / v.f[3];
	} else {
		if (screen->width != screen->pwidth || screen->height != screen->pheight) {
			*dx = x * (double)screen->width / (double)screen->pwidth - screen->x;
			*dy = y * (double)screen->height / (double)screen->pheight - screen->y;
		} else {
			*dx = x - screen->x;
			*dy = y - screen->y;
		}
	}
}

int nemoscreen_get_config_mode(struct nemocompz *compz, uint32_t nodeid, uint32_t screenid, struct nemomode *mode)
{
	int32_t width, height;
	uint32_t refresh;
	char *attr0, *attr1;
	const char *value;
	int index;

	asprintf(&attr0, "%d", nodeid);
	asprintf(&attr1, "%d", screenid);

	index = nemoitem_get_iftwo(compz->configs, "//nemoshell/screen", 0, "nodeid", attr0, "screenid", attr1);
	if (index < 0) {
		free(attr0);
		free(attr1);

		return 0;
	}

	if ((value = nemoitem_get_attr(compz->configs, index, "pwidth")) != NULL) {
		width = strtoul(value, 0, 10);
	} else if ((value = nemoitem_get_attr(compz->configs, index, "width")) != NULL) {
		width = strtoul(value, 0, 10);
	} else {
		width = 0;
	}

	if ((value = nemoitem_get_attr(compz->configs, index, "pheight")) != NULL) {
		height = strtoul(value, 0, 10);
	} else if ((value = nemoitem_get_attr(compz->configs, index, "height")) != NULL) {
		height = strtoul(value, 0, 10);
	} else {
		height = 0;
	}

	if ((value = nemoitem_get_attr(compz->configs, index, "refresh")) != NULL) {
		refresh = strtoul(value, 0, 10);
	} else {
		refresh = 0;
	}

	mode->width = width;
	mode->height = height;
	mode->refresh = refresh;

	free(attr0);
	free(attr1);

	return 1;
}

int nemoscreen_get_config_geometry(struct nemocompz *compz, uint32_t nodeid, uint32_t screenid, int32_t *x, int32_t *y, int32_t *width, int32_t *height, int32_t *r, int32_t *diagonal)
{
	char *attr0, *attr1;
	const char *value;
	int index;

	asprintf(&attr0, "%d", nodeid);
	asprintf(&attr1, "%d", screenid);

	index = nemoitem_get_iftwo(compz->configs, "//nemoshell/screen", 0, "nodeid", attr0, "screenid", attr1);
	if (index < 0) {
		free(attr0);
		free(attr1);

		return 0;
	}

	value = nemoitem_get_attr(compz->configs, index, "x");
	if (value != NULL && x != NULL) {
		*x = strtoul(value, 0, 10);
	}

	value = nemoitem_get_attr(compz->configs, index, "y");
	if (value != NULL && y != NULL) {
		*y = strtoul(value, 0, 10);
	}

	value = nemoitem_get_attr(compz->configs, index, "width");
	if (value != NULL && width != NULL) {
		*width = strtoul(value, 0, 10);
	}

	value = nemoitem_get_attr(compz->configs, index, "height");
	if (value != NULL && height != NULL) {
		*height = strtoul(value, 0, 10);
	}

	value = nemoitem_get_attr(compz->configs, index, "rotate");
	if (value != NULL && r != NULL) {
		*r = strtoul(value, 0, 10);
	}

	value = nemoitem_get_attr(compz->configs, index, "diagonal");
	if (value != NULL && diagonal != NULL) {
		*diagonal = strtoul(value, 0, 10);
	}

	free(attr0);
	free(attr1);

	return 1;
}

const char *nemoscreen_get_config(struct nemocompz *compz, uint32_t nodeid, uint32_t screenid, const char *attr)
{
	char *attr0, *attr1;
	int index;

	asprintf(&attr0, "%d", nodeid);
	asprintf(&attr1, "%d", screenid);

	index = nemoitem_get_iftwo(compz->configs, "//nemoshell/screen", 0, "nodeid", attr0, "screenid", attr1);
	if (index < 0) {
		free(attr0);
		free(attr1);

		return NULL;
	}

	free(attr0);
	free(attr1);

	return nemoitem_get_attr(compz->configs, index, attr);
}
