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
#include <pointer.h>
#include <datadevice.h>
#include <view.h>
#include <canvas.h>
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

static inline float pickgrab_calculate_distance(struct nemoview *view, float x, float y)
{
	float dx, dy;
	float x0, y0;

	nemoview_transform_to_global(view, view->geometry.px, view->geometry.py, &x0, &y0);

	dx = x - x0;
	dy = y - y0;

	return sqrtf(dx * dx + dy * dy);
}

static inline float pickgrab_calculate_angle(struct nemoview *view, float x, float y)
{
	float x0, y0;

	nemoview_transform_to_global(view, view->geometry.px, view->geometry.py, &x0, &y0);

	return atan2(x - x0, y - y0);
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
		struct nemocompz *compz = shell->compz;
		struct touchpoint *tp1 = pick->other->base.base.touchpoint.touchpoint;
		struct shellscreen *screen = NULL;
		int32_t width = nemoshell_bin_get_geometry_width(bin) * bin->view->geometry.sx;
		int32_t height = nemoshell_bin_get_geometry_height(bin) * bin->view->geometry.sy;

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

		if (bin->min_width >= width && bin->min_height >= height) {
			if (nemoview_has_state(bin->view, NEMOVIEW_CLOSE_STATE) == 0) {
				nemoshell_kill_bin(bin);
			} else {
				nemoshell_send_bin_close(bin);
			}

			goto closed;
		}

		if (nemoshell_bin_has_state(bin, NEMOSHELL_BIN_PITCHSCREEN_STATE) != 0) {
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
					width = screen->dw;
					height = screen->dh;

					nemoshell_kill_fullscreen_bin(shell, screen->target);
					nemoshell_set_fullscreen_bin(shell, bin, screen);

					nemoshell_kill_region_bin(shell, shell->default_layer, screen->dx, screen->dy, screen->dw, screen->dh);

					nemoseat_put_touchpoint_by_view(compz->seat, bin->view);

					if (screen->focus == NEMOSHELL_FULLSCREEN_ALL_FOCUS) {
						nemoseat_set_keyboard_focus(shell->compz->seat, bin->view);
						nemoseat_set_pointer_focus(shell->compz->seat, bin->view);
					}

					goto resized;
				}
			}
		}

		if (nemoshell_bin_has_flags(bin, NEMOSHELL_BIN_RESIZABLE_FLAG) != 0) {
			bin->resize_edges = WL_SHELL_SURFACE_RESIZE_LEFT | WL_SHELL_SURFACE_RESIZE_TOP;

			if (nemoshell_bin_has_state(bin, NEMOSHELL_BIN_PICKSCREEN_STATE) != 0) {
				screen = nemoshell_get_fullscreen_on(shell, tp0->x, tp0->y, NEMOSHELL_FULLSCREEN_PICK_TYPE);
				if (screen != NULL && (screen->sw <= width && screen->sh <= height)) {
					width = screen->dw;
					height = screen->dh;

					nemoshell_kill_fullscreen_bin(shell, screen->target);
					nemoshell_set_fullscreen_bin(shell, bin, screen);

					nemoshell_kill_region_bin(shell, shell->default_layer, screen->dx, screen->dy, screen->dw, screen->dh);

					nemoseat_put_touchpoint_by_view(compz->seat, bin->view);

					if (screen->focus == NEMOSHELL_FULLSCREEN_ALL_FOCUS) {
						nemoseat_set_keyboard_focus(compz->seat, bin->view);
						nemoseat_set_pointer_focus(compz->seat, bin->view);
					}

					goto resized;
				}
			}

			bin->callback->send_configure(bin->canvas, width, height);

resized:
			bin->has_scale = 1;
			bin->scale.serial = bin->next_serial;
			bin->scale.width = width;
			bin->scale.height = height;
		}

		nemoview_transform_notify(bin->view);

		touchpoint_update_grab(tp1);

closed:
		pick->other->done = 1;
	}

	nemoshell_end_touchpoint_shellgrab(grab);
	free(pick);

	if (tp0->focus != NULL) {
		float sx, sy;

		nemoview_transform_from_global(tp0->focus, tp0->x, tp0->y, &sx, &sy);

		nemocontent_touch_motion(tp0, tp0->focus->content, time, tp0->gid, sx, sy, tp0->x, tp0->y);
		nemocontent_touch_up(tp0, tp0->focus->content, time, tp0->gid);
	}
}

static void pick_shellgrab_touchpoint_motion(struct touchpoint_grab *base, uint32_t time, uint64_t touchid, float x, float y)
{
	struct touchpoint *tp = base->touchpoint;

	touchpoint_motion(tp, x, y);
}

static void pick_shellgrab_touchpoint_pressure(struct touchpoint_grab *base, uint32_t time, uint64_t touchid, float p)
{
	struct touchpoint *tp = base->touchpoint;

	touchpoint_pressure(tp, p);
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

		if ((shell->pick.flags & NEMOSHELL_PICK_ROTATE_FLAG) && (pick->type & NEMOSHELL_PICK_ROTATE_FLAG)) {
			if (d > shell->pick.rotate_distance)
				nemoview_set_rotation(bin->view, bin->view->geometry.r + pick->rotate.r - r);

			pick->rotate.r = pick->other->rotate.r = r;
		}

		if ((shell->pick.flags & NEMOSHELL_PICK_SCALE_FLAG) && (pick->type & NEMOSHELL_PICK_SCALE_FLAG)) {
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
			}

			pick->scale.distance = pick->other->scale.distance = d;
		}

		if ((shell->pick.flags & NEMOSHELL_PICK_RESIZE_FLAG) && (pick->type & NEMOSHELL_PICK_RESIZE_FLAG)) {
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

		if ((shell->pick.flags & NEMOSHELL_PICK_TRANSLATE_FLAG) && (pick->type & NEMOSHELL_PICK_TRANSLATE_FLAG)) {
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

	nemoshell_end_touchpoint_shellgrab(grab);
	free(pick);
}

static const struct touchpoint_grab_interface pick_shellgrab_touchpoint_interface = {
	pick_shellgrab_touchpoint_down,
	pick_shellgrab_touchpoint_up,
	pick_shellgrab_touchpoint_motion,
	pick_shellgrab_touchpoint_pressure,
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

	if (nemoshell_bin_is_fullscreen(bin) != 0 || nemoshell_bin_is_maximized(bin) != 0)
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
