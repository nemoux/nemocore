#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <signal.h>
#include <wayland-server.h>
#include <wayland-nemo-shell-server-protocol.h>

#include <pick.h>
#include <grab.h>
#include <shell.h>
#include <compz.h>
#include <seat.h>
#include <screen.h>
#include <touch.h>
#include <datadevice.h>
#include <view.h>
#include <canvas.h>
#include <actor.h>
#include <content.h>
#include <vieweffect.h>
#include <nemolog.h>
#include <nemomisc.h>

static inline float pickgrab_calculate_touchpoint_distance(struct touchpoint *tp0, struct touchpoint *tp1)
{
	float dx, dy;

	dx = tp1->x - tp0->x;
	dy = tp1->y - tp0->y;

	return sqrtf(dx * dx + dy * dy);
}

static inline float pickgrab_calculate_touchpoint_angle(struct touchpoint *tp0, struct touchpoint *tp1)
{
	return atan2(tp1->x - tp0->x, tp1->y - tp0->y);
}

static void pick_shellgrab_touchpoint_down(struct touchpoint_grab *base, uint32_t time, uint64_t touchid, float x, float y)
{
}

static void pick_shellgrab_touchpoint_up(struct touchpoint_grab *base, uint32_t time, uint64_t touchid)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(base, struct shellgrab, base.touchpoint);
	struct shellgrab_pick *pick = (struct shellgrab_pick *)container_of(grab, struct shellgrab_pick, base);
	struct touchpoint *tp0 = base->touchpoint;
	struct shellbin *bin = grab->bin;

	touchpoint_up(tp0);

	if (bin != NULL && pick->done == 0) {
		struct nemoshell *shell = bin->shell;
		struct touchpoint *tp1 = pick->other->base.base.touchpoint.touchpoint;

		if (bin->reset_pivot != 0) {
			if (bin->view->geometry.has_pivot == 0) {
				nemoview_correct_pivot(bin->view,
						nemoshell_bin_get_geometry_width(bin) * bin->scale.ax,
						nemoshell_bin_get_geometry_height(bin) * bin->scale.ay);
			}

			bin->reset_pivot = 0;
		}

		if (bin->reset_move != 0) {
			pick->move.dx = pick->other->move.dx = bin->view->geometry.x - (pick->tp0->x + pick->tp1->x) / 2.0f;
			pick->move.dy = pick->other->move.dy = bin->view->geometry.y - (pick->tp0->y + pick->tp1->y) / 2.0f;

			bin->reset_move = 0;
		}

		if (bin->view->geometry.sx != 1.0f || bin->view->geometry.sy != 1.0f) {
			struct nemocompz *compz = shell->compz;
			struct shellscreen *screen = NULL;
			int32_t width = nemoshell_bin_get_geometry_width(bin) * bin->view->geometry.sx;
			int32_t height = nemoshell_bin_get_geometry_height(bin) * bin->view->geometry.sy;

			if (shell->is_logging_grab != 0)
				nemolog_message("PICK", "[UP:SCALE] %llu: sx(%f) sy(%f) width(%d) height(%d) (%u)\n", touchid, bin->view->geometry.sx, bin->view->geometry.sy, width, height, time);

			if (bin->min_width >= width || bin->min_height >= height) {
				if (nemoview_has_state(bin->view, NEMOVIEW_CLOSE_STATE) == 0) {
					kill(bin->pid, SIGKILL);
				} else {
					nemoshell_send_bin_close(bin);
				}
			} else if (nemoview_has_state(bin->view, NEMOVIEW_RESIZE_STATE) != 0) {
				bin->resize_edges = WL_SHELL_SURFACE_RESIZE_LEFT | WL_SHELL_SURFACE_RESIZE_TOP;

				if (nemoshell_bin_has_state(bin, NEMOSHELL_BIN_PICKSCREEN_STATE) != 0) {
					if (nemocompz_get_scene_width(compz) * shell->pick.fullscreen_scale <= width ||
							nemocompz_get_scene_height(compz) * shell->pick.fullscreen_scale <= height)
						screen = nemoshell_get_fullscreen_on(shell, tp0->x, tp0->y, NEMOSHELL_FULLSCREEN_PICK_TYPE);

					if (screen != NULL) {
						nemoshell_set_fullscreen_bin(shell, bin, screen);

						nemoseat_put_touchpoint_by_view(compz->seat, bin->view);

						if (screen->focus == NEMOSHELL_FULLSCREEN_ALL_FOCUS) {
							nemoseat_set_keyboard_focus(compz->seat, bin->view);
							nemoseat_set_pointer_focus(compz->seat, bin->view);
							nemoseat_set_stick_focus(compz->seat, bin->view);
						}
					} else {
						bin->callback->send_configure(bin->canvas, width, height);
					}
				} else {
					bin->callback->send_configure(bin->canvas, width, height);
				}

				bin->has_scale = 1;
				bin->scale.serial = bin->next_serial;
				bin->scale.width = width;
				bin->scale.height = height;
			}
		}

		if (shell->transform_bin != NULL)
			shell->transform_bin(shell->userdata, bin);

		nemoview_transform_notify(bin->view);

		touchpoint_update_grab(tp1);

		pick->other->done = 1;
	}

	nemoshell_end_touchpoint_shellgrab(grab);
	free(pick);

	if (tp0->focus != NULL) {
		float sx, sy;

		nemoview_transform_from_global(tp0->focus, tp0->x, tp0->y, &sx, &sy);

		nemocontent_touch_motion(tp0, tp0->focus->content, time, tp0->gid, sx, sy, tp0->x, tp0->y);
		nemocontent_touch_up(tp0, tp0->focus->content, time, tp0->gid);

		touchpoint_set_focus(tp0, NULL);
	}
}

static void pick_shellgrab_touchpoint_motion(struct touchpoint_grab *base, uint32_t time, uint64_t touchid, float x, float y)
{
	struct touchpoint *tp = base->touchpoint;

	touchpoint_motion(tp, x, y);
}

static void pick_shellgrab_touchpoint_frame(struct touchpoint_grab *base, uint32_t frameid)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(base, struct shellgrab, base.touchpoint);
	struct shellgrab_pick *pick = (struct shellgrab_pick *)container_of(grab, struct shellgrab_pick, base);
	struct shellbin *bin = grab->bin;

	if (bin != NULL && pick->done == 0) {
		struct nemoshell *shell = bin->shell;
		struct touchpoint *tp = base->touchpoint;
		float d = pickgrab_calculate_touchpoint_distance(pick->tp0, pick->tp1);
		float r = pickgrab_calculate_touchpoint_angle(pick->tp0, pick->tp1);
		uint64_t touchid = tp->id;

		if (pick->frameid == frameid)
			return;
		pick->frameid = pick->other->frameid = frameid;

		if (bin->reset_pivot != 0) {
			if (bin->view->geometry.has_pivot == 0) {
				nemoview_correct_pivot(bin->view,
						nemoshell_bin_get_geometry_width(bin) * bin->scale.ax,
						nemoshell_bin_get_geometry_height(bin) * bin->scale.ay);
			}

			bin->reset_pivot = 0;
		}

		if (bin->reset_move != 0) {
			pick->move.dx = pick->other->move.dx = bin->view->geometry.x - (pick->tp0->x + pick->tp1->x) / 2.0f;
			pick->move.dy = pick->other->move.dy = bin->view->geometry.y - (pick->tp0->y + pick->tp1->y) / 2.0f;

			bin->reset_move = 0;
		}

		if ((shell->pick.flags & NEMOSHELL_PICK_ROTATE_FLAG) && (pick->type & (1 << NEMO_SURFACE_PICK_TYPE_ROTATE))) {
			if (d > shell->pick.rotate_distance) {
				nemoview_set_rotation(bin->view, bin->view->geometry.r + pick->rotate.r - r);

				if (shell->is_logging_grab != 0)
					nemolog_message("PICK", "[FRAME:ROTATE] %llu: r(%f) (%u)\n", touchid, bin->view->geometry.r, time);
			}

			pick->rotate.r = pick->other->rotate.r = r;
		}

		if ((shell->pick.flags & NEMOSHELL_PICK_SCALE_FLAG) && (pick->type & (1 << NEMO_SURFACE_PICK_TYPE_SCALE))) {
			if (d > shell->pick.scale_distance) {
				float s = d / pick->scale.distance;

				if (bin->view->geometry.sx * s * nemoshell_bin_get_geometry_width(bin) > bin->max_width ||
						bin->view->geometry.sy * s * nemoshell_bin_get_geometry_height(bin) > bin->max_height) {
					double sx = (double)bin->max_width / (double)nemoshell_bin_get_geometry_width(bin);
					double sy = (double)bin->max_height / (double)nemoshell_bin_get_geometry_height(bin);

					if (sx > sy) {
						nemoview_set_scale(bin->view, sy, sy);
					} else {
						nemoview_set_scale(bin->view, sx, sx);
					}
				} else {
					nemoview_set_scale(bin->view,
							bin->view->geometry.sx * s,
							bin->view->geometry.sy * s);
				}

				if (shell->is_logging_grab != 0)
					nemolog_message("PICK", "[FRAME:SCALE] %llu: sx(%f) sy(%f) (%u)\n", touchid, bin->view->geometry.sx, bin->view->geometry.sy, time);
			}

			pick->scale.distance = pick->other->scale.distance = d;
		}

		if ((shell->pick.flags & NEMOSHELL_PICK_RESIZE_FLAG) && (pick->type & (1 << NEMO_SURFACE_PICK_TYPE_RESIZE))) {
			if (fabsf(pick->resize.distance - d) > shell->pick.resize_interval) {
				int32_t width, height;

				if (bin->resize_edges & WL_SHELL_SURFACE_RESIZE_LEFT ||
						bin->resize_edges & WL_SHELL_SURFACE_RESIZE_RIGHT) {
					width = nemoshell_bin_get_geometry_width(bin) + (d - pick->resize.distance);
				} else {
					width = nemoshell_bin_get_geometry_width(bin);
				}

				if (bin->resize_edges & WL_SHELL_SURFACE_RESIZE_TOP ||
						bin->resize_edges & WL_SHELL_SURFACE_RESIZE_BOTTOM) {
					height = nemoshell_bin_get_geometry_height(bin) + (d - pick->resize.distance);
				} else {
					height = nemoshell_bin_get_geometry_height(bin);
				}

				width = MAX(width, bin->min_width);
				height = MAX(height, bin->min_height);
				width = MIN(width, bin->max_width);
				height = MIN(height, bin->max_height);

				bin->callback->send_configure(bin->canvas, width, height);

				pick->resize.distance = pick->other->resize.distance = d;
			}
		}

		if ((shell->pick.flags & NEMOSHELL_PICK_TRANSLATE_FLAG) && (pick->type & (1 << NEMO_SURFACE_PICK_TYPE_MOVE))) {
			float cx, cy;

			cx = pick->move.dx + (pick->tp0->x + pick->tp1->x) / 2.0f;
			cy = pick->move.dy + (pick->tp0->y + pick->tp1->y) / 2.0f;

			nemoview_set_position(bin->view, cx, cy);
		}

		nemoview_schedule_repaint(bin->view);
	}
}

static void pick_shellgrab_touchpoint_cancel(struct touchpoint_grab *base)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(base, struct shellgrab, base.touchpoint);
	struct shellgrab_pick *pick = (struct shellgrab_pick *)container_of(grab, struct shellgrab_pick, base);
	struct shellbin *bin = grab->bin;

	nemoshell_end_touchpoint_shellgrab(grab);
	free(pick);
}

static const struct touchpoint_grab_interface pick_shellgrab_touchpoint_interface = {
	pick_shellgrab_touchpoint_down,
	pick_shellgrab_touchpoint_up,
	pick_shellgrab_touchpoint_motion,
	pick_shellgrab_touchpoint_frame,
	pick_shellgrab_touchpoint_cancel
};

int nemoshell_pick_canvas_by_touchpoint(struct nemoshell *shell, struct touchpoint *tp0, struct touchpoint *tp1, uint32_t type, struct shellbin *bin)
{
	struct shellgrab_pick *pick0, *pick1;

	if (bin == NULL)
		return -1;

	if (nemoshell_bin_has_state(bin, NEMOSHELL_BIN_FIXED_STATE) != 0)
		return 0;

	if (nemoview_has_grab(bin->view) != 0)
		wl_signal_emit(&bin->ungrab_signal, bin);

	if (bin->config.fullscreen != 0 || bin->config.maximized != 0)
		nemoshell_put_fullscreen_bin(shell, bin);

	pick0 = (struct shellgrab_pick *)malloc(sizeof(struct shellgrab_pick));
	if (pick0 == NULL)
		return -1;
	memset(pick0, 0, sizeof(struct shellgrab_pick));

	pick1 = (struct shellgrab_pick *)malloc(sizeof(struct shellgrab_pick));
	if (pick1 == NULL)
		return -1;
	memset(pick1, 0, sizeof(struct shellgrab_pick));

	pick0->type = pick1->type = type;

	pick0->scale.distance = pick1->scale.distance = pickgrab_calculate_touchpoint_distance(tp0, tp1);
	pick0->resize.distance = pick1->resize.distance = pick0->scale.distance;

	pick0->rotate.r = pick1->rotate.r = pickgrab_calculate_touchpoint_angle(tp0, tp1);

	if (bin->view->geometry.has_pivot == 0) {
		float cx = (tp0->x + tp1->x) / 2.0f;
		float cy = (tp0->y + tp1->y) / 2.0f;
		float sx, sy;

		nemoview_transform_from_global(bin->view, cx, cy, &sx, &sy);

		nemoview_correct_pivot(bin->view, sx, sy);

		bin->scale.ax = sx / nemoshell_bin_get_geometry_width(bin);
		bin->scale.ay = sy / nemoshell_bin_get_geometry_height(bin);
	}

	pick0->move.dx = pick1->move.dx = bin->view->geometry.x - (tp0->x + tp1->x) / 2.0f;
	pick0->move.dy = pick1->move.dy = bin->view->geometry.y - (tp0->y + tp1->y) / 2.0f;

	bin->reset_pivot = 0;
	bin->reset_move = 0;

	pick0->tp0 = pick1->tp0 = tp0;
	pick0->tp1 = pick1->tp1 = tp1;

	pick0->other = pick1;
	pick1->other = pick0;

	if (shell->is_logging_grab != 0)
		nemolog_message("PICK", "[DOWN] %llu+%llu: x(%f) y(%f) sx(%f) sy(%f) r(%f)\n",
				tp0->gid, tp1->gid,
				bin->view->geometry.x, bin->view->geometry.y,
				bin->view->geometry.sx, bin->view->geometry.sy,
				bin->view->geometry.r);

	nemoshell_start_touchpoint_shellgrab(shell, &pick0->base, &pick_shellgrab_touchpoint_interface, bin, tp0);
	nemoshell_start_touchpoint_shellgrab(shell, &pick1->base, &pick_shellgrab_touchpoint_interface, bin, tp1);

	nemoview_transform_notify(bin->view);

	return 0;
}

int nemoshell_pick_canvas(struct nemoshell *shell, struct shellbin *bin, uint32_t serial0, uint32_t serial1, uint32_t type)
{
	struct touchpoint *tp0, *tp1;

	tp0 = nemoseat_get_touchpoint_by_grab_serial(shell->compz->seat, serial0);
	if (tp0 == NULL)
		return 0;
	tp1 = nemoseat_get_touchpoint_by_grab_serial(shell->compz->seat, serial1);
	if (tp1 == NULL)
		return 0;

	return nemoshell_pick_canvas_by_touchpoint(shell, tp0, tp1, type, bin);
}

static void pick_actorgrab_touchpoint_down(struct touchpoint_grab *base, uint32_t time, uint64_t touchid, float x, float y)
{
}

static void pick_actorgrab_touchpoint_up(struct touchpoint_grab *base, uint32_t time, uint64_t touchid)
{
	struct actorgrab *grab = (struct actorgrab *)container_of(base, struct actorgrab, base.touchpoint);
	struct actorgrab_pick *pick = (struct actorgrab_pick *)container_of(grab, struct actorgrab_pick, base);
	struct touchpoint *tp0 = base->touchpoint;
	struct nemoactor *actor = grab->actor;

	touchpoint_up(tp0);

	if (actor != NULL && pick->done == 0) {
		struct nemoshell *shell = grab->shell;
		struct touchpoint *tp1 = pick->other->base.base.touchpoint.touchpoint;

		if (actor->view->geometry.sx != 1.0f || actor->view->geometry.sy != 1.0f) {
			struct nemocompz *compz = actor->compz;
			struct nemoview *view = actor->view;
			int32_t width = actor->view->content->width * actor->view->geometry.sx;
			int32_t height = actor->view->content->height * actor->view->geometry.sy;

			if (actor->min_width >= width || actor->min_height >= height) {
				nemoactor_dispatch_destroy(actor);
			} else if (nemoview_has_state(actor->view, NEMOVIEW_RESIZE_STATE) != 0) {
				int32_t sx, sy;
				float fromx, fromy, tox, toy;

				nemoview_set_scale(view, 1.0f, 1.0f);
				nemoview_update_transform(view);

				sx = (width - actor->base.width) * -actor->scale.ax;
				sy = (height - actor->base.height) * -actor->scale.ay;

				nemoview_transform_to_global(view, 0.0f, 0.0f, &fromx, &fromy);
				nemoview_transform_to_global(view, sx, sy, &tox, &toy);

				nemoview_set_position(view,
						view->geometry.x + tox - fromx,
						view->geometry.y + toy - fromy);

				nemoactor_dispatch_resize(actor, width, height);
				nemoactor_dispatch_frame(actor);
			}
		}

		pick->other->done = 1;
	}

	nemoshell_end_touchpoint_actorgrab(grab);
	free(pick);

	if (actor != NULL)
		nemoview_transform_notify(actor->view);

	if (tp0->focus != NULL) {
		float sx, sy;

		nemoview_transform_from_global(tp0->focus, tp0->x, tp0->y, &sx, &sy);

		nemocontent_touch_motion(tp0, tp0->focus->content, time, tp0->gid, sx, sy, tp0->x, tp0->y);
		nemocontent_touch_up(tp0, tp0->focus->content, time, tp0->gid);

		touchpoint_set_focus(tp0, NULL);
	}
}

static void pick_actorgrab_touchpoint_motion(struct touchpoint_grab *base, uint32_t time, uint64_t touchid, float x, float y)
{
	struct touchpoint *tp = base->touchpoint;

	touchpoint_motion(tp, x, y);
}

static void pick_actorgrab_touchpoint_frame(struct touchpoint_grab *base, uint32_t frameid)
{
	struct actorgrab *grab = (struct actorgrab *)container_of(base, struct actorgrab, base.touchpoint);
	struct actorgrab_pick *pick = (struct actorgrab_pick *)container_of(grab, struct actorgrab_pick, base);
	struct nemoactor *actor = grab->actor;

	if (actor != NULL && pick->done == 0) {
		struct nemoshell *shell = grab->shell;
		struct touchpoint *tp = base->touchpoint;
		float d = pickgrab_calculate_touchpoint_distance(pick->tp0, pick->tp1);
		float r = pickgrab_calculate_touchpoint_angle(pick->tp0, pick->tp1);
		uint64_t touchid = tp->id;

		if (pick->frameid == frameid)
			return;
		pick->frameid = pick->other->frameid = frameid;

		if ((shell->pick.flags & NEMOSHELL_PICK_ROTATE_FLAG) && (pick->type & (1 << NEMO_SURFACE_PICK_TYPE_ROTATE))) {
			if (d > shell->pick.rotate_distance) {
				nemoview_set_rotation(actor->view, actor->view->geometry.r + pick->rotate.r - r);
			}

			pick->rotate.r = pick->other->rotate.r = r;
		}

		if ((shell->pick.flags & NEMOSHELL_PICK_SCALE_FLAG) && (pick->type & (1 << NEMO_SURFACE_PICK_TYPE_SCALE))) {
			if (d > shell->pick.scale_distance) {
				float s = d / pick->scale.distance;

				if (actor->view->geometry.sx * s * actor->view->content->width > actor->max_width ||
						actor->view->geometry.sy * s * actor->view->content->height > actor->max_height) {
					double sx = (double)actor->max_width / (double)actor->view->content->width;
					double sy = (double)actor->max_height / (double)actor->view->content->height;

					if (sx > sy) {
						nemoview_set_scale(actor->view, sy, sy);
					} else {
						nemoview_set_scale(actor->view, sx, sx);
					}
				} else {
					nemoview_set_scale(actor->view,
							actor->view->geometry.sx * s,
							actor->view->geometry.sy * s);
				}
			}

			pick->scale.distance = pick->other->scale.distance = d;
		}

		if ((shell->pick.flags & NEMOSHELL_PICK_TRANSLATE_FLAG) && (pick->type & (1 << NEMO_SURFACE_PICK_TYPE_MOVE))) {
			float cx, cy;

			cx = pick->move.dx + (pick->tp0->x + pick->tp1->x) / 2.0f;
			cy = pick->move.dy + (pick->tp0->y + pick->tp1->y) / 2.0f;

			nemoview_set_position(actor->view, cx, cy);
		}

		nemoview_schedule_repaint(actor->view);
	}
}

static void pick_actorgrab_touchpoint_cancel(struct touchpoint_grab *base)
{
	struct actorgrab *grab = (struct actorgrab *)container_of(base, struct actorgrab, base.touchpoint);
	struct actorgrab_pick *pick = (struct actorgrab_pick *)container_of(grab, struct actorgrab_pick, base);

	nemoshell_end_touchpoint_actorgrab(grab);
	free(pick);
}

static const struct touchpoint_grab_interface pick_actorgrab_touchpoint_interface = {
	pick_actorgrab_touchpoint_down,
	pick_actorgrab_touchpoint_up,
	pick_actorgrab_touchpoint_motion,
	pick_actorgrab_touchpoint_frame,
	pick_actorgrab_touchpoint_cancel
};

int nemoshell_pick_actor_by_touchpoint(struct nemoshell *shell, struct touchpoint *tp0, struct touchpoint *tp1, uint32_t type, struct nemoactor *actor)
{
	struct actorgrab_pick *pick0, *pick1;

	if (actor == NULL)
		return -1;

	if (nemoview_has_grab(actor->view) != 0)
		wl_signal_emit(&actor->ungrab_signal, actor);

	pick0 = (struct actorgrab_pick *)malloc(sizeof(struct actorgrab_pick));
	if (pick0 == NULL)
		return -1;
	memset(pick0, 0, sizeof(struct actorgrab_pick));

	pick1 = (struct actorgrab_pick *)malloc(sizeof(struct actorgrab_pick));
	if (pick1 == NULL)
		return -1;
	memset(pick1, 0, sizeof(struct actorgrab_pick));

	pick0->type = pick1->type = type;

	pick0->scale.distance = pick1->scale.distance = pickgrab_calculate_touchpoint_distance(tp0, tp1);

	pick0->rotate.r = pick1->rotate.r = pickgrab_calculate_touchpoint_angle(tp0, tp1);

	if (actor->view->geometry.has_pivot == 0) {
		float cx = (tp0->x + tp1->x) / 2.0f;
		float cy = (tp0->y + tp1->y) / 2.0f;
		float sx, sy;

		nemoview_transform_from_global(actor->view, cx, cy, &sx, &sy);

		nemoview_correct_pivot(actor->view, sx, sy);

		actor->scale.ax = sx / actor->view->content->width;
		actor->scale.ay = sy / actor->view->content->height;
	}

	pick0->move.dx = pick1->move.dx = actor->view->geometry.x - (tp0->x + tp1->x) / 2.0f;
	pick0->move.dy = pick1->move.dy = actor->view->geometry.y - (tp0->y + tp1->y) / 2.0f;

	pick0->tp0 = pick1->tp0 = tp0;
	pick0->tp1 = pick1->tp1 = tp1;

	pick0->other = pick1;
	pick1->other = pick0;

	nemoshell_start_touchpoint_actorgrab(shell, &pick0->base, &pick_actorgrab_touchpoint_interface, actor, tp0);
	nemoshell_start_touchpoint_actorgrab(shell, &pick1->base, &pick_actorgrab_touchpoint_interface, actor, tp1);

	nemoview_transform_notify(actor->view);

	return 0;
}

int nemoshell_pick_actor(struct nemoshell *shell, struct nemoactor *actor, uint32_t serial0, uint32_t serial1, uint32_t type)
{
	struct touchpoint *tp0, *tp1;

	tp0 = nemoseat_get_touchpoint_by_grab_serial(shell->compz->seat, serial0);
	if (tp0 == NULL)
		return 0;
	tp1 = nemoseat_get_touchpoint_by_grab_serial(shell->compz->seat, serial1);
	if (tp1 == NULL)
		return 0;

	return nemoshell_pick_actor_by_touchpoint(shell, tp0, tp1, type, actor);
}
