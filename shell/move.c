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

	pitchfilter_dispatch(filter, x - pointer->x, y - pointer->y, time);

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
		pitchfilter_dispatch(filter, 0.0f, 0.0f, time);

		if (grab->bin != NULL && pitchfilter_flush(filter) > 0) {
			struct nemoshell *shell = grab->bin->shell;
			struct vieweffect *effect;

			effect = vieweffect_create(grab->bin->view);
			effect->type = NEMO_VIEW_PITCH_EFFECT;
			effect->pitch.velocity = MIN(filter->dist / filter->dtime * shell->pitch.velocity, shell->pitch.max_velocity);
			effect->pitch.dx = filter->dx;
			effect->pitch.dy = filter->dy;
			effect->pitch.friction = shell->pitch.friction;

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

int nemoshell_move_canvas_by_pointer(struct nemoshell *shell, struct nemopointer *pointer, struct shellbin *bin)
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

	move->filter = pitchfilter_create(shell->pitch.max_samples, shell->pitch.dir_samples, shell->pitch.min_duration);

	nemoshell_start_pointer_shellgrab(&move->base, &move_shellgrab_pointer_interface, bin, pointer);

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

	nemoview_transform_to_global(bin->view,
			bin->view->content->width * 0.5f,
			bin->view->content->height * 0.5f,
			&tx, &ty);

	screen = nemoshell_get_fullscreen_on(shell, tx, ty, NEMO_SHELL_FULLSCREEN_PITCH_TYPE);
	if (screen != NULL) {
		struct shellbin *sbin, *nbin;

		wl_list_for_each_safe(sbin, nbin, &screen->bin_list, screen_link) {
			wl_list_remove(&sbin->screen_link);
			wl_list_init(&sbin->screen_link);

			if (sbin->resource != NULL) {
				struct wl_client *client = wl_resource_get_client(sbin->resource);
				pid_t pid;

				wl_client_get_credentials(client, &pid, NULL, NULL);

				kill(pid, SIGKILL);
			}
		}

		wl_list_insert(&screen->bin_list, &bin->screen_link);

		nemoshell_clear_bin_next_state(bin);
		bin->next_state.fullscreen = 1;
		bin->state_changed = 1;

		bin->type = NEMO_SHELL_SURFACE_NORMAL_TYPE;
		nemoshell_set_parent_bin(bin, NULL);

		bin->screen.x = screen->dx;
		bin->screen.y = screen->dy;
		bin->screen.width = screen->dw;
		bin->screen.height = screen->dh;
		bin->screen.r = screen->dr * M_PI / 180.0f;
		bin->has_screen = 1;

		nemoshell_send_bin_state(bin);
	}

	vieweffect_destroy(effect);
}

static void move_shellgrab_touchpoint_up(struct touchpoint_grab *base, uint32_t time, uint64_t touchid)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(base, struct shellgrab, base.touchpoint);
	struct shellgrab_move *move = (struct shellgrab_move *)container_of(grab, struct shellgrab_move, base);
	struct touchpoint *tp = base->touchpoint;
	struct pitchfilter *filter = move->filter;
	struct shellbin *bin = grab->bin;

	if (tp->focus != NULL) {
		nemocontent_touch_up(tp, tp->focus->content, time, touchid);
	}

	if (bin != NULL && bin->shell->is_logging_grab != 0)
		nemolog_message("MOVE", "[UP] %llu: (%u)\n", touchid, time);

	pitchfilter_dispatch(filter, 0.0f, 0.0f, time);

	if (bin != NULL && pitchfilter_flush(filter) > 0) {
		struct nemoshell *shell = bin->shell;
		struct vieweffect *effect;

		effect = vieweffect_create(grab->bin->view);
		effect->type = NEMO_VIEW_PITCH_EFFECT;
		effect->pitch.velocity = MIN(filter->dist / filter->dtime * shell->pitch.velocity, shell->pitch.max_velocity);
		effect->pitch.dx = filter->dx;
		effect->pitch.dy = filter->dy;
		effect->pitch.friction = shell->pitch.friction;

		if (shell->is_logging_grab != 0)
			nemolog_message("MOVE", "[PITCH] %llu: dx(%f) dy(%f) velocity(%f) (%u)\n", touchid, effect->pitch.dx, effect->pitch.dy, effect->pitch.velocity, time);

		if (grab->bin->on_pitchscreen != 0) {
			vieweffect_set_dispatch_done(effect, move_shellgrab_dispatch_effect_done);
			vieweffect_set_userdata(effect, grab->bin);
		}

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

	pitchfilter_dispatch(filter, x - tp->x, y - tp->y, time);

	touchpoint_move(tp, x, y);

	if (bin == NULL)
		return;

	if (tp->focus != NULL) {
		float sx, sy;

		nemoview_transform_from_global(tp->focus, x, y, &sx, &sy);

		nemocontent_touch_motion(tp, tp->focus->content, time, touchid, sx, sy, x, y);
	}

	cx = x + move->dx;
	cy = y + move->dy;

	nemoview_set_position(bin->view, cx, cy);
	nemoview_schedule_repaint(bin->view);

	if (bin->shell->is_logging_grab != 0)
		nemolog_message("MOVE", "[MOTION] %llu: x(%f) y(%f) (%u)\n", touchid, bin->view->geometry.x, bin->view->geometry.y, time);
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

int nemoshell_move_canvas_by_touchpoint(struct nemoshell *shell, struct touchpoint *tp, struct shellbin *bin)
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

	move->dx = bin->view->geometry.x - tp->x;
	move->dy = bin->view->geometry.y - tp->y;

	move->filter = pitchfilter_create(shell->pitch.max_samples, shell->pitch.dir_samples, shell->pitch.min_duration);

	if (shell->is_logging_grab != 0)
		nemolog_message("MOVE", "[DOWN] %llu: x(%f) y(%f)\n", tp->gid, bin->view->geometry.x, bin->view->geometry.y);

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
	struct pitchfilter *filter = move->filter;
	struct nemoactor *actor = grab->actor;
	int32_t cx, cy;

	pitchfilter_dispatch(filter, x - pointer->x, y - pointer->y, time);

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
		pitchfilter_dispatch(filter, 0.0f, 0.0f, time);

		if (grab->actor != NULL && pitchfilter_flush(filter) > 0) {
			struct nemoshell *shell = grab->shell;
			struct vieweffect *effect;

			effect = vieweffect_create(grab->actor->view);
			effect->type = NEMO_VIEW_PITCH_EFFECT;
			effect->pitch.velocity = MIN(filter->dist / filter->dtime * shell->pitch.velocity, shell->pitch.max_velocity);
			effect->pitch.dx = filter->dx;
			effect->pitch.dy = filter->dy;
			effect->pitch.friction = shell->pitch.friction;

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

	move->filter = pitchfilter_create(shell->pitch.max_samples, shell->pitch.dir_samples, shell->pitch.min_duration);

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

	pitchfilter_dispatch(filter, 0.0f, 0.0f, time);

	if (grab->actor != NULL && pitchfilter_flush(filter) > 0) {
		struct nemoshell *shell = grab->shell;
		struct vieweffect *effect;

		effect = vieweffect_create(grab->actor->view);
		effect->type = NEMO_VIEW_PITCH_EFFECT;
		effect->pitch.velocity = MIN(filter->dist / filter->dtime * shell->pitch.velocity, shell->pitch.max_velocity);
		effect->pitch.dx = filter->dx;
		effect->pitch.dy = filter->dy;
		effect->pitch.friction = shell->pitch.friction;

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

	pitchfilter_dispatch(filter, x - tp->x, y - tp->y, time);

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

int nemoshell_move_actor_by_touchpoint(struct nemoshell *shell, struct touchpoint *tp, struct nemoactor *actor)
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

	move->dx = actor->view->geometry.x - tp->x;
	move->dy = actor->view->geometry.y - tp->y;

	move->filter = pitchfilter_create(shell->pitch.max_samples, shell->pitch.dir_samples, shell->pitch.min_duration);

	nemoshell_start_touchpoint_actorgrab(&move->base, &move_actorgrab_touchpoint_interface, actor, tp);

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
