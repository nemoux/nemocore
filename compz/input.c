#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <input.h>
#include <compz.h>
#include <screen.h>
#include <nemoitem.h>
#include <nemomisc.h>

static void nemoinput_handle_screen_destroy(struct wl_listener *listener, void *data)
{
	struct inputnode *node = (struct inputnode *)container_of(listener, struct inputnode, screen_destroy_listener);

	node->screen = NULL;

	wl_list_remove(&node->screen_destroy_listener.link);
	wl_list_init(&node->screen_destroy_listener.link);
}

void nemoinput_set_screen(struct inputnode *node, struct nemoscreen *screen)
{
	if (node->screen != NULL) {
		wl_list_remove(&node->screen_destroy_listener.link);
		wl_list_init(&node->screen_destroy_listener.link);
	}

	if (screen != NULL) {
		node->screen_destroy_listener.notify = nemoinput_handle_screen_destroy;
		wl_signal_add(&screen->destroy_signal, &node->screen_destroy_listener);
	}

	node->screen = screen;
}

void nemoinput_put_screen(struct inputnode *node)
{
	if (node->screen != NULL) {
		node->screen = NULL;

		wl_list_remove(&node->screen_destroy_listener.link);
		wl_list_init(&node->screen_destroy_listener.link);
	}
}

void nemoinput_transform_to_global(struct inputnode *node, float dx, float dy, float *x, float *y)
{
	if (node->transform.enable != 0) {
		struct nemovector v = { { dx, dy, 0.0f, 1.0f } };

		nemomatrix_transform_vector(&node->transform.matrix, &v);

		if (fabsf(v.f[3]) < 1e-6) {
			*x = 0.0f;
			*y = 0.0f;
			return;
		}

		*x = v.f[0] / v.f[3];
		*y = v.f[1] / v.f[3];
	} else {
		*x = dx + node->x;
		*y = dy + node->y;
	}
}

void nemoinput_transform_from_global(struct inputnode *node, float x, float y, float *dx, float *dy)
{
	if (node->transform.enable != 0) {
		struct nemovector v = { { x, y, 0.0f, 1.0f } };

		nemomatrix_transform_vector(&node->transform.inverse, &v);

		if (fabsf(v.f[3]) < 1e-6) {
			*dx = 0.0f;
			*dy = 0.0f;
			return;
		}

		*dx = v.f[0] / v.f[3];
		*dy = v.f[1] / v.f[3];
	} else {
		*dx = x - node->x;
		*dy = y - node->y;
	}
}

void nemoinput_clear_transform(struct inputnode *node)
{
	node->geometry.sx = 1.0f;
	node->geometry.sy = 1.0f;
	node->geometry.px = 0.0f;
	node->geometry.py = 0.0f;
	node->geometry.r = 0.0f;

	node->transform.cosr = cos(0.0f);
	node->transform.sinr = sin(0.0f);

	node->transform.enable = 0;
	node->transform.dirty = 1;
	node->transform.custom = 0;
}

void nemoinput_update_transform(struct inputnode *node)
{
	node->transform.dirty = 0;

	if (node->transform.custom == 0) {
		if (node->transform.enable != 0) {
			struct nemomatrix *matrix = &node->transform.matrix;
			struct nemomatrix *inverse = &node->transform.inverse;

			nemomatrix_init_identity(matrix);

			if (node->geometry.r != 0.0f) {
				nemomatrix_translate(matrix, -node->geometry.px, -node->geometry.py);
				nemomatrix_rotate(matrix, node->transform.cosr, node->transform.sinr);
				nemomatrix_translate(matrix, node->geometry.px, node->geometry.py);
			}

			if (node->geometry.sx != 1.0f || node->geometry.sy != 1.0f) {
				nemomatrix_translate(matrix, -node->geometry.px, -node->geometry.py);
				nemomatrix_scale(matrix, node->geometry.sx, node->geometry.sy);
				nemomatrix_translate(matrix, node->geometry.px, node->geometry.py);
			}

			nemomatrix_translate(matrix, node->x, node->y);

			if (nemomatrix_invert(inverse, matrix) < 0)
				node->transform.enable = 0;
		} else {
			nemomatrix_init_translate(&node->transform.matrix, node->x, node->y);
			nemomatrix_init_translate(&node->transform.inverse, -node->x, -node->y);
		}
	}
}

void nemoinput_set_size(struct inputnode *node, int32_t width, int32_t height)
{
	if (node->width == width && node->height == height)
		return;

	node->width = width;
	node->height = height;
}

void nemoinput_set_position(struct inputnode *node, int32_t x, int32_t y)
{
	if (node->x == x && node->y == y)
		return;

	node->x = x;
	node->y = y;

	node->transform.dirty = 1;
}

void nemoinput_set_rotation(struct inputnode *node, float r)
{
	if (node->geometry.r == r)
		return;

	node->geometry.r = r;
	node->transform.cosr = cos(r);
	node->transform.sinr = sin(r);

	node->transform.enable = 1;
	node->transform.dirty = 1;
}

void nemoinput_set_scale(struct inputnode *node, float sx, float sy)
{
	if (node->geometry.sx == sx && node->geometry.sy == sy)
		return;

	node->geometry.sx = sx;
	node->geometry.sy = sy;

	node->transform.enable = 1;
	node->transform.dirty = 1;
}

void nemoinput_set_pivot(struct inputnode *node, float px, float py)
{
	if (node->geometry.px == px && node->geometry.py == py)
		return;

	node->geometry.px = px;
	node->geometry.py = py;

	node->transform.dirty = 1;
}

int nemoinput_set_custom(struct inputnode *node, const char *cmd)
{
	struct nemomatrix *matrix = &node->transform.matrix;
	struct nemomatrix *inverse = &node->transform.inverse;

	nemomatrix_init_identity(matrix);
	nemomatrix_append_command(matrix, cmd);

	if (nemomatrix_invert(inverse, matrix) < 0)
		return -1;

	node->transform.enable = 1;
	node->transform.dirty = 1;
	node->transform.custom = 1;

	return 0;
}
