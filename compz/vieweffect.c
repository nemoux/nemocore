#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <vieweffect.h>
#include <view.h>
#include <content.h>
#include <compz.h>
#include <nemomisc.h>

static int vieweffect_correct_position(struct nemoview *view, double *dx, double *dy, double *x, double *y)
{
	double tx, ty;
	int done = 0;

	tx = *dx < 0.0f ? view->geometry.x + *dx : view->geometry.x + view->content->width + *dx;
	ty = *dy < 0.0f ? view->geometry.y + *dy : view->geometry.y + view->content->height + *dy;

	if (pixman_region32_contains_point(&view->compz->region, tx, ty, NULL)) {
		*x = view->geometry.x + *dx;
		*y = view->geometry.y + *dy;
	} else {
		pixman_box32_t *extents;

		extents = pixman_region32_extents(&view->compz->region);
		if (view->geometry.x + *dx < extents->x1)
			*x = extents->x1;
		else if (view->geometry.x + view->content->width + *dx > extents->x2)
			*x = extents->x2 - view->content->width;
		else
			*x = view->geometry.x;

		if (view->geometry.y + *dy < extents->y1)
			*y = extents->y1;
		else if (view->geometry.y + view->content->height + *dy > extents->y2)
			*y = extents->y2 - view->content->height;
		else
			*y = view->geometry.y;

		done = 1;
	}

	return done;
}

static int vieweffect_handle_frame(struct nemoeffect *base, uint32_t msecs)
{
	struct vieweffect *effect = (struct vieweffect *)container_of(base, struct vieweffect, base);

	if (effect->type & NEMO_VIEW_PITCH_EFFECT) {
		double x, y, dx, dy;

		effect->pitch.velocity = MAX(effect->pitch.velocity - effect->pitch.friction * msecs / 1000.0f, 0.0f);

		dx = (effect->pitch.dx * effect->pitch.velocity) * msecs / 1000.0f;
		dy = (effect->pitch.dy * effect->pitch.velocity) * msecs / 1000.0f;

		if (vieweffect_correct_position(effect->view, &dx, &dy, &x, &y) != 0) {
			effect->type &= ~NEMO_VIEW_PITCH_EFFECT;
		}

		nemoview_set_position(effect->view, x, y);

		if (effect->pitch.velocity <= 1e-6) {
			effect->type &= ~NEMO_VIEW_PITCH_EFFECT;
		}
	}

	nemoview_schedule_repaint(effect->view);

	return effect->type == 0x0;
}

static void vieweffect_handle_done(struct nemoeffect *base)
{
	struct vieweffect *effect = (struct vieweffect *)container_of(base, struct vieweffect, base);

	if (wl_list_empty(&base->link))
		vieweffect_destroy(effect);
}

static void vieweffect_handle_destroy(struct wl_listener *listener, void *data)
{
	struct vieweffect *effect = (struct vieweffect *)container_of(listener, struct vieweffect, destroy_listener);

	vieweffect_destroy(effect);
}

struct vieweffect *vieweffect_create(struct nemoview *view)
{
	struct vieweffect *effect;

	effect = (struct vieweffect *)malloc(sizeof(struct vieweffect));
	if (effect == NULL)
		return NULL;
	memset(effect, 0, sizeof(struct vieweffect));

	effect->base.frame = vieweffect_handle_frame;
	effect->base.done = vieweffect_handle_done;

	wl_list_init(&effect->base.link);

	effect->view = view;

	effect->destroy_listener.notify = vieweffect_handle_destroy;
	wl_signal_add(&view->destroy_signal, &effect->destroy_listener);

	return effect;
}

void vieweffect_destroy(struct vieweffect *effect)
{
	wl_list_remove(&effect->base.link);

	wl_list_remove(&effect->destroy_listener.link);

	free(effect);
}

void vieweffect_dispatch(struct nemocompz *compz, struct vieweffect *effect)
{
	nemocompz_dispatch_effect(compz, &effect->base);
}
