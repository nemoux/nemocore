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

#define NEMOSHELL_VIEW_PITCH_EPSILON		(0.85f)

static int vieweffect_handle_frame(struct nemoeffect *base, uint32_t msecs)
{
	struct vieweffect *effect = (struct vieweffect *)container_of(base, struct vieweffect, base);
	struct nemoview *view = effect->view;

	if (effect->type & NEMOVIEW_PITCH_EFFECT) {
		struct nemocompz *compz = view->compz;
		double x, y, dx, dy;

		effect->pitch.velocity = MAX(effect->pitch.velocity - effect->pitch.friction * msecs, 0.0f);

retry:
		dx = (effect->pitch.dx * effect->pitch.velocity) * msecs;
		dy = (effect->pitch.dy * effect->pitch.velocity) * msecs;

		if (effect->pitch.velocity <= 1.0f) {
			nemoview_transform_notify(view);

			effect->type &= ~NEMOVIEW_PITCH_EFFECT;
		} else if (nemocompz_contain_view_near(compz, view, dx, dy) == 0) {
			effect->pitch.velocity *= NEMOSHELL_VIEW_PITCH_EPSILON;

			goto retry;
		} else {
			x = view->geometry.x + dx;
			y = view->geometry.y + dy;

			nemoview_set_position(view, x, y);
		}
	}

	nemoview_schedule_repaint(view);

	return effect->type == 0x0;
}

static void vieweffect_handle_done(struct nemoeffect *base)
{
	struct vieweffect *effect = (struct vieweffect *)container_of(base, struct vieweffect, base);

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

	nemoview_set_state(view, NEMOVIEW_EFFECT_STATE);

	return effect;
}

void vieweffect_destroy(struct vieweffect *effect)
{
	wl_list_remove(&effect->base.link);

	wl_list_remove(&effect->destroy_listener.link);

	nemoview_put_state(effect->view, NEMOVIEW_EFFECT_STATE);

	free(effect);
}

void vieweffect_dispatch(struct nemocompz *compz, struct vieweffect *effect)
{
	nemocompz_dispatch_effect(compz, &effect->base);
}

int vieweffect_revoke(struct nemocompz *compz, struct nemoview *view)
{
	struct nemoeffect *anim, *next;

	wl_list_for_each_safe(anim, next, &compz->effect_list, link) {
		struct vieweffect *effect = (struct vieweffect *)container_of(anim, struct vieweffect, base);

		if (effect->view == view) {
			vieweffect_destroy(effect);
		}
	}

	return 0;
}
