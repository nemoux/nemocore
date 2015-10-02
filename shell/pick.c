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
#include <touch.h>
#include <view.h>
#include <canvas.h>
#include <actor.h>
#include <content.h>
#include <nemomisc.h>

static float pickgrab_calculate_touchpoint_distance(struct touchpoint *tp0, struct touchpoint *tp1)
{
	float dx, dy;

	dx = tp1->x - tp0->x;
	dy = tp1->y - tp0->y;

	return sqrtf(dx * dx + dy * dy);
}

static float pickgrab_calculate_touchpoint_angle(struct touchpoint *tp0, struct touchpoint *tp1)
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
	struct touchpoint *tp1 = pick->other->base.base.touchpoint.touchpoint;
	struct shellbin *bin = grab->bin;

	if (bin != NULL && (pick->type & (1 << NEMO_SURFACE_PICK_TYPE_SCALE))) {
		struct nemoshell *shell = bin->shell;
		struct nemocompz *compz = shell->compz;
		struct shellscreen *screen = NULL;
		double distance = pickgrab_calculate_touchpoint_distance(pick->tp0, pick->tp1);
		int32_t width, height;

		width = pick->width * (distance / pick->touch.distance);
		height = pick->height * (distance / pick->touch.distance);

		width = MAX(width, bin->min_width);
		height = MAX(height, bin->min_height);
		width = MIN(width, bin->max_width);
		height = MIN(height, bin->max_height);

		bin->reset_scale = 1;

		if (bin->on_pickscreen != 0) {
			if (nemocompz_get_scene_width(compz) <= width || nemocompz_get_scene_height(compz) <= height)
				screen = nemoshell_get_fullscreen_on(shell, tp0->x, tp0->y);

			if (screen != NULL) {
				nemoshell_clear_bin_next_state(bin);
				bin->next_state.fullscreen = 1;
				bin->state_changed = 1;

				bin->type = NEMO_SHELL_SURFACE_NORMAL_TYPE;
				nemoshell_set_parent_bin(bin, NULL);

				bin->screen.x = screen->dx;
				bin->screen.y = screen->dy;
				bin->screen.width = screen->dw;
				bin->screen.height = screen->dh;
				bin->has_screen = 1;

				nemoshell_send_bin_state(bin);
			} else {
				bin->client->send_configure(bin->canvas, width, height);
			}
		} else {
			bin->client->send_configure(bin->canvas, width, height);
		}
	}

	if (tp0->focus != NULL) {
		nemocontent_touch_up(tp0, tp0->focus->content, time, tp0->gid);
	}

	touchpoint_set_focus(tp0, NULL);

	touchpoint_update_grab(tp1);

	if (bin != NULL) {
		bin->resize_edges = 0;
		nemoshell_send_bin_configure(bin);
	}

	nemoshell_end_touchpoint_shellgrab(grab);
	nemoshell_end_touchpoint_shellgrab(&pick->other->base);
	free(pick->other);
	free(pick);
}

static void pick_shellgrab_touchpoint_motion(struct touchpoint_grab *base, uint32_t time, uint64_t touchid, float x, float y)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(base, struct shellgrab, base.touchpoint);
	struct shellgrab_pick *pick = (struct shellgrab_pick *)container_of(grab, struct shellgrab_pick, base);
	struct touchpoint *tp = base->touchpoint;
	struct shellbin *bin = grab->bin;

	touchpoint_move(tp, x, y);

	if (bin == NULL)
		return;

	if (tp->focus != NULL) {
		float sx, sy;

		nemoview_transform_from_global(tp->focus, x, y, &sx, &sy);

		nemocontent_touch_motion(tp, tp->focus->content, time, touchid, sx, sy, x, y);
	}

	if (pick->type & (1 << NEMO_SURFACE_PICK_TYPE_ROTATE)) {
		float r = pickgrab_calculate_touchpoint_angle(pick->tp0, pick->tp1);

		nemoview_set_rotation(bin->view, pick->r + pick->touch.r - r);
	}

	if (pick->type & (1 << NEMO_SURFACE_PICK_TYPE_SCALE)) {
		double distance = pickgrab_calculate_touchpoint_distance(pick->tp0, pick->tp1);

		if (pick->sx * (distance / pick->touch.distance) * pick->width > bin->max_width ||
				pick->sy * (distance / pick->touch.distance) * pick->height > bin->max_height) {
			nemoview_set_scale(bin->view,
					(double)bin->max_width / (double)pick->width,
					(double)bin->max_height / (double)pick->height);
		} else if (pick->sx * (distance / pick->touch.distance) * pick->width < bin->min_width ||
				pick->sy * (distance / pick->touch.distance) * pick->height < bin->min_height) {
			nemoview_set_scale(bin->view,
					(double)bin->min_width / (double)pick->width,
					(double)bin->min_height / (double)pick->height);
		} else {
			nemoview_set_scale(bin->view,
					pick->sx * (distance / pick->touch.distance),
					pick->sy * (distance / pick->touch.distance));
		}
	}

	if (pick->type & (1 << NEMO_SURFACE_PICK_TYPE_RESIZE)) {
		double distance = pickgrab_calculate_touchpoint_distance(pick->tp0, pick->tp1);
		int32_t width, height;

		if (bin->resize_edges & WL_SHELL_SURFACE_RESIZE_LEFT ||
				bin->resize_edges & WL_SHELL_SURFACE_RESIZE_RIGHT) {
			width = pick->width + (distance - pick->touch.distance);
		} else {
			width = pick->width;
		}

		if (bin->resize_edges & WL_SHELL_SURFACE_RESIZE_TOP ||
				bin->resize_edges & WL_SHELL_SURFACE_RESIZE_BOTTOM) {
			height = pick->height + (distance - pick->touch.distance);
		} else {
			height = pick->height;
		}

		width = MAX(width, bin->min_width);
		height = MAX(height, bin->min_height);
		width = MIN(width, bin->max_width);
		height = MIN(height, bin->max_height);

		bin->client->send_configure(bin->canvas, width, height);
	}

	if (pick->type & (1 << NEMO_SURFACE_PICK_TYPE_MOVE)) {
		float cx, cy;

		cx = pick->dx + (pick->tp0->x + pick->tp1->x) / 2.0f;
		cy = pick->dy + (pick->tp0->y + pick->tp1->y) / 2.0f;

		nemoview_set_position(bin->view, cx, cy);
	}

	nemoview_schedule_repaint(bin->view);
}

static void pick_shellgrab_touchpoint_frame(struct touchpoint_grab *base)
{
}

static void pick_shellgrab_touchpoint_cancel(struct touchpoint_grab *base)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(base, struct shellgrab, base.touchpoint);
	struct shellgrab_pick *pick = (struct shellgrab_pick *)container_of(grab, struct shellgrab_pick, base);

	if (grab->bin != NULL) {
		grab->bin->resize_edges = 0;
		nemoshell_send_bin_configure(grab->bin);
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

int nemoshell_pick_canvas_by_touchpoint_on_area(struct touchpoint *tp0, struct touchpoint *tp1, struct shellbin *bin)
{
	struct shellgrab_pick *pick0, *pick1;
	uint32_t type;
	int area0, area1;

	if (bin == NULL)
		return -1;

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

	area0 = nemoview_get_point_area(bin->view, tp0->x, tp0->y);
	area1 = nemoview_get_point_area(bin->view, tp1->x, tp1->y);

	if (area0 == NEMO_VIEW_CENTER_AREA || area1 == NEMO_VIEW_CENTER_AREA) {
		bin->resize_edges = 0;
	} else if (area0 == NEMO_VIEW_LEFTTOP_AREA && area1 == NEMO_VIEW_RIGHTBOTTOM_AREA) {
		bin->resize_edges = WL_SHELL_SURFACE_RESIZE_RIGHT | WL_SHELL_SURFACE_RESIZE_BOTTOM;
	} else if (area0 == NEMO_VIEW_RIGHTBOTTOM_AREA && area1 == NEMO_VIEW_LEFTTOP_AREA) {
		bin->resize_edges = WL_SHELL_SURFACE_RESIZE_LEFT | WL_SHELL_SURFACE_RESIZE_TOP;
	} else if (area0 == NEMO_VIEW_LEFTBOTTOM_AREA && area1 == NEMO_VIEW_RIGHTTOP_AREA) {
		bin->resize_edges = WL_SHELL_SURFACE_RESIZE_RIGHT | WL_SHELL_SURFACE_RESIZE_TOP;
	} else if (area0 == NEMO_VIEW_RIGHTTOP_AREA && area1 == NEMO_VIEW_LEFTBOTTOM_AREA) {
		bin->resize_edges = WL_SHELL_SURFACE_RESIZE_LEFT | WL_SHELL_SURFACE_RESIZE_BOTTOM;
	} else if ((area0 == NEMO_VIEW_TOP_AREA || area0 == NEMO_VIEW_LEFTTOP_AREA || area0 == NEMO_VIEW_RIGHTTOP_AREA) &&
			(area1 == NEMO_VIEW_BOTTOM_AREA || area1 == NEMO_VIEW_LEFTBOTTOM_AREA || area1 == NEMO_VIEW_RIGHTBOTTOM_AREA)) {
		bin->resize_edges = WL_SHELL_SURFACE_RESIZE_BOTTOM;
	} else if ((area0 == NEMO_VIEW_BOTTOM_AREA || area0 == NEMO_VIEW_LEFTBOTTOM_AREA || area0 == NEMO_VIEW_RIGHTBOTTOM_AREA) &&
			(area1 == NEMO_VIEW_TOP_AREA || area1 == NEMO_VIEW_LEFTTOP_AREA || area1 == NEMO_VIEW_RIGHTTOP_AREA)) {
		bin->resize_edges = WL_SHELL_SURFACE_RESIZE_TOP;
	} else if ((area0 == NEMO_VIEW_LEFT_AREA || area0 == NEMO_VIEW_LEFTTOP_AREA || area0 == NEMO_VIEW_LEFTBOTTOM_AREA) &&
			(area1 == NEMO_VIEW_RIGHT_AREA || area1 == NEMO_VIEW_RIGHTTOP_AREA || area1 == NEMO_VIEW_RIGHTBOTTOM_AREA)) {
		bin->resize_edges = WL_SHELL_SURFACE_RESIZE_RIGHT;
	} else if ((area0 == NEMO_VIEW_RIGHT_AREA || area0 == NEMO_VIEW_RIGHTTOP_AREA || area0 == NEMO_VIEW_RIGHTBOTTOM_AREA) &&
			(area1 == NEMO_VIEW_LEFT_AREA || area1 == NEMO_VIEW_LEFTTOP_AREA || area1 == NEMO_VIEW_LEFTBOTTOM_AREA)) {
		bin->resize_edges = WL_SHELL_SURFACE_RESIZE_LEFT;
	}

	if (bin->resize_edges == 0) {
		type = (1 << NEMO_SURFACE_PICK_TYPE_ROTATE) | (1 << NEMO_SURFACE_PICK_TYPE_MOVE);
	} else {
		type = (1 << NEMO_SURFACE_PICK_TYPE_RESIZE);

		nemoshell_send_bin_configure(bin);
	}

	pick0->type = pick1->type = type;

	pick0->width = pick1->width = bin->geometry.width;
	pick0->height = pick1->height = bin->geometry.height;
	pick0->sx = pick1->sx = bin->view->geometry.sx;
	pick0->sy = pick1->sy = bin->view->geometry.sy;
	pick0->r = pick1->r = bin->view->geometry.r;
	pick0->touch.distance = pick1->touch.distance = pickgrab_calculate_touchpoint_distance(tp0, tp1);
	pick0->touch.r = pick1->touch.r = pickgrab_calculate_touchpoint_angle(tp0, tp1);

	if (bin->view->geometry.has_pivot == 0)
		nemoview_correct_pivot(bin->view, bin->view->content->width * 0.5f, bin->view->content->height * 0.5f);

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

int nemoshell_pick_canvas_by_touchpoint(struct touchpoint *tp0, struct touchpoint *tp1, uint32_t type, struct shellbin *bin)
{
	struct shellgrab_pick *pick0, *pick1;

	if (bin == NULL)
		return -1;

	if (bin->grabbed > 0)
		wl_signal_emit(&bin->ungrab_signal, bin);

	if (bin->state.fullscreen != 0 || bin->state.maximized != 0) {
		bin->state_changed = 1;
		bin->next_state.fullscreen = 0;

		nemoshell_send_bin_state(bin);
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

	pick0->width = pick1->width = bin->geometry.width;
	pick0->height = pick1->height = bin->geometry.height;
	pick0->sx = pick1->sx = bin->view->geometry.sx;
	pick0->sy = pick1->sy = bin->view->geometry.sy;
	pick0->r = pick1->r = bin->view->geometry.r;
	pick0->touch.distance = pick1->touch.distance = pickgrab_calculate_touchpoint_distance(tp0, tp1);
	pick0->touch.r = pick1->touch.r = pickgrab_calculate_touchpoint_angle(tp0, tp1);

	if (bin->view->geometry.has_pivot == 0)
		nemoview_correct_pivot(bin->view, bin->view->content->width * 0.5f, bin->view->content->height * 0.5f);

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

int nemoshell_pick_canvas(struct nemoshell *shell, struct shellbin *bin, uint32_t serial0, uint32_t serial1, uint32_t type)
{
	struct touchpoint *tp0, *tp1;

	tp0 = nemoseat_get_touchpoint_by_grab_serial(shell->compz->seat, serial0);
	if (tp0 == NULL)
		return 0;
	tp1 = nemoseat_get_touchpoint_by_grab_serial(shell->compz->seat, serial1);
	if (tp1 == NULL)
		return 0;

	return nemoshell_pick_canvas_by_touchpoint(tp0, tp1, type, bin);
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

	if (actor != NULL && (pick->type & (1 << NEMO_SURFACE_PICK_TYPE_SCALE))) {
		struct nemocompz *compz = actor->compz;
		struct nemoview *view = actor->view;
		double distance = pickgrab_calculate_touchpoint_distance(pick->tp0, pick->tp1);
		int32_t width, height;
		int32_t sx, sy;
		float fromx, fromy, tox, toy;

		width = pick->width * (distance / pick->touch.distance);
		height = pick->height * (distance / pick->touch.distance);

		width = MAX(width, actor->min_width);
		height = MAX(height, actor->min_height);
		width = MIN(width, actor->max_width);
		height = MIN(height, actor->max_height);

		nemoview_set_scale(view, 1.0f, 1.0f);
		nemoview_update_transform(view);

		sx = (width - actor->base.width) * -0.5f;
		sy = (height - actor->base.height) * -0.5f;

		nemoview_transform_to_global(view, 0.0f, 0.0f, &fromx, &fromy);
		nemoview_transform_to_global(view, sx, sy, &tox, &toy);

		nemoview_set_position(view,
				view->geometry.x + tox - fromx,
				view->geometry.y + toy - fromy);

		nemoactor_dispatch_resize(actor, width, height);
		nemoactor_dispatch_frame(actor);
	}

	if (tp->focus != NULL) {
		nemocontent_touch_up(tp, tp->focus->content, time, touchid);
	}

	nemoshell_end_touchpoint_actorgrab(grab);
	nemoshell_end_touchpoint_actorgrab(&pick->other->base);
	free(pick->other);
	free(pick);
}

static void pick_actorgrab_touchpoint_motion(struct touchpoint_grab *base, uint32_t time, uint64_t touchid, float x, float y)
{
	struct actorgrab *grab = (struct actorgrab *)container_of(base, struct actorgrab, base.touchpoint);
	struct actorgrab_pick *pick = (struct actorgrab_pick *)container_of(grab, struct actorgrab_pick, base);
	struct touchpoint *tp = base->touchpoint;
	struct nemoactor *actor = grab->actor;

	touchpoint_move(tp, x, y);

	if (grab->actor == NULL)
		return;

	if (tp->focus != NULL) {
		float sx, sy;

		nemoview_transform_from_global(tp->focus, x, y, &sx, &sy);

		nemocontent_touch_motion(tp, tp->focus->content, time, touchid, sx, sy, x, y);
	}

	if (pick->type & (1 << NEMO_SURFACE_PICK_TYPE_ROTATE)) {
		float r = pickgrab_calculate_touchpoint_angle(pick->tp0, pick->tp1);

		nemoview_set_rotation(actor->view, pick->r + pick->touch.r - r);
	}

	if (pick->type & (1 << NEMO_SURFACE_PICK_TYPE_SCALE)) {
		double distance = pickgrab_calculate_touchpoint_distance(pick->tp0, pick->tp1);

		if (pick->sx * (distance / pick->touch.distance) * pick->width > actor->max_width ||
				pick->sy * (distance / pick->touch.distance) * pick->height > actor->max_height) {
			nemoview_set_scale(actor->view,
					(double)actor->max_width / (double)pick->width,
					(double)actor->max_height / (double)pick->height);
		} else if (pick->sx * (distance / pick->touch.distance) * pick->width < actor->min_width ||
				pick->sy * (distance / pick->touch.distance) * pick->height < actor->min_height) {
			nemoview_set_scale(actor->view,
					(double)actor->min_width / (double)pick->width,
					(double)actor->min_height / (double)pick->height);
		} else {
			nemoview_set_scale(actor->view,
					pick->sx * (distance / pick->touch.distance),
					pick->sy * (distance / pick->touch.distance));
		}
	}

	if (pick->type & (1 << NEMO_SURFACE_PICK_TYPE_RESIZE)) {
		double distance = pickgrab_calculate_touchpoint_distance(pick->tp0, pick->tp1);

		nemoactor_dispatch_resize(actor,
				MAX(MIN(pick->width * (distance / pick->touch.distance), actor->max_width), actor->min_width),
				MAX(MIN(pick->height * (distance / pick->touch.distance), actor->max_height), actor->min_height));
	}

	if (pick->type & (1 << NEMO_SURFACE_PICK_TYPE_MOVE)) {
		float cx, cy;

		cx = pick->dx + (pick->tp0->x + pick->tp1->x) / 2.0f;
		cy = pick->dy + (pick->tp0->y + pick->tp1->y) / 2.0f;

		nemoview_set_position(actor->view, cx, cy);
	}

	nemoview_schedule_repaint(actor->view);
}

static void pick_actorgrab_touchpoint_frame(struct touchpoint_grab *base)
{
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

int nemoshell_pick_actor_by_touchpoint(struct touchpoint *tp0, struct touchpoint *tp1, uint32_t type, struct nemoactor *actor)
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

	pick0->type = pick1->type = type;

	pick0->width = pick1->width = actor->view->content->width;
	pick0->height = pick1->height = actor->view->content->height;
	pick0->sx = pick1->sx = actor->view->geometry.sx;
	pick0->sy = pick1->sy = actor->view->geometry.sy;
	pick0->r = pick1->r = actor->view->geometry.r;
	pick0->touch.distance = pick1->touch.distance = pickgrab_calculate_touchpoint_distance(tp0, tp1);
	pick0->touch.r = pick1->touch.r = pickgrab_calculate_touchpoint_angle(tp0, tp1);

	if (actor->view->geometry.has_pivot == 0)
		nemoview_correct_pivot(actor->view, actor->view->content->width * 0.5f, actor->view->content->height * 0.5f);

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

	return nemoshell_pick_actor_by_touchpoint(tp0, tp1, type, actor);
}
