#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>
#include <wayland-nemo-shell-server-protocol.h>

#include <pick.h>
#include <grab.h>
#include <shell.h>
#include <compz.h>
#include <seat.h>
#include <screen.h>
#include <touch.h>
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

static inline float pickgrab_calculate_touchpoint_direction_degree(struct touchpoint *tp0, struct touchpoint *tp1)
{
	float d = fabsf(atan2(tp0->dx, tp0->dy) - atan2(tp1->dx, tp1->dy)) * 180.0f / M_PI;

	return MIN(d, 360.0f - d);
}

static void pick_shellgrab_touchpoint_down(struct touchpoint_grab *base, uint32_t time, uint64_t touchid, float x, float y)
{
}

static void pick_shellgrab_touchpoint_up(struct touchpoint_grab *base, uint32_t time, uint64_t touchid)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(base, struct shellgrab, base.touchpoint);
	struct shellgrab_pick *pick = (struct shellgrab_pick *)container_of(grab, struct shellgrab_pick, base);
	struct touchpoint *tp0 = base->touchpoint;
	struct touchpoint *tp1 = pick->other->base.base.touchpoint.touchpoint;
	struct shellbin *bin = grab->bin;
	struct nemoshell *shell;

	if (bin == NULL)
		goto out;
	shell = bin->shell;

	if (pick->has_reset != 0 && bin->reset_scale == 0) {
		pick->sx = 1.0f;
		pick->sy = 1.0f;

		pick->width = bin->view->content->width;
		pick->height = bin->view->content->height;

		if (bin->view->geometry.has_pivot == 0) {
			float sx, sy;

			nemoview_transform_from_global(bin->view, bin->px, bin->py, &sx, &sy);

			nemoview_correct_pivot(bin->view, sx, sy);
		}

		if (shell->is_logging_grab != 0)
			nemolog_message("PICK", "[UP:RESET] %llu: width(%d) height(%d) (%u)\n", touchid, pick->width, pick->height, time);
	}

	if (pick->type & (1 << NEMO_SURFACE_PICK_TYPE_SCALE)) {
		struct nemocompz *compz = shell->compz;
		struct shellscreen *screen = NULL;
		int32_t width = pick->width * pick->sx;
		int32_t height = pick->height * pick->sy;

		bin->reset_scale = 1;

		if (shell->is_logging_grab != 0)
			nemolog_message("PICK", "[UP:SCALE] %llu: sx(%f) sy(%f) width(%d) height(%d) (%u)\n", touchid, pick->sx, pick->sy, width, height, time);

		if (bin->on_pickscreen != 0) {
			if (nemocompz_get_scene_width(compz) * shell->pick.fullscreen_scale <= width &&
					nemocompz_get_scene_height(compz) * shell->pick.fullscreen_scale <= height)
				screen = nemoshell_get_fullscreen_on(shell, tp0->x, tp0->y, NEMO_SHELL_FULLSCREEN_PICK_TYPE);

			if (screen != NULL) {
				nemoshell_set_fullscreen_bin(shell, bin, screen);
				nemoshell_set_fullscreen_opaque(shell, bin);

				nemoseat_put_touchpoint_by_view(compz->seat, bin->view);

				if (screen->focus == NEMO_SHELL_FULLSCREEN_ALL_FOCUS) {
					nemoseat_set_keyboard_focus(compz->seat, bin->view);
					nemoseat_set_pointer_focus(compz->seat, bin->view);
					nemoseat_set_stick_focus(compz->seat, bin->view);
				}
			} else {
				bin->client->send_configure(bin->canvas, width, height);
			}
		} else {
			bin->client->send_configure(bin->canvas, width, height);
		}
	}

	nemoview_transform_done(bin->view);

	bin->resize_edges = 0;
	nemoshell_send_bin_configure(bin);

out:
	nemoshell_end_touchpoint_shellgrab(grab);
	nemoshell_end_touchpoint_shellgrab(&pick->other->base);
	free(pick->other);
	free(pick);

	if (tp0->focus != NULL) {
		nemocontent_touch_up(tp0, tp0->focus->content, time, tp0->gid);

		touchpoint_set_focus(tp0, NULL);
	}

	touchpoint_update_grab(tp1);
}

static void pick_shellgrab_touchpoint_motion(struct touchpoint_grab *base, uint32_t time, uint64_t touchid, float x, float y)
{
	struct touchpoint *tp = base->touchpoint;

	touchpoint_update_direction(tp, x, y);

	if (tp->focus != NULL) {
		float sx, sy;

		nemoview_transform_from_global(tp->focus, x, y, &sx, &sy);

		nemocontent_touch_motion(tp, tp->focus->content, time, touchid, sx, sy, x, y);
	}
}

static void pick_shellgrab_touchpoint_frame(struct touchpoint_grab *base, uint32_t frameid)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(base, struct shellgrab, base.touchpoint);
	struct shellgrab_pick *pick = (struct shellgrab_pick *)container_of(grab, struct shellgrab_pick, base);
	struct touchpoint *tp = base->touchpoint;
	struct shellbin *bin = grab->bin;
	struct nemoshell *shell;
	double distance = pickgrab_calculate_touchpoint_distance(pick->tp0, pick->tp1);
	float r = pickgrab_calculate_touchpoint_angle(pick->tp0, pick->tp1);
	float d = pickgrab_calculate_touchpoint_direction_degree(pick->tp0, pick->tp1);
	uint64_t touchid = tp->id;

	if (bin == NULL)
		return;
	shell = bin->shell;

	if (pick->frameid == frameid)
		return;
	pick->frameid = pick->other->frameid = frameid;

	if (pick->has_reset != 0 && bin->reset_scale == 0) {
		pick->sx = pick->other->sx = 1.0f;
		pick->sy = pick->other->sy = 1.0f;

		pick->width = pick->other->width = bin->view->content->width;
		pick->height = pick->other->height = bin->view->content->height;

		if (bin->view->geometry.has_pivot == 0) {
			float sx, sy;

			nemoview_transform_from_global(bin->view, bin->px, bin->py, &sx, &sy);

			nemoview_correct_pivot(bin->view, sx, sy);
		}

		pick->dx = pick->other->dx = bin->view->geometry.x - (pick->tp0->x + pick->tp1->x) / 2.0f;
		pick->dy = pick->other->dy = bin->view->geometry.y - (pick->tp0->y + pick->tp1->y) / 2.0f;

		pick->has_reset = pick->other->has_reset = 0;

		if (shell->is_logging_grab != 0)
			nemolog_message("PICK", "[FRAME:RESET] %llu: width(%d) height(%d) (%u)\n", touchid, pick->width, pick->height, time);
	}

	if (pick->type & (1 << NEMO_SURFACE_PICK_TYPE_ROTATE)) {
		if (distance > shell->pick.min_distance && d > shell->pick.rotate_degree) {
			if (fabsf(pick->rotate.r - r) < shell->pick.max_rotate) {
				pick->r = pick->other->r = pick->r + pick->rotate.r - r;

				nemoview_set_rotation(bin->view, pick->r);
			}

			if (shell->is_logging_grab != 0)
				nemolog_message("PICK", "[FRAME:ROTATE] %llu: r(%f) (%u)\n", touchid, pick->r, time);
		}

		pick->rotate.r = pick->other->rotate.r = r;
	}

	if (pick->type & (1 << NEMO_SURFACE_PICK_TYPE_SCALE) || pick->type & (1 << NEMO_SURFACE_PICK_TYPE_SCALEONLY)) {
		if (distance > shell->pick.min_distance && d > shell->pick.scale_degree) {
			float s = distance / pick->scale.distance;

			if (fabsf(1.0f - s) < shell->pick.max_scale) {
				if (pick->sx * s * pick->width > bin->max_width ||
						pick->sy * s * pick->height > bin->max_height) {
					double sx = (double)bin->max_width / (double)pick->width;
					double sy = (double)bin->max_height / (double)pick->height;

					if (sx > sy) {
						pick->sx = pick->other->sx = sy;
						pick->sy = pick->other->sy = sy;
					} else {
						pick->sx = pick->other->sx = sx;
						pick->sy = pick->other->sy = sx;
					}
				} else if (pick->sx * s * pick->width < bin->min_width ||
						pick->sy * s * pick->height < bin->min_height) {
					double sx = (double)bin->min_width / (double)pick->width;
					double sy = (double)bin->min_height / (double)pick->height;

					if (sx > sy) {
						pick->sx = pick->other->sx = sx;
						pick->sy = pick->other->sy = sx;
					} else {
						pick->sx = pick->other->sx = sy;
						pick->sy = pick->other->sy = sy;
					}
				} else {
					pick->sx = pick->other->sx = pick->sx * s;
					pick->sy = pick->other->sy = pick->sy * s;
				}

				nemoview_set_scale(bin->view, pick->sx, pick->sy);
			}

			if (shell->is_logging_grab != 0)
				nemolog_message("PICK", "[FRAME:SCALE] %llu: sx(%f) sy(%f) (%u)\n", touchid, pick->sx, pick->sy, time);
		}

		pick->scale.distance = pick->other->scale.distance = distance;
	}

	if (pick->type & (1 << NEMO_SURFACE_PICK_TYPE_RESIZE)) {
		if (fabsf(pick->resize.distance - distance) > shell->pick.resize_interval) {
			int32_t width, height;

			if (bin->resize_edges & WL_SHELL_SURFACE_RESIZE_LEFT ||
					bin->resize_edges & WL_SHELL_SURFACE_RESIZE_RIGHT) {
				width = pick->width + (distance - pick->resize.distance);
			} else {
				width = pick->width;
			}

			if (bin->resize_edges & WL_SHELL_SURFACE_RESIZE_TOP ||
					bin->resize_edges & WL_SHELL_SURFACE_RESIZE_BOTTOM) {
				height = pick->height + (distance - pick->resize.distance);
			} else {
				height = pick->height;
			}

			width = MAX(width, bin->min_width);
			height = MAX(height, bin->min_height);
			width = MIN(width, bin->max_width);
			height = MIN(height, bin->max_height);

			bin->client->send_configure(bin->canvas, width, height);

			pick->width = pick->other->width = width;
			pick->height = pick->other->height = height;

			pick->resize.distance = pick->other->resize.distance = distance;
		}
	}

	if (pick->type & (1 << NEMO_SURFACE_PICK_TYPE_MOVE)) {
		float cx, cy;

		cx = pick->dx + (pick->tp0->x + pick->tp1->x) / 2.0f;
		cy = pick->dy + (pick->tp0->y + pick->tp1->y) / 2.0f;

		nemoview_set_position(bin->view, cx, cy);
	}

	nemoview_schedule_repaint(bin->view);
}

static void pick_shellgrab_touchpoint_cancel(struct touchpoint_grab *base)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(base, struct shellgrab, base.touchpoint);
	struct shellgrab_pick *pick = (struct shellgrab_pick *)container_of(grab, struct shellgrab_pick, base);
	struct shellbin *bin = grab->bin;

	if (bin != NULL) {
		bin->resize_edges = 0;
		nemoshell_send_bin_configure(bin);
	}

	nemoshell_end_touchpoint_shellgrab(grab);
	nemoshell_end_touchpoint_shellgrab(&pick->other->base);
	free(pick->other);
	free(pick);
}

static const struct touchpoint_grab_interface pick_shellgrab_touchpoint_interface = {
	pick_shellgrab_touchpoint_down,
	pick_shellgrab_touchpoint_up,
	pick_shellgrab_touchpoint_motion,
	pick_shellgrab_touchpoint_frame,
	pick_shellgrab_touchpoint_cancel
};

typedef enum {
	NEMOSHELL_VIEW_TOP_AREA = 0,
	NEMOSHELL_VIEW_BOTTOM_AREA = 1,
	NEMOSHELL_VIEW_LEFT_AREA = 2,
	NEMOSHELL_VIEW_RIGHT_AREA = 3,
	NEMOSHELL_VIEW_LEFTTOP_AREA = 4,
	NEMOSHELL_VIEW_RIGHTTOP_AREA = 5,
	NEMOSHELL_VIEW_LEFTBOTTOM_AREA = 6,
	NEMOSHELL_VIEW_RIGHTBOTTOM_AREA = 7,
	NEMOSHELL_VIEW_CENTER_AREA,
	NEMOSHELL_VIEW_LAST_AREA = NEMOSHELL_VIEW_CENTER_AREA
} NemoShellViewArea;

static int nemoshell_pick_get_view_point_area(struct nemoview *view, float x, float y, float b)
{
	float sx, sy;
	float dx, dy;
	float mx = view->content->width * 0.5f;
	float my = view->content->height * 0.5f;
	float ex = view->content->width * b;
	float ey = view->content->height * b;
	int area = NEMOSHELL_VIEW_CENTER_AREA;
	int i;

	nemoview_transform_from_global(view, x, y, &sx, &sy);

	dx = sx > mx ? view->content->width - sx : sx;
	dy = sy > my ? view->content->height - sy : sy;

	if (dx < ex && dy < ey) {
		if (sx < mx && sy < my)
			area = NEMOSHELL_VIEW_LEFTTOP_AREA;
		else if (sx > mx && sy < my)
			area = NEMOSHELL_VIEW_RIGHTTOP_AREA;
		else if (sx < mx && sy > my)
			area = NEMOSHELL_VIEW_LEFTBOTTOM_AREA;
		else if (sx > mx && sy > my)
			area = NEMOSHELL_VIEW_RIGHTBOTTOM_AREA;
	} else if (dx < dy && dx < ex) {
		if (sx < mx)
			area = NEMOSHELL_VIEW_LEFT_AREA;
		else
			area = NEMOSHELL_VIEW_RIGHT_AREA;
	} else if (dy < dx && dy < ey) {
		if (sy < my)
			area = NEMOSHELL_VIEW_TOP_AREA;
		else
			area = NEMOSHELL_VIEW_BOTTOM_AREA;
	}

	return area;
}

int nemoshell_pick_canvas_by_touchpoint_on_area(struct nemoshell *shell, struct touchpoint *tp0, struct touchpoint *tp1, struct shellbin *bin)
{
	struct shellgrab_pick *pick0, *pick1;
	uint32_t type;
	int area0, area1;

	if (bin == NULL)
		return -1;

	if (bin->fixed > 0)
		return 0;

	if (bin->grabbed > 0)
		wl_signal_emit(&bin->ungrab_signal, bin);

	pick0 = (struct shellgrab_pick *)malloc(sizeof(struct shellgrab_pick));
	if (pick0 == NULL)
		return -1;
	memset(pick0, 0, sizeof(struct shellgrab_pick));

	pick1 = (struct shellgrab_pick *)malloc(sizeof(struct shellgrab_pick));
	if (pick1 == NULL)
		return -1;
	memset(pick1, 0, sizeof(struct shellgrab_pick));

	area0 = nemoshell_pick_get_view_point_area(bin->view, tp0->x, tp0->y, 0.35f);
	area1 = nemoshell_pick_get_view_point_area(bin->view, tp1->x, tp1->y, 0.35f);

	if (area0 == NEMOSHELL_VIEW_CENTER_AREA || area1 == NEMOSHELL_VIEW_CENTER_AREA) {
		bin->resize_edges = 0;
	} else if (area0 == NEMOSHELL_VIEW_LEFTTOP_AREA && area1 == NEMOSHELL_VIEW_RIGHTBOTTOM_AREA) {
		bin->resize_edges = WL_SHELL_SURFACE_RESIZE_RIGHT | WL_SHELL_SURFACE_RESIZE_BOTTOM;
	} else if (area0 == NEMOSHELL_VIEW_RIGHTBOTTOM_AREA && area1 == NEMOSHELL_VIEW_LEFTTOP_AREA) {
		bin->resize_edges = WL_SHELL_SURFACE_RESIZE_LEFT | WL_SHELL_SURFACE_RESIZE_TOP;
	} else if (area0 == NEMOSHELL_VIEW_LEFTBOTTOM_AREA && area1 == NEMOSHELL_VIEW_RIGHTTOP_AREA) {
		bin->resize_edges = WL_SHELL_SURFACE_RESIZE_RIGHT | WL_SHELL_SURFACE_RESIZE_TOP;
	} else if (area0 == NEMOSHELL_VIEW_RIGHTTOP_AREA && area1 == NEMOSHELL_VIEW_LEFTBOTTOM_AREA) {
		bin->resize_edges = WL_SHELL_SURFACE_RESIZE_LEFT | WL_SHELL_SURFACE_RESIZE_BOTTOM;
	} else if ((area0 == NEMOSHELL_VIEW_TOP_AREA || area0 == NEMOSHELL_VIEW_LEFTTOP_AREA || area0 == NEMOSHELL_VIEW_RIGHTTOP_AREA) &&
			(area1 == NEMOSHELL_VIEW_BOTTOM_AREA || area1 == NEMOSHELL_VIEW_LEFTBOTTOM_AREA || area1 == NEMOSHELL_VIEW_RIGHTBOTTOM_AREA)) {
		bin->resize_edges = WL_SHELL_SURFACE_RESIZE_BOTTOM;
	} else if ((area0 == NEMOSHELL_VIEW_BOTTOM_AREA || area0 == NEMOSHELL_VIEW_LEFTBOTTOM_AREA || area0 == NEMOSHELL_VIEW_RIGHTBOTTOM_AREA) &&
			(area1 == NEMOSHELL_VIEW_TOP_AREA || area1 == NEMOSHELL_VIEW_LEFTTOP_AREA || area1 == NEMOSHELL_VIEW_RIGHTTOP_AREA)) {
		bin->resize_edges = WL_SHELL_SURFACE_RESIZE_TOP;
	} else if ((area0 == NEMOSHELL_VIEW_LEFT_AREA || area0 == NEMOSHELL_VIEW_LEFTTOP_AREA || area0 == NEMOSHELL_VIEW_LEFTBOTTOM_AREA) &&
			(area1 == NEMOSHELL_VIEW_RIGHT_AREA || area1 == NEMOSHELL_VIEW_RIGHTTOP_AREA || area1 == NEMOSHELL_VIEW_RIGHTBOTTOM_AREA)) {
		bin->resize_edges = WL_SHELL_SURFACE_RESIZE_RIGHT;
	} else if ((area0 == NEMOSHELL_VIEW_RIGHT_AREA || area0 == NEMOSHELL_VIEW_RIGHTTOP_AREA || area0 == NEMOSHELL_VIEW_RIGHTBOTTOM_AREA) &&
			(area1 == NEMOSHELL_VIEW_LEFT_AREA || area1 == NEMOSHELL_VIEW_LEFTTOP_AREA || area1 == NEMOSHELL_VIEW_LEFTBOTTOM_AREA)) {
		bin->resize_edges = WL_SHELL_SURFACE_RESIZE_LEFT;
	}

	if (bin->resize_edges == 0) {
		type = (1 << NEMO_SURFACE_PICK_TYPE_ROTATE) | (1 << NEMO_SURFACE_PICK_TYPE_MOVE);
	} else {
		type = (1 << NEMO_SURFACE_PICK_TYPE_RESIZE);

		nemoshell_send_bin_configure(bin);
	}

	pick0->type = pick1->type = type;

	pick0->width = pick1->width = bin->view->content->width;
	pick0->height = pick1->height = bin->view->content->height;
	pick0->sx = pick1->sx = bin->view->geometry.sx;
	pick0->sy = pick1->sy = bin->view->geometry.sy;
	pick0->r = pick1->r = bin->view->geometry.r;
	pick0->has_reset = pick1->has_reset = bin->reset_scale;

	pick0->scale.distance = pick1->scale.distance = pickgrab_calculate_touchpoint_distance(tp0, tp1);
	pick0->resize.distance = pick1->resize.distance = pick0->scale.distance;

	pick0->rotate.r = pick1->rotate.r = pickgrab_calculate_touchpoint_angle(tp0, tp1);

	if (bin->view->geometry.has_pivot == 0) {
		float cx = (tp0->x + tp1->x) / 2.0f;
		float cy = (tp0->y + tp1->y) / 2.0f;
		float sx, sy;

		nemoview_transform_from_global(bin->view, cx, cy, &sx, &sy);

		nemoview_correct_pivot(bin->view, sx, sy);

		bin->px = cx;
		bin->py = cy;
		bin->ax = sx / bin->view->content->width;
		bin->ay = sy / bin->view->content->height;
	}

	pick0->dx = pick1->dx = bin->view->geometry.x - (tp0->x + tp1->x) / 2.0f;
	pick0->dy = pick1->dy = bin->view->geometry.y - (tp0->y + tp1->y) / 2.0f;

	pick0->tp0 = pick1->tp0 = tp0;
	pick0->tp1 = pick1->tp1 = tp1;

	pick0->other = pick1;
	pick1->other = pick0;

	nemoshell_start_touchpoint_shellgrab(&pick0->base, &pick_shellgrab_touchpoint_interface, bin, tp0);
	nemoshell_start_touchpoint_shellgrab(&pick1->base, &pick_shellgrab_touchpoint_interface, bin, tp1);

	return 0;
}

int nemoshell_pick_canvas_by_touchpoint(struct nemoshell *shell, struct touchpoint *tp0, struct touchpoint *tp1, uint32_t type, struct shellbin *bin)
{
	struct shellgrab_pick *pick0, *pick1;

	if (bin == NULL)
		return -1;

	if (bin->fixed > 0)
		return 0;

	if (bin->grabbed > 0)
		wl_signal_emit(&bin->ungrab_signal, bin);

	if (bin->state.fullscreen != 0 || bin->state.maximized != 0) {
		nemoshell_put_fullscreen_opaque(shell, bin);
		nemoshell_put_fullscreen_bin(shell, bin);
	}

	pick0 = (struct shellgrab_pick *)malloc(sizeof(struct shellgrab_pick));
	if (pick0 == NULL)
		return -1;
	memset(pick0, 0, sizeof(struct shellgrab_pick));

	pick1 = (struct shellgrab_pick *)malloc(sizeof(struct shellgrab_pick));
	if (pick1 == NULL)
		return -1;
	memset(pick1, 0, sizeof(struct shellgrab_pick));

	pick0->type = pick1->type = type;

	pick0->width = pick1->width = bin->view->content->width;
	pick0->height = pick1->height = bin->view->content->height;
	pick0->sx = pick1->sx = bin->view->geometry.sx;
	pick0->sy = pick1->sy = bin->view->geometry.sy;
	pick0->r = pick1->r = bin->view->geometry.r;
	pick0->has_reset = pick1->has_reset = bin->reset_scale;

	pick0->scale.distance = pick1->scale.distance = pickgrab_calculate_touchpoint_distance(tp0, tp1);
	pick0->resize.distance = pick1->resize.distance = pick0->scale.distance;

	pick0->rotate.r = pick1->rotate.r = pickgrab_calculate_touchpoint_angle(tp0, tp1);

	if (bin->view->geometry.has_pivot == 0) {
		float cx = (tp0->x + tp1->x) / 2.0f;
		float cy = (tp0->y + tp1->y) / 2.0f;
		float sx, sy;

		nemoview_transform_from_global(bin->view, cx, cy, &sx, &sy);

		nemoview_correct_pivot(bin->view, sx, sy);

		bin->px = cx;
		bin->py = cy;
		bin->ax = sx / bin->view->content->width;
		bin->ay = sy / bin->view->content->height;
	}

	pick0->dx = pick1->dx = bin->view->geometry.x - (tp0->x + tp1->x) / 2.0f;
	pick0->dy = pick1->dy = bin->view->geometry.y - (tp0->y + tp1->y) / 2.0f;

	pick0->tp0 = pick1->tp0 = tp0;
	pick0->tp1 = pick1->tp1 = tp1;

	pick0->other = pick1;
	pick1->other = pick0;

	if (shell->is_logging_grab != 0)
		nemolog_message("PICK", "[DOWN] %llu+%llu: x(%f) y(%f) sx(%f) sy(%f) r(%f) reset(%d)\n",
				tp0->gid, tp1->gid,
				bin->view->geometry.x, bin->view->geometry.y,
				bin->view->geometry.sx, bin->view->geometry.sy,
				bin->view->geometry.r,
				bin->reset_scale);

	nemoshell_start_touchpoint_shellgrab(&pick0->base, &pick_shellgrab_touchpoint_interface, bin, tp0);
	nemoshell_start_touchpoint_shellgrab(&pick1->base, &pick_shellgrab_touchpoint_interface, bin, tp1);

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
	struct touchpoint *tp = base->touchpoint;
	struct nemoactor *actor = grab->actor;
	struct nemoshell *shell = grab->shell;

	if (actor != NULL && (pick->type & (1 << NEMO_SURFACE_PICK_TYPE_SCALE))) {
		struct nemocompz *compz = actor->compz;
		struct nemoview *view = actor->view;
		int32_t width = pick->width * pick->sx;
		int32_t height = pick->height * pick->sy;
		int32_t sx, sy;
		float fromx, fromy, tox, toy;

		nemoview_set_scale(view, 1.0f, 1.0f);
		nemoview_update_transform(view);

		sx = (width - actor->base.width) * -actor->ax;
		sy = (height - actor->base.height) * -actor->ay;

		nemoview_transform_to_global(view, 0.0f, 0.0f, &fromx, &fromy);
		nemoview_transform_to_global(view, sx, sy, &tox, &toy);

		nemoview_set_position(view,
				view->geometry.x + tox - fromx,
				view->geometry.y + toy - fromy);

		if (nemoactor_dispatch_resize(actor, width, height, 0) == 0)
			nemoactor_dispatch_frame(actor);
	}

	nemoview_transform_done(actor->view);

	nemoshell_end_touchpoint_actorgrab(grab);
	nemoshell_end_touchpoint_actorgrab(&pick->other->base);
	free(pick->other);
	free(pick);

	if (tp->focus != NULL) {
		nemocontent_touch_up(tp, tp->focus->content, time, touchid);
	}
}

static void pick_actorgrab_touchpoint_motion(struct touchpoint_grab *base, uint32_t time, uint64_t touchid, float x, float y)
{
	struct touchpoint *tp = base->touchpoint;

	touchpoint_update_direction(tp, x, y);

	if (tp->focus != NULL) {
		float sx, sy;

		nemoview_transform_from_global(tp->focus, x, y, &sx, &sy);

		nemocontent_touch_motion(tp, tp->focus->content, time, touchid, sx, sy, x, y);
	}
}

static void pick_actorgrab_touchpoint_frame(struct touchpoint_grab *base, uint32_t frameid)
{
	struct actorgrab *grab = (struct actorgrab *)container_of(base, struct actorgrab, base.touchpoint);
	struct actorgrab_pick *pick = (struct actorgrab_pick *)container_of(grab, struct actorgrab_pick, base);
	struct touchpoint *tp = base->touchpoint;
	struct nemoactor *actor = grab->actor;
	struct nemoshell *shell = grab->shell;
	double distance = pickgrab_calculate_touchpoint_distance(pick->tp0, pick->tp1);
	float r = pickgrab_calculate_touchpoint_angle(pick->tp0, pick->tp1);
	float d = pickgrab_calculate_touchpoint_direction_degree(pick->tp0, pick->tp1);
	uint64_t touchid = tp->id;

	if (actor == NULL)
		return;

	if (pick->frameid == frameid)
		return;
	pick->frameid = pick->other->frameid = frameid;

	if (pick->type & (1 << NEMO_SURFACE_PICK_TYPE_ROTATE)) {
		if (distance > shell->pick.min_distance && d > shell->pick.rotate_degree) {
			if (fabsf(pick->rotate.r - r) < shell->pick.max_rotate) {
				pick->r = pick->other->r = pick->r + pick->rotate.r - r;

				nemoview_set_rotation(actor->view, pick->r);
			}
		}

		pick->rotate.r = pick->other->rotate.r = r;
	}

	if (pick->type & (1 << NEMO_SURFACE_PICK_TYPE_SCALE) || pick->type & (1 << NEMO_SURFACE_PICK_TYPE_SCALEONLY)) {
		if (distance > shell->pick.min_distance && d > shell->pick.scale_degree) {
			float s = distance / pick->scale.distance;

			if (fabsf(1.0f - s) < shell->pick.max_scale) {
				if (pick->sx * s * pick->width > actor->max_width ||
						pick->sy * s * pick->height > actor->max_height) {
					double sx = (double)actor->max_width / (double)pick->width;
					double sy = (double)actor->max_height / (double)pick->height;

					if (sx > sy) {
						pick->sx = pick->other->sx = sy;
						pick->sy = pick->other->sy = sy;
					} else {
						pick->sx = pick->other->sx = sx;
						pick->sy = pick->other->sy = sx;
					}
				} else if (pick->sx * s * pick->width < actor->min_width ||
						pick->sy * s * pick->height < actor->min_height) {
					double sx = (double)actor->min_width / (double)pick->width;
					double sy = (double)actor->min_height / (double)pick->height;

					if (sx > sy) {
						pick->sx = pick->other->sx = sx;
						pick->sy = pick->other->sy = sx;
					} else {
						pick->sx = pick->other->sx = sy;
						pick->sy = pick->other->sy = sy;
					}
				} else {
					pick->sx = pick->other->sx = pick->sx * s;
					pick->sy = pick->other->sy = pick->sy * s;
				}

				nemoview_set_scale(actor->view, pick->sx, pick->sy);
			}
		}

		pick->scale.distance = pick->other->scale.distance = distance;
	}

	if (pick->type & (1 << NEMO_SURFACE_PICK_TYPE_MOVE)) {
		float cx, cy;

		cx = pick->dx + (pick->tp0->x + pick->tp1->x) / 2.0f;
		cy = pick->dy + (pick->tp0->y + pick->tp1->y) / 2.0f;

		nemoview_set_position(actor->view, cx, cy);
	}

	nemoview_schedule_repaint(actor->view);
}

static void pick_actorgrab_touchpoint_cancel(struct touchpoint_grab *base)
{
	struct actorgrab *grab = (struct actorgrab *)container_of(base, struct actorgrab, base.touchpoint);
	struct actorgrab_pick *pick = (struct actorgrab_pick *)container_of(grab, struct actorgrab_pick, base);

	nemoshell_end_touchpoint_actorgrab(grab);
	nemoshell_end_touchpoint_actorgrab(&pick->other->base);
	free(pick->other);
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

	if (actor->grabbed > 0)
		wl_signal_emit(&actor->ungrab_signal, actor);

	pick0 = (struct actorgrab_pick *)malloc(sizeof(struct actorgrab_pick));
	if (pick0 == NULL)
		return -1;
	memset(pick0, 0, sizeof(struct actorgrab_pick));

	pick1 = (struct actorgrab_pick *)malloc(sizeof(struct actorgrab_pick));
	if (pick1 == NULL)
		return -1;
	memset(pick1, 0, sizeof(struct actorgrab_pick));

	pick0->base.shell = shell;
	pick1->base.shell = shell;

	pick0->type = pick1->type = type;

	pick0->width = pick1->width = actor->view->content->width;
	pick0->height = pick1->height = actor->view->content->height;
	pick0->sx = pick1->sx = actor->view->geometry.sx;
	pick0->sy = pick1->sy = actor->view->geometry.sy;
	pick0->r = pick1->r = actor->view->geometry.r;

	pick0->scale.distance = pick1->scale.distance = pickgrab_calculate_touchpoint_distance(tp0, tp1);

	pick0->rotate.r = pick1->rotate.r = pickgrab_calculate_touchpoint_angle(tp0, tp1);

	if (actor->view->geometry.has_pivot == 0) {
		float cx = (tp0->x + tp1->x) / 2.0f;
		float cy = (tp0->y + tp1->y) / 2.0f;
		float sx, sy;

		nemoview_transform_from_global(actor->view, cx, cy, &sx, &sy);

		nemoview_correct_pivot(actor->view, sx, sy);

		actor->px = cx;
		actor->py = cy;
		actor->ax = sx / actor->view->content->width;
		actor->ay = sy / actor->view->content->height;
	}

	pick0->dx = pick1->dx = actor->view->geometry.x - (tp0->x + tp1->x) / 2.0f;
	pick0->dy = pick1->dy = actor->view->geometry.y - (tp0->y + tp1->y) / 2.0f;

	pick0->tp0 = pick1->tp0 = tp0;
	pick0->tp1 = pick1->tp1 = tp1;

	pick0->other = pick1;
	pick1->other = pick0;

	nemoshell_start_touchpoint_actorgrab(&pick0->base, &pick_actorgrab_touchpoint_interface, actor, tp0);
	nemoshell_start_touchpoint_actorgrab(&pick1->base, &pick_actorgrab_touchpoint_interface, actor, tp1);

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
