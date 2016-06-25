#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <signal.h>
#include <limits.h>
#include <wayland-server.h>

#include <move.h>
#include <shell.h>
#include <compz.h>
#include <seat.h>
#include <screen.h>
#include <pointer.h>
#include <touch.h>
#include <datadevice.h>
#include <view.h>
#include <canvas.h>
#include <actor.h>
#include <vieweffect.h>
#include <nemolog.h>
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
	struct shellbin *bin = grab->bin;
	int32_t cx, cy;

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

	if (pointer->focus != NULL) {
		nemocontent_pointer_button(pointer, pointer->focus->content, time, button, state);
	}

	if (pointer->button_count == 0 && state == WL_POINTER_BUTTON_STATE_RELEASED) {
		nemoshell_end_pointer_shellgrab(grab);
		free(move);
	}
}

static void move_shellgrab_pointer_cancel(struct nemopointer_grab *base)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(base, struct shellgrab, base.pointer);
	struct shellgrab_move *move = (struct shellgrab_move *)container_of(grab, struct shellgrab_move, base);

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

int nemoshell_move_canvas_by_pointer(struct nemoshell *shell, struct nemopointer *pointer, struct shellbin *bin)
{
	struct shellgrab_move *move;

	if (bin == NULL)
		return -1;

	if (bin->state.fullscreen != 0 || bin->state.maximized != 0)
		return 0;

	if (bin->grabbed > 0)
		wl_signal_emit(&bin->ungrab_signal, bin);

	move = (struct shellgrab_move *)malloc(sizeof(struct shellgrab_move));
	if (move == NULL)
		return -1;
	memset(move, 0, sizeof(struct shellgrab_move));

	move->dx = bin->view->geometry.x - pointer->grab_x;
	move->dy = bin->view->geometry.y - pointer->grab_y;

	nemoshell_start_pointer_shellgrab(shell, &move->base, &move_shellgrab_pointer_interface, bin, pointer);

	nemoview_transform_notify(bin->view);

	move->base.bin_change_listener.notify = move_shellgrab_handle_bin_change;
	wl_signal_add(&bin->change_signal, &move->base.bin_change_listener);

	return 0;
}

static void move_shellgrab_touchpoint_down(struct touchpoint_grab *base, uint32_t time, uint64_t touchid, float x, float y)
{
}

static void move_shellgrab_dispatch_effect_done(struct nemoeffect *base)
{
	struct vieweffect *effect = (struct vieweffect *)container_of(base, struct vieweffect, base);
	struct shellbin *bin = (struct shellbin *)vieweffect_get_userdata(effect);
	struct nemoshell *shell = bin->shell;
	struct shellscreen *screen;
	float tx, ty;

	if (bin->on_pitchscreen != 0) {
		nemoview_transform_to_global(bin->view,
				bin->view->content->width * 0.5f,
				bin->view->content->height * 0.5f,
				&tx, &ty);

		screen = nemoshell_get_fullscreen_on(shell, tx, ty, NEMOSHELL_FULLSCREEN_PITCH_TYPE);
		if (screen != NULL) {
			struct shellbin *sbin, *nbin;

			wl_list_for_each_safe(sbin, nbin, &screen->bin_list, screen_link) {
				wl_list_remove(&sbin->screen_link);
				wl_list_init(&sbin->screen_link);

				if (sbin->resource != NULL) {
					kill(sbin->pid, SIGKILL);
				}
			}

			nemoshell_set_fullscreen_bin(shell, bin, screen);

			if (screen->focus == NEMOSHELL_FULLSCREEN_ALL_FOCUS) {
				nemoseat_set_keyboard_focus(shell->compz->seat, bin->view);
				nemoseat_set_pointer_focus(shell->compz->seat, bin->view);
				nemoseat_set_stick_focus(shell->compz->seat, bin->view);
			}
		}
	}

	if (shell->transform_bin != NULL)
		shell->transform_bin(shell->userdata, bin);

	vieweffect_destroy(effect);
}

static void move_shellgrab_touchpoint_up(struct touchpoint_grab *base, uint32_t time, uint64_t touchid)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(base, struct shellgrab, base.touchpoint);
	struct shellgrab_move *move = (struct shellgrab_move *)container_of(grab, struct shellgrab_move, base);
	struct touchpoint *tp = base->touchpoint;
	struct shellbin *bin = grab->bin;
	int needs_notify = 1;

	if (bin != NULL && bin->shell->is_logging_grab != 0)
		nemolog_message("MOVE", "[UP] %llu: (%u)\n", touchid, time);

	if (bin != NULL &&
			bin->state.fullscreen == 0 &&
			bin->state.maximized == 0 &&
			nemoshell_check_touchgrab_duration(&move->touch, bin->shell->pitch.samples, bin->shell->pitch.max_duration) > 0) {
		struct nemoshell *shell = bin->shell;
		struct vieweffect *effect;
		float dx, dy;

		nemoshell_update_touchgrab_velocity(&move->touch, shell->pitch.samples, &dx, &dy);

		effect = vieweffect_create(grab->bin->view);
		effect->type = NEMOVIEW_PITCH_EFFECT;
		effect->pitch.velocity = sqrtf(dx * dx + dy * dy) * shell->pitch.coefficient;
		effect->pitch.dx = dx / effect->pitch.velocity;
		effect->pitch.dy = dy / effect->pitch.velocity;
		effect->pitch.friction = shell->pitch.friction;

		if (shell->is_logging_grab != 0)
			nemolog_message("MOVE", "[PITCH] %llu: dx(%f) dy(%f) (%u)\n", touchid, effect->pitch.dx, effect->pitch.dy, time);

		vieweffect_set_dispatch_done(effect, move_shellgrab_dispatch_effect_done);
		vieweffect_set_userdata(effect, grab->bin);
		vieweffect_dispatch(bin->shell->compz, effect);

		needs_notify = 0;
	} else if (bin != NULL) {
		struct nemoshell *shell = bin->shell;

		if (shell->transform_bin != NULL)
			shell->transform_bin(shell->userdata, bin);
	}

	nemoshell_end_touchpoint_shellgrab(grab);
	nemoshell_end_touchgrab(&move->touch);
	free(move);

	if (bin != NULL && needs_notify != 0)
		nemoview_transform_notify(bin->view);

	if (tp->focus != NULL) {
		nemocontent_touch_up(tp, tp->focus->content, time, touchid);
	}
}

static void move_shellgrab_touchpoint_motion(struct touchpoint_grab *base, uint32_t time, uint64_t touchid, float x, float y)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(base, struct shellgrab, base.touchpoint);
	struct shellgrab_move *move = (struct shellgrab_move *)container_of(grab, struct shellgrab_move, base);
	struct touchpoint *tp = base->touchpoint;
	struct shellbin *bin = grab->bin;

	touchpoint_update(tp, x, y);

	if (bin != NULL &&
			bin->state.fullscreen == 0 &&
			bin->state.maximized == 0) {
		int32_t cx, cy;

		if (bin->reset_move != 0) {
			move->dx = bin->view->geometry.x - tp->x;
			move->dy = bin->view->geometry.y - tp->y;

			bin->reset_move = 0;
		}

		cx = x + move->dx;
		cy = y + move->dy;

		nemoview_set_position(bin->view, cx, cy);
		nemoview_schedule_repaint(bin->view);

		if (bin->shell->is_logging_grab != 0)
			nemolog_message("MOVE", "[MOTION] %llu: x(%f) y(%f) (%u)\n", touchid, bin->view->geometry.x, bin->view->geometry.y, time);
	}
}

static void move_shellgrab_touchpoint_frame(struct touchpoint_grab *base, uint32_t frameid)
{
}

static void move_shellgrab_touchpoint_cancel(struct touchpoint_grab *base)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(base, struct shellgrab, base.touchpoint);
	struct shellgrab_move *move = (struct shellgrab_move *)container_of(grab, struct shellgrab_move, base);

	nemoshell_end_touchpoint_shellgrab(grab);
	nemoshell_end_touchgrab(&move->touch);
	free(move);
}

static const struct touchpoint_grab_interface move_shellgrab_touchpoint_interface = {
	move_shellgrab_touchpoint_down,
	move_shellgrab_touchpoint_up,
	move_shellgrab_touchpoint_motion,
	move_shellgrab_touchpoint_frame,
	move_shellgrab_touchpoint_cancel
};

int nemoshell_move_canvas_by_touchpoint(struct nemoshell *shell, struct touchpoint *tp, struct shellbin *bin)
{
	struct shellgrab_move *move;

	if (bin == NULL)
		return -1;

	if (bin->state.fullscreen != 0 || bin->state.maximized != 0)
		return 0;

	if (bin->grabbed > 0)
		touchpoint_done_grab(tp);

	move = (struct shellgrab_move *)malloc(sizeof(struct shellgrab_move));
	if (move == NULL)
		return -1;
	memset(move, 0, sizeof(struct shellgrab_move));

	move->dx = bin->view->geometry.x - tp->x;
	move->dy = bin->view->geometry.y - tp->y;

	bin->reset_move = 0;

	if (shell->is_logging_grab != 0)
		nemolog_message("MOVE", "[DOWN] %llu: x(%f) y(%f)\n", tp->gid, bin->view->geometry.x, bin->view->geometry.y);

	nemoshell_start_touchpoint_shellgrab(shell, &move->base, &move_shellgrab_touchpoint_interface, bin, tp);
	nemoshell_start_touchgrab(shell, &move->touch, tp, NEMOSHELL_TOUCH_DEFAULT_TIMEOUT);

	nemoview_transform_notify(bin->view);

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
		return nemoshell_move_canvas_by_pointer(shell, pointer, bin);
	}

	tp = nemoseat_get_touchpoint_by_grab_serial(shell->compz->seat, serial);
	if (tp != NULL) {
		return nemoshell_move_canvas_by_touchpoint(shell, tp, bin);
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
	struct nemoactor *actor = grab->actor;
	int32_t cx, cy;

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

	if (pointer->focus != NULL) {
		nemocontent_pointer_button(pointer, pointer->focus->content, time, button, state);
	}

	if (pointer->button_count == 0 && state == WL_POINTER_BUTTON_STATE_RELEASED) {
		nemoshell_end_pointer_actorgrab(grab);
		free(move);
	}
}

static void move_actorgrab_pointer_cancel(struct nemopointer_grab *base)
{
	struct actorgrab *grab = (struct actorgrab *)container_of(base, struct actorgrab, base.pointer);
	struct actorgrab_move *move = (struct actorgrab_move *)container_of(grab, struct actorgrab_move, base);

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

int nemoshell_move_actor_by_pointer(struct nemoshell *shell, struct nemopointer *pointer, struct nemoactor *actor)
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

	move->base.shell = shell;

	move->dx = actor->view->geometry.x - pointer->grab_x;
	move->dy = actor->view->geometry.y - pointer->grab_y;

	nemoshell_start_pointer_actorgrab(shell, &move->base, &move_actorgrab_pointer_interface, actor, pointer);

	nemoview_transform_notify(actor->view);

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
	struct nemoactor *actor = grab->actor;
	struct nemoshell *shell = grab->shell;
	int needs_notify = 1;

	if (actor != NULL && nemoshell_check_touchgrab_duration(&move->touch, shell->pitch.samples, shell->pitch.max_duration) > 0) {
		struct vieweffect *effect;
		float dx, dy;

		nemoshell_update_touchgrab_velocity(&move->touch, shell->pitch.samples, &dx, &dy);

		effect = vieweffect_create(actor->view);
		effect->type = NEMOVIEW_PITCH_EFFECT;
		effect->pitch.velocity = sqrtf(dx * dx + dy * dy) * shell->pitch.coefficient;
		effect->pitch.dx = dx / effect->pitch.velocity;
		effect->pitch.dy = dy / effect->pitch.velocity;
		effect->pitch.friction = shell->pitch.friction;

		vieweffect_dispatch(tp->touch->seat->compz, effect);

		needs_notify = 0;
	}

	nemoshell_end_touchpoint_actorgrab(grab);
	nemoshell_end_touchgrab(&move->touch);
	free(move);

	if (actor != NULL && needs_notify != 0)
		nemoview_transform_notify(actor->view);

	if (tp->focus != NULL) {
		nemocontent_touch_up(tp, tp->focus->content, time, touchid);
	}
}

static void move_actorgrab_touchpoint_motion(struct touchpoint_grab *base, uint32_t time, uint64_t touchid, float x, float y)
{
	struct actorgrab *grab = (struct actorgrab *)container_of(base, struct actorgrab, base.touchpoint);
	struct actorgrab_move *move = (struct actorgrab_move *)container_of(grab, struct actorgrab_move, base);
	struct touchpoint *tp = base->touchpoint;
	struct nemoactor *actor = grab->actor;
	int32_t cx, cy;

	touchpoint_update(tp, x, y);

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

static void move_actorgrab_touchpoint_frame(struct touchpoint_grab *base, uint32_t frameid)
{
}

static void move_actorgrab_touchpoint_cancel(struct touchpoint_grab *base)
{
	struct actorgrab *grab = (struct actorgrab *)container_of(base, struct actorgrab, base.touchpoint);
	struct actorgrab_move *move = (struct actorgrab_move *)container_of(grab, struct actorgrab_move, base);

	nemoshell_end_touchpoint_actorgrab(grab);
	nemoshell_end_touchgrab(&move->touch);
	free(move);
}

static const struct touchpoint_grab_interface move_actorgrab_touchpoint_interface = {
	move_actorgrab_touchpoint_down,
	move_actorgrab_touchpoint_up,
	move_actorgrab_touchpoint_motion,
	move_actorgrab_touchpoint_frame,
	move_actorgrab_touchpoint_cancel
};

int nemoshell_move_actor_by_touchpoint(struct nemoshell *shell, struct touchpoint *tp, struct nemoactor *actor)
{
	struct actorgrab_move *move;

	if (actor == NULL)
		return -1;

	if (actor->grabbed > 0)
		touchpoint_done_grab(tp);

	move = (struct actorgrab_move *)malloc(sizeof(struct actorgrab_move));
	if (move == NULL)
		return -1;
	memset(move, 0, sizeof(struct actorgrab_move));

	move->dx = actor->view->geometry.x - tp->x;
	move->dy = actor->view->geometry.y - tp->y;

	nemoshell_start_touchpoint_actorgrab(shell, &move->base, &move_actorgrab_touchpoint_interface, actor, tp);
	nemoshell_start_touchgrab(shell, &move->touch, tp, NEMOSHELL_TOUCH_DEFAULT_TIMEOUT);

	nemoview_transform_notify(actor->view);

	return 0;
}

int nemoshell_move_actor(struct nemoshell *shell, struct nemoactor *actor, uint32_t serial)
{
	struct nemopointer *pointer;
	struct touchpoint *tp;

	pointer = nemoseat_get_pointer_by_grab_serial(shell->compz->seat, serial);
	if (pointer != NULL) {
		return nemoshell_move_actor_by_pointer(shell, pointer, actor);
	}

	tp = nemoseat_get_touchpoint_by_grab_serial(shell->compz->seat, serial);
	if (tp != NULL) {
		return nemoshell_move_actor_by_touchpoint(shell, tp, actor);
	}

	return 0;
}
