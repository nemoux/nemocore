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
#include <datadevice.h>
#include <view.h>
#include <canvas.h>
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

	if (bin != NULL) {
		cx = x + move->dx;
		cy = y + move->dy;

		nemoview_set_position(bin->view, cx, cy);
		nemoview_schedule_repaint(bin->view);
	}
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

	if (state == WL_POINTER_BUTTON_STATE_RELEASED) {
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

	if (bin == NULL || pointer->button_count == 0)
		return -1;

	if (nemoshell_bin_has_state(bin, NEMOSHELL_BIN_FIXED_STATE) != 0)
		return 0;

	if (nemoview_has_grab(bin->view) != 0)
		wl_signal_emit(&bin->ungrab_signal, bin);

	if (nemoshell_bin_is_fullscreen(bin) != 0 || nemoshell_bin_is_maximized(bin) != 0)
		nemoshell_put_fullscreen_bin(shell, bin);

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

	nemoview_transform_to_global(bin->view,
			bin->view->content->width * bin->view->geometry.fx,
			bin->view->content->height * bin->view->geometry.fy,
			&tx, &ty);

	screen = nemoshell_get_fullscreen_on(shell, tx, ty, NEMOSHELL_FULLSCREEN_PITCH_TYPE);
	if (screen != NULL) {
		if (bin->fullscreen.target != NULL)
			screen = nemoshell_get_fullscreen_by(shell, bin->fullscreen.target);

		if (screen != NULL) {
			nemoshell_kill_fullscreen_bin(shell, screen->target);
			nemoshell_set_fullscreen_bin(shell, bin, screen);

			nemoshell_kill_region_bin(shell, shell->default_layer, screen->dx, screen->dy, screen->dw, screen->dh);

			if (screen->focus == NEMOSHELL_FULLSCREEN_ALL_FOCUS) {
				nemoseat_set_keyboard_focus(shell->compz->seat, bin->view);
				nemoseat_set_pointer_focus(shell->compz->seat, bin->view);
			}

			bin->has_scale = 1;
			bin->scale.serial = bin->next_serial;
			bin->scale.width = screen->dw;
			bin->scale.height = screen->dh;
		}
	}

	vieweffect_destroy(effect);
}

static void move_shellgrab_touchpoint_up(struct touchpoint_grab *base, uint32_t time, uint64_t touchid)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(base, struct shellgrab, base.touchpoint);
	struct shellgrab_move *move = (struct shellgrab_move *)container_of(grab, struct shellgrab_move, base);
	struct touchpoint *tp = base->touchpoint;
	struct shellbin *bin = grab->bin;
	int needs_notify = 1;

	touchpoint_up(tp);

	if (bin != NULL && nemoview_has_state(bin->view, NEMOVIEW_EFFECT_STATE) == 0) {
		if (nemoshell_check_touchgrab_duration(&move->touch, bin->shell->pitch.samples, bin->shell->pitch.max_duration) > 0) {
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

			if (nemoshell_bin_has_state(bin, NEMOSHELL_BIN_PITCHSCREEN_STATE) != 0) {
				vieweffect_set_dispatch_done(effect, move_shellgrab_dispatch_effect_done);
				vieweffect_set_userdata(effect, bin);
			}

			vieweffect_dispatch(bin->shell->compz, effect);
		} else if (nemoshell_bin_has_state(bin, NEMOSHELL_BIN_PITCHSCREEN_STATE) != 0) {
			struct nemoshell *shell = bin->shell;
			struct shellscreen *screen;
			float tx, ty;

			nemoview_transform_to_global(bin->view,
					bin->view->content->width * bin->view->geometry.fx,
					bin->view->content->height * bin->view->geometry.fy,
					&tx, &ty);

			screen = nemoshell_get_fullscreen_on(shell, tx, ty, NEMOSHELL_FULLSCREEN_PITCH_TYPE);
			if (screen != NULL) {
				if (bin->fullscreen.target != NULL)
					screen = nemoshell_get_fullscreen_by(shell, bin->fullscreen.target);

				if (screen != NULL) {
					nemoshell_kill_fullscreen_bin(shell, screen->target);
					nemoshell_set_fullscreen_bin(shell, bin, screen);

					nemoshell_kill_region_bin(shell, shell->default_layer, screen->dx, screen->dy, screen->dw, screen->dh);

					if (screen->focus == NEMOSHELL_FULLSCREEN_ALL_FOCUS) {
						nemoseat_set_keyboard_focus(shell->compz->seat, bin->view);
						nemoseat_set_pointer_focus(shell->compz->seat, bin->view);
					}
				}
			}
		}

		needs_notify = 0;
	}

	nemoshell_end_touchpoint_shellgrab(grab);
	nemoshell_end_touchgrab(&move->touch);
	free(move);

	if (bin != NULL && needs_notify != 0)
		nemoview_transform_notify(bin->view);

	if (tp->focus != NULL) {
		float sx, sy;

		nemoview_transform_from_global(tp->focus, tp->x, tp->y, &sx, &sy);

		nemocontent_touch_motion(tp, tp->focus->content, time, touchid, sx, sy, tp->x, tp->y);
		nemocontent_touch_up(tp, tp->focus->content, time, touchid);
	}
}

static void move_shellgrab_touchpoint_motion(struct touchpoint_grab *base, uint32_t time, uint64_t touchid, float x, float y)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(base, struct shellgrab, base.touchpoint);
	struct shellgrab_move *move = (struct shellgrab_move *)container_of(grab, struct shellgrab_move, base);
	struct touchpoint *tp = base->touchpoint;
	struct shellbin *bin = grab->bin;

	touchpoint_motion(tp, x, y);

	if (bin != NULL) {
		int32_t cx, cy;

		if (bin->reset_move != 0) {
			move->dx = bin->view->geometry.x - tp->x;
			move->dy = bin->view->geometry.y - tp->y;

			bin->reset_move = 0;
		}

		cx = tp->x + move->dx;
		cy = tp->y + move->dy;

		nemoview_set_position(bin->view, cx, cy);
		nemoview_schedule_repaint(bin->view);
	}
}

static void move_shellgrab_touchpoint_pressure(struct touchpoint_grab *base, uint32_t time, uint64_t touchid, float p)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(base, struct shellgrab, base.touchpoint);
	struct touchpoint *tp = base->touchpoint;

	touchpoint_pressure(tp, p);

	if (tp->focus != NULL) {
		nemocontent_touch_pressure(tp, tp->focus->content, time, touchid, p);
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
	move_shellgrab_touchpoint_pressure,
	move_shellgrab_touchpoint_frame,
	move_shellgrab_touchpoint_cancel
};

int nemoshell_move_canvas_by_touchpoint(struct nemoshell *shell, struct touchpoint *tp, struct shellbin *bin)
{
	struct shellgrab_move *move;

	if (bin == NULL)
		return -1;

	if (nemoshell_bin_has_state(bin, NEMOSHELL_BIN_FIXED_STATE) != 0)
		return 0;

	if (nemoview_has_grab(bin->view) != 0)
		wl_signal_emit(&bin->ungrab_signal, bin);

	if (nemoshell_bin_is_fullscreen(bin) != 0 || nemoshell_bin_is_maximized(bin) != 0)
		nemoshell_put_fullscreen_bin(shell, bin);

	move = (struct shellgrab_move *)malloc(sizeof(struct shellgrab_move));
	if (move == NULL)
		return -1;
	memset(move, 0, sizeof(struct shellgrab_move));

	move->dx = bin->view->geometry.x - tp->x;
	move->dy = bin->view->geometry.y - tp->y;

	bin->reset_move = 0;

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

int nemoshell_move_canvas_force(struct nemoshell *shell, struct touchpoint *tp, struct shellbin *bin)
{
	struct shellgrab_move *move;

	if (bin == NULL)
		return -1;

	if (nemoview_has_grab(bin->view) != 0)
		wl_signal_emit(&bin->ungrab_signal, bin);

	if (nemoshell_bin_is_fullscreen(bin) != 0 || nemoshell_bin_is_maximized(bin) != 0)
		nemoshell_put_fullscreen_bin(shell, bin);

	move = (struct shellgrab_move *)malloc(sizeof(struct shellgrab_move));
	if (move == NULL)
		return -1;
	memset(move, 0, sizeof(struct shellgrab_move));

	move->dx = bin->view->geometry.x - tp->x;
	move->dy = bin->view->geometry.y - tp->y;

	bin->reset_move = 0;

	nemoshell_start_touchpoint_shellgrab(shell, &move->base, &move_shellgrab_touchpoint_interface, bin, tp);
	nemoshell_start_touchgrab(shell, &move->touch, tp, NEMOSHELL_TOUCH_DEFAULT_TIMEOUT);

	nemoview_transform_notify(bin->view);

	move->base.bin_change_listener.notify = move_shellgrab_handle_bin_change;
	wl_signal_add(&bin->change_signal, &move->base.bin_change_listener);

	return 0;
}
