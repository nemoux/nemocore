#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <limits.h>
#include <wayland-server.h>

#include <move.h>
#include <shell.h>
#include <compz.h>
#include <seat.h>
#include <screen.h>
#include <pointer.h>
#include <touch.h>
#include <view.h>
#include <canvas.h>
#include <actor.h>
#include <vieweffect.h>
#include <pitchfilter.h>
#include <nemomisc.h>

static void move_shellgrab_handle_bin_change(struct wl_listener *listener, void *data)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(listener, struct shellgrab, bin_change_listener);
	struct shellgrab_move *move = (struct shellgrab_move *)container_of(grab, struct shellgrab_move, base);
	struct shellbin *bin = grab->bin;

	move->dx += bin->last_sx;
	move->dy += bin->last_sy;
}

static void move_shellgrab_pointer_focus(struct nemopointer_grab *base)
{
}

static void move_shellgrab_pointer_motion(struct nemopointer_grab *base, uint32_t time, float x, float y)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(base, struct shellgrab, base.pointer);
	struct shellgrab_move *move = (struct shellgrab_move *)container_of(grab, struct shellgrab_move, base);
	struct nemopointer *pointer = base->pointer;
	struct pitchfilter *filter = move->filter;
	struct shellbin *bin = grab->bin;
	int32_t cx, cy;

	pitchfilter_dispatch(filter, x, y, x - pointer->x, y - pointer->y, time);

	nemopointer_move(pointer, x, y);

	if (grab->bin == NULL)
		return;

	cx = x + move->dx;
	cy = y + move->dy;

	nemoview_set_position(bin->view, cx, cy);
	nemoview_schedule_repaint(bin->view);
}

static void move_shellgrab_pointer_axis(struct nemopointer_grab *base, uint32_t time, uint32_t axis, float value)
{
}

static void move_shellgrab_pointer_button(struct nemopointer_grab *base, uint32_t time, uint32_t button, uint32_t state)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(base, struct shellgrab, base.pointer);
	struct shellgrab_move *move = (struct shellgrab_move *)container_of(grab, struct shellgrab_move, base);
	struct nemopointer *pointer = base->pointer;
	struct pitchfilter *filter = move->filter;

	if (pointer->focus != NULL) {
		nemocontent_pointer_button(pointer, pointer->focus->content, time, button, state);
	}

	if (pointer->button_count == 0 &&
			state == WL_POINTER_BUTTON_STATE_RELEASED) {
		if (grab->bin != NULL && pitchfilter_flush(filter) > 0) {
			struct vieweffect *effect;

			effect = vieweffect_create(grab->bin->view);
			effect->type = NEMO_VIEW_PITCH_EFFECT;
			effect->pitch.velocity = MIN(filter->dist / filter->dtime * 1000.0f, 5000.0f);
			effect->pitch.dx = filter->dx;
			effect->pitch.dy = filter->dy;
			effect->pitch.friction = 18.0f * 1000.0f;

			vieweffect_dispatch(grab->bin->shell->compz, effect);
		}

		pitchfilter_destroy(filter);
		nemoshell_end_pointer_shellgrab(grab);
		free(move);
	}
}

static void move_shellgrab_pointer_cancel(struct nemopointer_grab *base)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(base, struct shellgrab, base.pointer);
	struct shellgrab_move *move = (struct shellgrab_move *)container_of(grab, struct shellgrab_move, base);
	struct pitchfilter *filter = move->filter;

	pitchfilter_destroy(filter);
	nemoshell_end_pointer_shellgrab(grab);
	free(move);
}

static const struct nemopointer_grab_interface move_shellgrab_pointer_interface = {
	move_shellgrab_pointer_focus,
	move_shellgrab_pointer_motion,
	move_shellgrab_pointer_axis,
	move_shellgrab_pointer_button,
	move_shellgrab_pointer_cancel
};

int nemoshell_move_canvas_by_pointer(struct nemopointer *pointer, struct shellbin *bin)
{
	struct shellgrab_move *move;

	if (bin == NULL)
		return -1;

	if (bin->grabbed > 0)
		wl_signal_emit(&bin->ungrab_signal, bin);

	move = (struct shellgrab_move *)malloc(sizeof(struct shellgrab_move));
	if (move == NULL)
		return -1;
	memset(move, 0, sizeof(struct shellgrab_move));

	move->dx = bin->view->geometry.x - pointer->grab_x;
	move->dy = bin->view->geometry.y - pointer->grab_y;

	move->filter = pitchfilter_create(20.0f, 10);

	nemoshell_start_pointer_shellgrab(&move->base, &move_shellgrab_pointer_interface, bin, pointer);

	move->base.bin_change_listener.notify = move_shellgrab_handle_bin_change;
	wl_signal_add(&bin->change_signal, &move->base.bin_change_listener);

	return 0;
}

static void move_shellgrab_touchpoint_down(struct touchpoint_grab *base, uint32_t time, uint64_t touchid, float x, float y)
{
}

static void move_shellgrab_touchpoint_up(struct touchpoint_grab *base, uint32_t time, uint64_t touchid)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(base, struct shellgrab, base.touchpoint);
	struct shellgrab_move *move = (struct shellgrab_move *)container_of(grab, struct shellgrab_move, base);
	struct touchpoint *tp = base->touchpoint;
	struct pitchfilter *filter = move->filter;

	if (tp->focus != NULL &&
			tp->focus->canvas != NULL &&
			nemoshell_is_nemo_surface_for_canvas(tp->focus->canvas)) {
		nemocontent_touch_up(tp, tp->focus->content, time, touchid);
	}

	if (grab->bin != NULL && pitchfilter_flush(filter) > 0) {
		struct vieweffect *effect;

		effect = vieweffect_create(grab->bin->view);
		effect->type = NEMO_VIEW_PITCH_EFFECT;
		if (tp->touch->node->screen != NULL)
			effect->pitch.velocity = MIN(tp->touch->node->screen->diagonal * filter->dist / filter->dtime * 1000.0f, 5000.0f);
		else
			effect->pitch.velocity = 5000.0f;
		effect->pitch.dx = filter->dx;
		effect->pitch.dy = filter->dy;
		effect->pitch.friction = 12.0f * 1000.0f;

		vieweffect_dispatch(grab->bin->shell->compz, effect);
	}

	pitchfilter_destroy(filter);
	nemoshell_end_touchpoint_shellgrab(grab);
	free(move);
}

static void move_shellgrab_touchpoint_motion(struct touchpoint_grab *base, uint32_t time, uint64_t touchid, float x, float y)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(base, struct shellgrab, base.touchpoint);
	struct shellgrab_move *move = (struct shellgrab_move *)container_of(grab, struct shellgrab_move, base);
	struct touchpoint *tp = base->touchpoint;
	struct pitchfilter *filter = move->filter;
	struct shellbin *bin = grab->bin;
	int32_t cx, cy;

	pitchfilter_dispatch(filter, x, y, x - tp->x, y - tp->y, time);

	touchpoint_move(tp, x, y);

	if (grab->bin == NULL)
		return;

	if (tp->focus != NULL &&
			tp->focus->canvas != NULL &&
			nemoshell_is_nemo_surface_for_canvas(tp->focus->canvas)) {
		float sx, sy;

		nemoview_transform_from_global(tp->focus, x, y, &sx, &sy);

		nemocontent_touch_motion(tp, tp->focus->content, time, touchid, sx, sy, x, y);
	}

	cx = x + move->dx;
	cy = y + move->dy;

	nemoview_set_position(bin->view, cx, cy);
	nemoview_schedule_repaint(bin->view);
}

static void move_shellgrab_touchpoint_frame(struct touchpoint_grab *base)
{
}

static void move_shellgrab_touchpoint_cancel(struct touchpoint_grab *base)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(base, struct shellgrab, base.touchpoint);
	struct shellgrab_move *move = (struct shellgrab_move *)container_of(grab, struct shellgrab_move, base);
	struct pitchfilter *filter = move->filter;

	pitchfilter_destroy(filter);
	nemoshell_end_touchpoint_shellgrab(grab);
	free(move);
}

static const struct touchpoint_grab_interface move_shellgrab_touchpoint_interface = {
	move_shellgrab_touchpoint_down,
	move_shellgrab_touchpoint_up,
	move_shellgrab_touchpoint_motion,
	move_shellgrab_touchpoint_frame,
	move_shellgrab_touchpoint_cancel
};

int nemoshell_move_canvas_by_touchpoint(struct touchpoint *tp, struct shellbin *bin)
{
	struct shellgrab_move *move;

	if (bin == NULL)
		return -1;

	if (bin->grabbed > 0)
		wl_signal_emit(&bin->ungrab_signal, bin);

	move = (struct shellgrab_move *)malloc(sizeof(struct shellgrab_move));
	if (move == NULL)
		return -1;
	memset(move, 0, sizeof(struct shellgrab_move));

	move->dx = bin->view->geometry.x - tp->x;
	move->dy = bin->view->geometry.y - tp->y;

	move->filter = pitchfilter_create(20.0f, 30);

	nemoshell_start_touchpoint_shellgrab(&move->base, &move_shellgrab_touchpoint_interface, bin, tp);

	move->base.bin_change_listener.notify = move_shellgrab_handle_bin_change;
	wl_signal_add(&bin->change_signal, &move->base.bin_change_listener);

	return 0;
}

int nemoshell_move_canvas(struct nemoshell *shell, struct shellbin *bin, uint32_t serial)
{
	struct nemopointer *pointer;
	struct touchpoint *tp;

	pointer = nemoseat_get_pointer_by_grab_serial(shell->compz->seat, serial);
	if (pointer != NULL) {
		return nemoshell_move_canvas_by_pointer(pointer, bin);
	}

	tp = nemoseat_get_touchpoint_by_grab_serial(shell->compz->seat, serial);
	if (tp != NULL) {
		return nemoshell_move_canvas_by_touchpoint(tp, bin);
	}

	return 0;
}

static void move_actorgrab_pointer_focus(struct nemopointer_grab *base)
{
}

static void move_actorgrab_pointer_motion(struct nemopointer_grab *base, uint32_t time, float x, float y)
{
	struct actorgrab *grab = (struct actorgrab *)container_of(base, struct actorgrab, base.pointer);
	struct actorgrab_move *move = (struct actorgrab_move *)container_of(grab, struct actorgrab_move, base);
	struct nemopointer *pointer = base->pointer;
	struct pitchfilter *filter = move->filter;
	struct nemoactor *actor = grab->actor;
	int32_t cx, cy;

	pitchfilter_dispatch(filter, x, y, x - pointer->x, y - pointer->y, time);

	nemopointer_move(pointer, x, y);

	if (grab->actor == NULL)
		return;

	cx = x + move->dx;
	cy = y + move->dy;

	nemoview_set_position(actor->view, cx, cy);
	nemoview_schedule_repaint(actor->view);
}

static void move_actorgrab_pointer_axis(struct nemopointer_grab *base, uint32_t time, uint32_t axis, float value)
{
}

static void move_actorgrab_pointer_button(struct nemopointer_grab *base, uint32_t time, uint32_t button, uint32_t state)
{
	struct actorgrab *grab = (struct actorgrab *)container_of(base, struct actorgrab, base.pointer);
	struct actorgrab_move *move = (struct actorgrab_move *)container_of(grab, struct actorgrab_move, base);
	struct nemopointer *pointer = base->pointer;
	struct pitchfilter *filter = move->filter;

	if (pointer->focus != NULL) {
		nemocontent_pointer_button(pointer, pointer->focus->content, time, button, state);
	}

	if (pointer->button_count == 0 &&
			state == WL_POINTER_BUTTON_STATE_RELEASED) {
		if (grab->actor != NULL && pitchfilter_flush(filter) > 0) {
			struct vieweffect *effect;

			effect = vieweffect_create(grab->actor->view);
			effect->type = NEMO_VIEW_PITCH_EFFECT;
			effect->pitch.velocity = MIN(filter->dist / filter->dtime * 1000.0f, 5000.0f);
			effect->pitch.dx = filter->dx;
			effect->pitch.dy = filter->dy;
			effect->pitch.friction = 18.0f * 1000.0f;

			vieweffect_dispatch(pointer->seat->compz, effect);
		}

		pitchfilter_destroy(filter);
		nemoshell_end_pointer_actorgrab(grab);
		free(move);
	}
}

static void move_actorgrab_pointer_cancel(struct nemopointer_grab *base)
{
	struct actorgrab *grab = (struct actorgrab *)container_of(base, struct actorgrab, base.pointer);
	struct actorgrab_move *move = (struct actorgrab_move *)container_of(grab, struct actorgrab_move, base);
	struct pitchfilter *filter = move->filter;

	pitchfilter_destroy(filter);
	nemoshell_end_pointer_actorgrab(grab);
	free(move);
}

static const struct nemopointer_grab_interface move_actorgrab_pointer_interface = {
	move_actorgrab_pointer_focus,
	move_actorgrab_pointer_motion,
	move_actorgrab_pointer_axis,
	move_actorgrab_pointer_button,
	move_actorgrab_pointer_cancel
};

int nemoshell_move_actor_by_pointer(struct nemopointer *pointer, struct nemoactor *actor)
{
	struct actorgrab_move *move;

	if (actor == NULL)
		return -1;

	if (actor->grabbed > 0)
		wl_signal_emit(&actor->ungrab_signal, actor);

	move = (struct actorgrab_move *)malloc(sizeof(struct actorgrab_move));
	if (move == NULL)
		return -1;
	memset(move, 0, sizeof(struct actorgrab_move));

	move->dx = actor->view->geometry.x - pointer->grab_x;
	move->dy = actor->view->geometry.y - pointer->grab_y;

	move->filter = pitchfilter_create(20.0f, 10);

	nemoshell_start_pointer_actorgrab(&move->base, &move_actorgrab_pointer_interface, actor, pointer);

	return 0;
}

static void move_actorgrab_touchpoint_down(struct touchpoint_grab *base, uint32_t time, uint64_t touchid, float x, float y)
{
}

static void move_actorgrab_touchpoint_up(struct touchpoint_grab *base, uint32_t time, uint64_t touchid)
{
	struct actorgrab *grab = (struct actorgrab *)container_of(base, struct actorgrab, base.touchpoint);
	struct actorgrab_move *move = (struct actorgrab_move *)container_of(grab, struct actorgrab_move, base);
	struct touchpoint *tp = base->touchpoint;
	struct pitchfilter *filter = move->filter;

	if (tp->focus != NULL) {
		nemocontent_touch_up(tp, tp->focus->content, time, touchid);
	}

	if (grab->actor != NULL && pitchfilter_flush(filter) > 0) {
		struct vieweffect *effect;

		effect = vieweffect_create(grab->actor->view);
		effect->type = NEMO_VIEW_PITCH_EFFECT;
		if (tp->touch->node->screen != NULL)
			effect->pitch.velocity = MIN(tp->touch->node->screen->diagonal * filter->dist / filter->dtime * 1000.0f, 5000.0f);
		else
			effect->pitch.velocity = 5000.0f;
		effect->pitch.dx = filter->dx;
		effect->pitch.dy = filter->dy;
		effect->pitch.friction = 12.0f * 1000.0f;

		vieweffect_dispatch(tp->touch->seat->compz, effect);
	}

	pitchfilter_destroy(filter);
	nemoshell_end_touchpoint_actorgrab(grab);
	free(move);
}

static void move_actorgrab_touchpoint_motion(struct touchpoint_grab *base, uint32_t time, uint64_t touchid, float x, float y)
{
	struct actorgrab *grab = (struct actorgrab *)container_of(base, struct actorgrab, base.touchpoint);
	struct actorgrab_move *move = (struct actorgrab_move *)container_of(grab, struct actorgrab_move, base);
	struct touchpoint *tp = base->touchpoint;
	struct pitchfilter *filter = move->filter;
	struct nemoactor *actor = grab->actor;
	int32_t cx, cy;

	pitchfilter_dispatch(filter, x, y, x - tp->x, y - tp->y, time);

	touchpoint_move(tp, x, y);

	if (grab->actor == NULL)
		return;

	if (tp->focus != NULL) {
		float sx, sy;

		nemoview_transform_from_global(tp->focus, x, y, &sx, &sy);

		nemocontent_touch_motion(tp, tp->focus->content, time, touchid, sx, sy, x, y);
	}

	cx = x + move->dx;
	cy = y + move->dy;

	nemoview_set_position(actor->view, cx, cy);
	nemoview_schedule_repaint(actor->view);
}

static void move_actorgrab_touchpoint_frame(struct touchpoint_grab *base)
{
}

static void move_actorgrab_touchpoint_cancel(struct touchpoint_grab *base)
{
	struct actorgrab *grab = (struct actorgrab *)container_of(base, struct actorgrab, base.touchpoint);
	struct actorgrab_move *move = (struct actorgrab_move *)container_of(grab, struct actorgrab_move, base);
	struct pitchfilter *filter = move->filter;

	pitchfilter_destroy(filter);
	nemoshell_end_touchpoint_actorgrab(grab);
	free(move);
}

static const struct touchpoint_grab_interface move_actorgrab_touchpoint_interface = {
	move_actorgrab_touchpoint_down,
	move_actorgrab_touchpoint_up,
	move_actorgrab_touchpoint_motion,
	move_actorgrab_touchpoint_frame,
	move_actorgrab_touchpoint_cancel
};

int nemoshell_move_actor_by_touchpoint(struct touchpoint *tp, struct nemoactor *actor)
{
	struct actorgrab_move *move;

	if (actor == NULL)
		return -1;

	if (actor->grabbed > 0)
		wl_signal_emit(&actor->ungrab_signal, actor);

	move = (struct actorgrab_move *)malloc(sizeof(struct actorgrab_move));
	if (move == NULL)
		return -1;
	memset(move, 0, sizeof(struct actorgrab_move));

	move->dx = actor->view->geometry.x - tp->x;
	move->dy = actor->view->geometry.y - tp->y;

	move->filter = pitchfilter_create(20.0f, 30);

	nemoshell_start_touchpoint_actorgrab(&move->base, &move_actorgrab_touchpoint_interface, actor, tp);

	return 0;
}

int nemoshell_move_actor(struct nemoshell *shell, struct nemoactor *actor, uint32_t serial)
{
	struct nemopointer *pointer;
	struct touchpoint *tp;

	pointer = nemoseat_get_pointer_by_grab_serial(shell->compz->seat, serial);
	if (pointer != NULL) {
		return nemoshell_move_actor_by_pointer(pointer, actor);
	}

	tp = nemoseat_get_touchpoint_by_grab_serial(shell->compz->seat, serial);
	if (tp != NULL) {
		return nemoshell_move_actor_by_touchpoint(tp, actor);
	}

	return 0;
}
