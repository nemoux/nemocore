#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <viewanimation.h>
#include <view.h>
#include <compz.h>
#include <nemoease.h>
#include <nemomisc.h>

static void viewanimation_handle_frame(struct nemoanimation *base, double progress)
{
	struct viewanimation *animation = (struct viewanimation *)container_of(base, struct viewanimation, base);

	if (animation->type & NEMOVIEW_TRANSLATE_ANIMATION) {
		float sx = animation->translate.base.x;
		float sy = animation->translate.base.y;
		float ex = animation->translate.x;
		float ey = animation->translate.y;

		nemoview_set_position(animation->view,
				(ex - sx) * progress + sx,
				(ey - sy) * progress + sy);
	}

	if (animation->type & NEMOVIEW_ROTATE_ANIMATION) {
		float start = animation->rotate.base.r;
		float end = animation->rotate.r;
		float dr = fabsf(end - start);

		if (start <= end) {
			nemoview_set_rotation(animation->view, (dr <= M_PI ? dr : dr - M_PI * 2.0f) * progress + start);
		} else {
			nemoview_set_rotation(animation->view, start - (dr <= M_PI ? dr : dr - M_PI * 2.0f) * progress);
		}
	}

	if (animation->type & NEMOVIEW_SCALE_ANIMATION) {
		float sx = animation->scale.base.sx;
		float sy = animation->scale.base.sy;
		float ex = animation->scale.sx;
		float ey = animation->scale.sy;

		nemoview_set_scale(animation->view,
				(ex - sx) * progress + sx,
				(ey - sy) * progress + sy);
	}

	if (animation->type & NEMOVIEW_ALPHA_ANIMATION) {
		float sv = animation->alpha.base.v;
		float ev = animation->alpha.v;

		nemoview_set_alpha(animation->view,
				(ev - sv) * progress + sv);
	}

	nemoview_schedule_repaint(animation->view);
}

static void viewanimation_handle_done(struct nemoanimation *base)
{
	struct viewanimation *animation = (struct viewanimation *)container_of(base, struct viewanimation, base);

	if (wl_list_empty(&base->link))
		viewanimation_destroy(animation);
}

static void viewanimation_handle_destroy(struct wl_listener *listener, void *data)
{
	struct viewanimation *animation = (struct viewanimation *)container_of(listener, struct viewanimation, destroy_listener);

	viewanimation_destroy(animation);
}

struct viewanimation *viewanimation_create(struct nemoview *view, uint32_t ease, uint32_t delay, uint32_t duration)
{
	struct viewanimation *animation;

	animation = (struct viewanimation *)malloc(sizeof(struct viewanimation));
	if (animation == NULL)
		return NULL;
	memset(animation, 0, sizeof(struct viewanimation));

	animation->base.delay = delay;
	animation->base.duration = duration;
	nemoease_set(&animation->base.ease, ease);

	animation->base.frame = viewanimation_handle_frame;
	animation->base.done = viewanimation_handle_done;

	wl_list_init(&animation->base.link);

	animation->view = view;

	animation->destroy_listener.notify = viewanimation_handle_destroy;
	wl_signal_add(&view->destroy_signal, &animation->destroy_listener);

	return animation;
}

struct viewanimation *viewanimation_create_cubic(struct nemoview *view, double sx, double sy, double ex, double ey, uint32_t delay, uint32_t duration)
{
	struct viewanimation *animation;

	animation = (struct viewanimation *)malloc(sizeof(struct viewanimation));
	if (animation == NULL)
		return NULL;
	memset(animation, 0, sizeof(struct viewanimation));

	animation->base.delay = delay;
	animation->base.duration = duration;
	nemoease_set_cubic(&animation->base.ease, sx, sy, ex, ey);

	animation->base.frame = viewanimation_handle_frame;
	animation->base.done = viewanimation_handle_done;

	wl_list_init(&animation->base.link);

	animation->view = view;

	animation->destroy_listener.notify = viewanimation_handle_destroy;
	wl_signal_add(&view->destroy_signal, &animation->destroy_listener);

	return animation;
}

void viewanimation_destroy(struct viewanimation *animation)
{
	wl_list_remove(&animation->base.link);

	wl_list_remove(&animation->destroy_listener.link);

	free(animation);
}

void viewanimation_dispatch(struct nemocompz *compz, struct viewanimation *animation)
{
	struct nemoview *view = animation->view;

	if (animation->type & NEMOVIEW_TRANSLATE_ANIMATION) {
		animation->translate.base.x = view->geometry.x;
		animation->translate.base.y = view->geometry.y;
	}

	if (animation->type & NEMOVIEW_ROTATE_ANIMATION) {
		animation->rotate.base.r = view->geometry.r;
	}

	if (animation->type & NEMOVIEW_SCALE_ANIMATION) {
		animation->scale.base.sx = view->geometry.sx;
		animation->scale.base.sy = view->geometry.sy;
	}

	if (animation->type & NEMOVIEW_ALPHA_ANIMATION) {
		animation->alpha.base.v = view->alpha;
	}

	nemocompz_dispatch_animation(compz, &animation->base);
}

int viewanimation_revoke(struct nemocompz *compz, struct nemoview *view, uint32_t type)
{
	struct nemoanimation *anim, *next;

	wl_list_for_each_safe(anim, next, &compz->animation_list, link) {
		struct viewanimation *animation = (struct viewanimation *)container_of(anim, struct viewanimation, base);

		if (animation->view == view) {
			animation->type &= ~type;

			if (animation->type == 0x0)
				viewanimation_destroy(animation);
		}
	}

	return 0;
}
