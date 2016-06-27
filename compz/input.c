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
	if (node->screen_destroy_listener.notify) {
		wl_list_remove(&node->screen_destroy_listener.link);
		node->screen_destroy_listener.notify = NULL;
	}

	if (screen != NULL) {
		node->screen_destroy_listener.notify = nemoinput_handle_screen_destroy;
		wl_signal_add(&screen->destroy_signal, &node->screen_destroy_listener);
	}

	node->screen = screen;
}

void nemoinput_put_screen(struct inputnode *node)
{
	node->screen = NULL;

	wl_list_remove(&node->screen_destroy_listener.link);
}

void nemoinput_set_geometry(struct inputnode *node, int32_t x, int32_t y, int32_t width, int32_t height)
{
	node->x = x;
	node->y = y;
	node->width = width;
	node->height = height;
}

int nemoinput_set_transform(struct inputnode *node, const char *cmd)
{
	struct nemomatrix *matrix = &node->transform.matrix;
	struct nemomatrix *inverse = &node->transform.inverse;

	nemomatrix_init_identity(matrix);
	nemomatrix_append_command(matrix, cmd);

	if (nemomatrix_invert(inverse, matrix) < 0)
		return -1;

	node->transform.enable = 1;

	return 0;
}

void nemoinput_transform_to_global(struct inputnode *node, float dx, float dy, float *x, float *y)
{
	if (node->transform.enable != 0) {
		struct nemovector v = { { dx, dy, 0.0f, 1.0f } };

		nemomatrix_transform(&node->transform.inverse, &v);

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

		nemomatrix_transform(&node->transform.matrix, &v);

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
