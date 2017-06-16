#define _GNU_SOURCE
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
#include <presentation.h>
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

	if (screen->transform.dirty != 0) {
		nemoscreen_update_transform(screen);
	}

	if (compz->dirty != 0) {
		wl_list_for_each(canvas, &compz->canvas_list, link) {
			wl_list_insert_list(&frame_callback_list, &canvas->frame_callback_list);
			wl_list_init(&canvas->frame_callback_list);
		}

		nemocompz_update_transform(compz);
		nemocompz_accumulate_damage(compz);
		nemocompz_flush_damage(compz);

		compz->dirty = 0;
	}

	if (compz->layer_notify != 0) {
		nemocompz_update_layer(compz);

		compz->layer_notify = 0;
	}

	if (pixman_region32_not_empty(&screen->damage)) {
		r = screen->repaint_frame(screen, &screen->damage);
	}

	wl_list_for_each_safe(cb, cnext, &frame_callback_list, link) {
		wl_callback_send_done(cb->resource, screen->frame_msecs);
		wl_resource_destroy(cb->resource);
	}

	return r;
}

void nemoscreen_finish_frame(struct nemoscreen *screen, uint32_t secs, uint32_t usecs, uint32_t psf_flags)
{
	struct nemocompz *compz = screen->compz;

	screen->repaint_scheduled = 0;

	if (screen->repaint_needed == 0)
		return;
	screen->repaint_needed = 0;

	screen->frame_msecs = secs * 1000 + usecs / 1000;
	screen->frame_count++;

	nemoscreen_repaint_frame(screen);

	if (!wl_list_empty(&compz->feedback_list)) {
		struct nemocanvas *canvas, *tmp;
		struct wl_list feedback_list;

		wl_list_init(&feedback_list);

		wl_list_for_each_safe(canvas, tmp, &compz->feedback_list, feedback_link) {
			if (canvas->base.screen_mask & (1 << screen->id)) {
				canvas->base.screen_dirty &= ~(1 << screen->id);

				if (canvas->base.screen_dirty == 0x0) {
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
		}

		nemopresentation_present_feedback_list(&feedback_list, screen,
				1000000000000LL / screen->current_mode->refresh,
				secs, usecs * 1000, screen->msc, psf_flags);
	}
}

static void nemoscreen_dispatch_repaint(void *data)
{
	struct nemoscreen *screen = (struct nemoscreen *)data;

	if (nemoscreen_has_state(screen, NEMOSCREEN_DISPLAY_STATE) != 0)
		screen->repaint(screen);
}

void nemoscreen_schedule_repaint(struct nemoscreen *screen)
{
	if (screen->frameout_timeout == 0) {
		screen->compz->dirty = 1;

		screen->repaint_needed = 1;
		if (screen->repaint_scheduled != 0)
			return;

		wl_event_loop_add_idle(screen->compz->loop, nemoscreen_dispatch_repaint, screen);
		screen->repaint_scheduled = 1;
	} else if (screen->frameout_scheduled == 0) {
		screen->compz->dirty = 1;

		screen->frameout_scheduled = 1;

		if (nemoscreen_has_state(screen, NEMOSCREEN_DISPLAY_STATE) != 0) {
			screen->repaint_needed = 1;

			screen->repaint(screen);
		}

		wl_event_source_timer_update(screen->frameout_timer, screen->frameout_timeout);
	} else {
		screen->frameout_needed = 1;
	}
}

static int nemoscreen_dispatch_frameout(void *data)
{
	struct nemoscreen *screen = (struct nemoscreen *)data;

	screen->frameout_scheduled = 0;

	if (screen->frameout_needed != 0) {
		screen->compz->dirty = 1;

		screen->frameout_needed = 0;
		screen->frameout_scheduled = 1;

		if (nemoscreen_has_state(screen, NEMOSCREEN_DISPLAY_STATE) != 0) {
			screen->repaint_needed = 1;

			screen->repaint(screen);
		}

		wl_event_source_timer_update(screen->frameout_timer, screen->frameout_timeout);
	}

	return 1;
}

int nemoscreen_read_pixels(struct nemoscreen *screen, pixman_format_code_t format, void *pixels, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	return screen->read_pixels(screen, format, pixels, x, y, width, height);
}

int nemoscreen_switch_mode(struct nemoscreen *screen, int32_t width, int32_t height, uint32_t refresh)
{
	struct nemomode mode = {
		.width = width,
		.height = height,
		.refresh = refresh
	};

	if (screen->switch_mode(screen, &mode) < 0)
		return -1;

	screen->width = width;
	screen->height = height;

	screen->transform.dirty = 1;

	return 0;
}

static void nemoscreen_unbind_output(struct wl_resource *resource)
{
	wl_list_remove(wl_resource_get_link(resource));
}

static void nemoscreen_bind_output(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct nemoscreen *screen = (struct nemoscreen *)data;
	struct nemocompz *compz = screen->compz;
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
			compz->output.x,
			compz->output.y,
			screen->mmwidth,
			screen->mmheight,
			screen->subpixel,
			screen->make,
			screen->model,
			WL_OUTPUT_TRANSFORM_NORMAL);
	if (version >= 2)
		wl_output_send_scale(resource, 1);

	wl_output_send_mode(resource,
			WL_OUTPUT_MODE_CURRENT | WL_OUTPUT_MODE_PREFERRED,
			compz->output.width,
			compz->output.height,
			60000);

	if (version >= 2)
		wl_output_send_done(resource);
}

void nemoscreen_prepare(struct nemoscreen *screen)
{
	struct nemocompz *compz = screen->compz;

	screen->state = NEMOSCREEN_SCOPE_STATE;

	screen->geometry.sx = 1.0f;
	screen->geometry.sy = 1.0f;

	wl_signal_init(&screen->destroy_signal);
	wl_list_init(&screen->resource_list);

	wl_list_init(&screen->overlay_destroy_listener.link);

	pixman_region32_init(&screen->region);
	pixman_region32_init(&screen->damage);

	screen->id = ffs(~compz->screen_idpool) - 1;
	compz->screen_idpool |= 1 << screen->id;

	screen->frameout_timer = wl_event_loop_add_timer(compz->loop, nemoscreen_dispatch_frameout, screen);
	if (screen->frameout_timer == NULL)
		goto err1;

	screen->global = wl_global_create(compz->display, &wl_output_interface, 2, screen, nemoscreen_bind_output);
	if (screen->global == NULL)
		goto err2;

	return;

err2:
	wl_event_source_remove(screen->frameout_timer);

err1:
	nemoscreen_finish(screen);
}

void nemoscreen_finish(struct nemoscreen *screen)
{
	screen->compz->screen_idpool &= ~(1 << screen->id);

	wl_signal_emit(&screen->destroy_signal, screen);

	wl_list_remove(&screen->overlay_destroy_listener.link);

	wl_event_source_remove(screen->frameout_timer);

	pixman_region32_fini(&screen->region);
	pixman_region32_fini(&screen->damage);
}

static void nemoscreen_update_region(struct nemoscreen *screen)
{
	pixman_region32_fini(&screen->region);

	if (screen->transform.enable == 0) {
		pixman_region32_init_rect(&screen->region,
				screen->x, screen->y, screen->width, screen->height);

		screen->rx = screen->x;
		screen->ry = screen->y;
		screen->rw = screen->width;
		screen->rh = screen->height;
	} else {
		float minx = HUGE_VALF, miny = HUGE_VALF;
		float maxx = -HUGE_VALF, maxy = -HUGE_VALF;
		int32_t s[4][2] = {
			{ 0, 0 },
			{ 0, screen->height },
			{ screen->width, 0 },
			{ screen->width, screen->height }
		};
		float tx, ty;
		int i;

		for (i = 0; i < 4; i++) {
			struct nemovector v = { { s[i][0], s[i][1], 0.0f, 1.0f } };

			nemomatrix_transform_vector(&screen->transform.matrix, &v);

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

		screen->rx = floorf(minx + 0.5f);
		screen->ry = floorf(miny + 0.5f);
		screen->rw = floorf(maxx - minx + 0.5f);
		screen->rh = floorf(maxy - miny + 0.5f);

		pixman_region32_init_rect(&screen->region,
				screen->rx, screen->ry,
				screen->rw, screen->rh);
	}
}

void nemoscreen_update_transform(struct nemoscreen *screen)
{
	screen->transform.dirty = 0;

	if (screen->transform.custom == 0) {
		if (screen->transform.enable != 0) {
			struct nemomatrix *matrix = &screen->transform.matrix;
			struct nemomatrix *inverse = &screen->transform.inverse;

			nemomatrix_init_identity(matrix);

			if (screen->geometry.r != 0.0f) {
				nemomatrix_translate(matrix, -screen->geometry.px, -screen->geometry.py);
				nemomatrix_rotate(matrix, screen->transform.cosr, screen->transform.sinr);
				nemomatrix_translate(matrix, screen->geometry.px, screen->geometry.py);
			}

			if (screen->geometry.sx != 1.0f || screen->geometry.sy != 1.0f) {
				nemomatrix_translate(matrix, -screen->geometry.px, -screen->geometry.py);
				nemomatrix_scale(matrix, 1.0f / screen->geometry.sx, 1.0f / screen->geometry.sy);
				nemomatrix_translate(matrix, screen->geometry.px, screen->geometry.py);
			}

			nemomatrix_translate(matrix, screen->x, screen->y);

			if (nemomatrix_invert(inverse, matrix) < 0)
				screen->transform.enable = 0;
		} else {
			nemomatrix_init_translate(&screen->transform.matrix, screen->x, screen->y);
			nemomatrix_init_translate(&screen->transform.inverse, -screen->x, -screen->y);
		}
	}

	nemomatrix_init_identity(&screen->render.matrix);
	nemomatrix_multiply(&screen->render.matrix,
			&screen->transform.inverse);
	nemomatrix_translate(&screen->render.matrix,
			-screen->width / 2.0f,
			-screen->height / 2.0f);
	nemomatrix_scale(&screen->render.matrix,
			2.0f / screen->width,
			-2.0f / screen->height);

	nemoscreen_update_region(screen);

	nemoscreen_damage_dirty(screen);

	nemocompz_scene_dirty(screen->compz);
}

void nemoscreen_transform_to_global(struct nemoscreen *screen, float dx, float dy, float *x, float *y)
{
	if (screen->transform.enable != 0) {
		struct nemovector v = { { dx, dy, 0.0f, 1.0f } };

		nemomatrix_transform_vector(&screen->transform.matrix, &v);

		if (fabsf(v.f[3]) < 1e-6) {
			*x = 0.0f;
			*y = 0.0f;
			return;
		}

		*x = v.f[0] / v.f[3];
		*y = v.f[1] / v.f[3];
	} else {
		*x = dx + screen->x;
		*y = dy + screen->y;
	}
}

void nemoscreen_transform_from_global(struct nemoscreen *screen, float x, float y, float *dx, float *dy)
{
	if (screen->transform.enable != 0) {
		struct nemovector v = { { x, y, 0.0f, 1.0f } };

		nemomatrix_transform_vector(&screen->transform.inverse, &v);

		if (fabsf(v.f[3]) < 1e-6) {
			*dx = 0.0f;
			*dy = 0.0f;
			return;
		}

		*dx = v.f[0] / v.f[3];
		*dy = v.f[1] / v.f[3];
	} else {
		*dx = x - screen->x;
		*dy = y - screen->y;
	}
}

void nemoscreen_clear_transform(struct nemoscreen *screen)
{
	screen->geometry.sx = 1.0f;
	screen->geometry.sy = 1.0f;
	screen->geometry.px = 0.0f;
	screen->geometry.py = 0.0f;
	screen->geometry.r = 0.0f;

	screen->transform.cosr = cos(0.0f);
	screen->transform.sinr = sin(0.0f);

	screen->transform.enable = 0;
	screen->transform.dirty = 1;
	screen->transform.custom = 0;
}

void nemoscreen_set_position(struct nemoscreen *screen, int32_t x, int32_t y)
{
	if (screen->x == x && screen->y == y)
		return;

	screen->x = x;
	screen->y = y;

	screen->transform.dirty = 1;
}

void nemoscreen_set_rotation(struct nemoscreen *screen, float r)
{
	if (screen->geometry.r == r)
		return;

	screen->geometry.r = r;
	screen->transform.cosr = cos(-r);
	screen->transform.sinr = sin(-r);

	screen->transform.enable = 1;
	screen->transform.dirty = 1;
}

void nemoscreen_set_scale(struct nemoscreen *screen, float sx, float sy)
{
	if (screen->geometry.sx == sx && screen->geometry.sy == sy)
		return;

	screen->geometry.sx = sx;
	screen->geometry.sy = sy;

	screen->transform.enable = 1;
	screen->transform.dirty = 1;
}

void nemoscreen_set_pivot(struct nemoscreen *screen, float px, float py)
{
	if (screen->geometry.px == px && screen->geometry.py == py)
		return;

	screen->geometry.px = px;
	screen->geometry.py = py;

	screen->transform.dirty = 1;
}

int nemoscreen_set_custom(struct nemoscreen *screen, const char *cmd)
{
	struct nemomatrix *matrix = &screen->transform.matrix;
	struct nemomatrix *inverse = &screen->transform.inverse;

	nemomatrix_init_identity(matrix);
	nemomatrix_append_command(matrix, cmd);

	if (nemomatrix_invert(inverse, matrix) < 0)
		return -1;

	screen->transform.enable = 1;
	screen->transform.dirty = 1;
	screen->transform.custom = 1;

	return 0;
}

int nemoscreen_set_frameout(struct nemoscreen *screen, uint32_t timeout)
{
	screen->frameout_timeout = timeout;

	return 0;
}

static void nemoscreen_handle_overlay_destroy(struct wl_listener *listener, void *data)
{
	struct nemoscreen *screen = (struct nemoscreen *)container_of(listener, struct nemoscreen, overlay_destroy_listener);

	nemoscreen_set_overlay(screen, NULL);
}

int nemoscreen_set_overlay(struct nemoscreen *screen, struct nemoview *view)
{
	if (screen->overlay != NULL) {
		wl_list_remove(&screen->overlay_destroy_listener.link);
		wl_list_init(&screen->overlay_destroy_listener.link);
	}

	if (view != NULL) {
		screen->overlay_destroy_listener.notify = nemoscreen_handle_overlay_destroy;
		wl_signal_add(&view->destroy_signal, &screen->overlay_destroy_listener);

		screen->overlay = view;

		nemoscreen_set_state(screen, NEMOSCREEN_OVERLAY_STATE);
	} else {
		screen->overlay = NULL;

		nemoscreen_put_state(screen, NEMOSCREEN_OVERLAY_STATE);
	}

	return 0;
}

void nemoscreen_transform_dirty(struct nemoscreen *screen)
{
	screen->transform.dirty = 1;
}

void nemoscreen_damage_dirty(struct nemoscreen *screen)
{
	pixman_region32_union(&screen->damage, &screen->damage, &screen->region);
}
