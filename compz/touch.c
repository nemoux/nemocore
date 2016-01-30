#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>
#include <wayland-nemo-seat-server-protocol.h>

#include <touch.h>
#include <tuionode.h>
#include <compz.h>
#include <seat.h>
#include <canvas.h>
#include <view.h>
#include <screen.h>
#include <picker.h>
#include <virtuio.h>
#include <binding.h>
#include <nemomisc.h>

static void default_touchpoint_grab_down(struct touchpoint_grab *grab, uint32_t time, uint64_t touchid, float x, float y)
{
	struct touchpoint *tp = grab->touchpoint;
	struct nemoview *view;
	float sx, sy;

	view = nemocompz_pick_view(tp->touch->seat->compz, x, y, &sx, &sy);
	if (view != NULL) {
		touchpoint_set_focus(tp, view);
	}

	touchpoint_update(tp, x, y);

	if (tp->focus != NULL) {
		nemocontent_touch_down(tp, tp->focus->content, time, touchid, sx, sy, x, y);
	}

	virtuio_dispatch_events(tp->touch->seat->compz);
}

static void default_touchpoint_grab_up(struct touchpoint_grab *grab, uint32_t time, uint64_t touchid)
{
	struct touchpoint *tp = grab->touchpoint;

	if (tp->focus != NULL) {
		nemocontent_touch_up(tp, tp->focus->content, time, touchid);
	}
}

static void default_touchpoint_grab_motion(struct touchpoint_grab *grab, uint32_t time, uint64_t touchid, float x, float y)
{
	struct touchpoint *tp = grab->touchpoint;

	touchpoint_update(tp, x, y);

	if (tp->focus != NULL) {
		float sx, sy;

		nemoview_transform_from_global(tp->focus, x, y, &sx, &sy);

		nemocontent_touch_motion(tp, tp->focus->content, time, touchid, sx, sy, x, y);
	}
}

static void default_touchpoint_grab_frame(struct touchpoint_grab *grab, uint32_t frameid)
{
	struct touchpoint *tp = grab->touchpoint;

	if (tp->focus != NULL) {
		nemocontent_touch_frame(tp, tp->focus->content);
	}
}

static void default_touchpoint_grab_cancel(struct touchpoint_grab *grab)
{
}

static const struct touchpoint_grab_interface default_touchpoint_grab_interface = {
	default_touchpoint_grab_down,
	default_touchpoint_grab_up,
	default_touchpoint_grab_motion,
	default_touchpoint_grab_frame,
	default_touchpoint_grab_cancel
};

static void wayland_touch_release(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static const struct wl_touch_interface wayland_touch_implementation = {
	wayland_touch_release
};

static void nemotouch_unbind_wayland(struct wl_resource *resource)
{
	wl_list_remove(wl_resource_get_link(resource));
}

int nemotouch_bind_wayland(struct wl_client *client, struct wl_resource *seat_resource, uint32_t id)
{
	struct nemoseat *seat = (struct nemoseat *)wl_resource_get_user_data(seat_resource);
	struct wl_resource *resource;

	resource = wl_resource_create(client, &wl_touch_interface, wl_resource_get_version(seat_resource), id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return -1;
	}

	wl_resource_set_implementation(resource, &wayland_touch_implementation, seat, nemotouch_unbind_wayland);

	wl_list_insert(&seat->touch.resource_list, wl_resource_get_link(resource));

	return 0;
}

static void nemo_touch_release(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void nemo_touch_bypass(struct wl_client *client, struct wl_resource *resource, int32_t touchid, wl_fixed_t sx, wl_fixed_t sy)
{
	struct nemoseat *seat = (struct nemoseat *)wl_resource_get_user_data(resource);
	struct nemoview *view;
	struct touchpoint *tp;
	uint32_t time;
	float x, y;
	float tx, ty;

	tp = nemoseat_get_touchpoint_by_id(seat, touchid);
	if (tp == NULL || tp->focus == NULL)
		return;

	nemoview_transform_to_global(tp->focus, wl_fixed_to_double(sx), wl_fixed_to_double(sy), &x, &y);

	view = nemocompz_pick_view_below(seat->compz, x, y, &tx, &ty, tp->focus);
	if (view != NULL) {
		time = time_current_msecs();

		nemocontent_touch_up(tp, tp->focus->content, time, tp->gid);

		touchpoint_set_focus(tp, view);

		nemocontent_touch_down(tp, tp->focus->content, time, touchid, tx, ty, x, y);

		tp->grab_serial = wl_display_get_serial(seat->compz->display);
		tp->grab_time = time;

		nemocompz_run_touch_binding(seat->compz, tp, time);
	}
}

static void nemo_touch_calibrate(struct wl_client *client, struct wl_resource *resource,
		const char *name,
		wl_fixed_t x0, wl_fixed_t y0,
		wl_fixed_t x1, wl_fixed_t y1,
		wl_fixed_t x2, wl_fixed_t y2,
		wl_fixed_t x3, wl_fixed_t y3)
{
	struct nemoseat *seat = (struct nemoseat *)wl_resource_get_user_data(resource);
	struct touchnode *node;

	node = nemotouch_get_node_by_name(seat->compz, name);
	if (node != NULL && node->calibrate != NULL) {
		node->calibrate(node,
				wl_fixed_to_double(x0), wl_fixed_to_double(y0),
				wl_fixed_to_double(x1), wl_fixed_to_double(y1),
				wl_fixed_to_double(x2), wl_fixed_to_double(y2),
				wl_fixed_to_double(x3), wl_fixed_to_double(y3));
	}
}

static const struct nemo_touch_interface nemo_touch_implementation = {
	nemo_touch_release,
	nemo_touch_bypass,
	nemo_touch_calibrate
};

static void nemotouch_unbind_nemo(struct wl_resource *resource)
{
	wl_list_remove(wl_resource_get_link(resource));
}

int nemotouch_bind_nemo(struct wl_client *client, struct wl_resource *seat_resource, uint32_t id)
{
	struct nemoseat *seat = (struct nemoseat *)wl_resource_get_user_data(seat_resource);
	struct wl_resource *resource;

	resource = wl_resource_create(client, &nemo_touch_interface, wl_resource_get_version(seat_resource), id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return -1;
	}

	wl_resource_set_implementation(resource, &nemo_touch_implementation, seat, nemotouch_unbind_nemo);

	wl_list_insert(&seat->touch.nemo_resource_list, wl_resource_get_link(resource));

	return 0;
}

struct nemotouch *nemotouch_create(struct nemoseat *seat, struct inputnode *node)
{
	struct nemocompz *compz = seat->compz;
	struct nemotouch *touch;
	char *env;

	touch = (struct nemotouch *)malloc(sizeof(struct nemotouch));
	if (touch == NULL)
		return NULL;
	memset(touch, 0, sizeof(struct nemotouch));

	touch->seat = seat;
	touch->node = node;

	wl_list_init(&touch->touchpoint_list);

	wl_list_insert(&seat->touch.device_list, &touch->link);

	env = getenv("NEMOUX_TOUCH_LOG");
	if (env != NULL && strcmp(env, "ON") == 0)
		touch->is_logging = 1;

	return touch;

err1:
	free(touch);

	return NULL;
}

void nemotouch_destroy(struct nemotouch *touch)
{
	wl_list_remove(&touch->link);

	free(touch);
}

float touchpoint_get_moving_distance(struct touchpoint *tp, float x, float y)
{
	float dx = x - tp->x;
	float dy = y - tp->y;

	return sqrtf(dx * dx + dy * dy);
}

void touchpoint_update(struct touchpoint *tp, float x, float y)
{
	tp->x = x;
	tp->y = y;
}

void touchpoint_update_direction(struct touchpoint *tp, float x, float y)
{
	if (tp->x != x || tp->y != y) {
		tp->dx = x - tp->x;
		tp->dy = y - tp->y;

		tp->x = x;
		tp->y = y;
	}
}

int touchpoint_update_velocity(struct touchpoint *tp, uint32_t nsamples)
{
	float x0, y0;
	float x1, y1;
	uint32_t t0, t1;
	uint32_t i0, i1;

	if (tp->nsamples < nsamples)
		return 0;

	i0 = (tp->esample - nsamples) % NEMOCOMPZ_TOUCH_SAMPLE_MAX;
	i1 = (tp->esample - 1) % NEMOCOMPZ_TOUCH_SAMPLE_MAX;

	x0 = tp->samples[i0].x;
	y0 = tp->samples[i0].y;
	t0 = tp->samples[i0].time;

	x1 = tp->samples[i1].x;
	y1 = tp->samples[i1].y;
	t1 = tp->samples[i1].time;

	tp->dx = (x1 - x0) / (float)(t1 - t0);
	tp->dy = (y1 - y0) / (float)(t1 - t0);

	return 1;
}

static void touchpoint_handle_focus_resource_destroy(struct wl_listener *listener, void *data)
{
	struct touchpoint *tp = (struct touchpoint *)container_of(listener, struct touchpoint, focus_resource_listener);

	tp->focus = NULL;

	touchpoint_set_focus(tp, NULL);
}

static void touchpoint_handle_focus_view_destroy(struct wl_listener *listener, void *data)
{
	struct touchpoint *tp = (struct touchpoint *)container_of(listener, struct touchpoint, focus_view_listener);

	tp->focus = NULL;

	touchpoint_set_focus(tp, NULL);
}

void touchpoint_set_focus(struct touchpoint *tp, struct nemoview *view)
{
	wl_list_remove(&tp->focus_view_listener.link);
	wl_list_init(&tp->focus_view_listener.link);
	wl_list_remove(&tp->focus_resource_listener.link);
	wl_list_init(&tp->focus_resource_listener.link);

	if (view != NULL)
		wl_signal_add(&view->destroy_signal, &tp->focus_view_listener);
	if (view != NULL && view->canvas != NULL && view->canvas->resource != NULL)
		wl_resource_add_destroy_listener(view->canvas->resource, &tp->focus_resource_listener);

	tp->focus = view;

	wl_signal_emit(&tp->touch->seat->touch.focus_signal, tp);
}

static struct touchpoint *nemotouch_create_touchpoint(struct nemotouch *touch, uint64_t id)
{
	struct touchpoint *tp;

	tp = (struct touchpoint *)malloc(sizeof(struct touchpoint));
	if (tp == NULL)
		return NULL;
	memset(tp, 0, sizeof(struct touchpoint));

	tp->id = id;
	tp->gid = ++touch->seat->compz->touch_ids;
	tp->touch = touch;

	wl_signal_init(&tp->destroy_signal);

	tp->default_grab.interface = &default_touchpoint_grab_interface;
	tp->default_grab.touchpoint = tp;
	tp->grab = &tp->default_grab;

	wl_list_init(&tp->focus_resource_listener.link);
	tp->focus_resource_listener.notify = touchpoint_handle_focus_resource_destroy;
	wl_list_init(&tp->focus_view_listener.link);
	tp->focus_view_listener.notify = touchpoint_handle_focus_view_destroy;

	wl_list_insert(&touch->touchpoint_list, &tp->link);

	return tp;
}

static void nemotouch_destroy_touchpoint(struct nemotouch *touch, struct touchpoint *tp)
{
	wl_signal_emit(&tp->destroy_signal, tp);

	wl_list_remove(&tp->focus_resource_listener.link);
	wl_list_remove(&tp->focus_view_listener.link);

	wl_list_remove(&tp->link);

	free(tp);
}

struct touchpoint *nemotouch_get_touchpoint_by_id(struct nemotouch *touch, uint64_t id)
{
	struct touchpoint *tp;

	wl_list_for_each(tp, &touch->touchpoint_list, link) {
		if (tp->id == id)
			return tp;
	}

	return NULL;
}

struct touchpoint *nemotouch_get_touchpoint_list_by_id(struct nemotouch *touch, struct wl_list *list, uint64_t id)
{
	struct touchpoint *tp;

	wl_list_for_each(tp, &touch->touchpoint_list, link) {
		if (tp->id == id)
			return tp;
	}

	wl_list_for_each(tp, list, link) {
		if (tp->id == id)
			return tp;
	}

	return NULL;
}

struct touchpoint *nemotouch_get_touchpoint_by_serial(struct nemotouch *touch, uint32_t serial)
{
	struct touchpoint *tp;

	wl_list_for_each(tp, &touch->touchpoint_list, link) {
		if (tp->grab_serial == serial)
			return tp;
	}

	return NULL;
}

void nemotouch_notify_down(struct nemotouch *touch, uint32_t time, int id, float x, float y)
{
	struct touchpoint *tp;

	if (touch == NULL)
		return;

	if (touch->is_logging != 0)
		nemolog_message("TOUCH", "[DOWN] %d: %f %f (%u)\n", id, x, y, time);

	tp = nemotouch_create_touchpoint(touch, id);
	if (tp == NULL)
		return;

	tp->grab_x = x;
	tp->grab_y = y;

	tp->grab->interface->down(tp->grab, time, tp->gid, x, y);

	tp->grab_serial = wl_display_get_serial(touch->seat->compz->display);
	tp->grab_time = time;

	nemocompz_run_touch_binding(touch->seat->compz, tp, time);
}

void nemotouch_notify_up(struct nemotouch *touch, uint32_t time, int id)
{
	struct touchpoint *tp;

	if (touch == NULL)
		return;

	if (touch->is_logging != 0)
		nemolog_message("TOUCH", "[UP] %d: (%u)\n", id, time);

	tp = nemotouch_get_touchpoint_by_id(touch, id);
	if (tp == NULL)
		return;

	tp->grab->interface->up(tp->grab, time, tp->gid);

	nemotouch_destroy_touchpoint(touch, tp);
}

void nemotouch_notify_motion(struct nemotouch *touch, uint32_t time, int id, float x, float y)
{
	struct touchpoint *tp;

	if (touch == NULL)
		return;

	if (touch->is_logging != 0)
		nemolog_message("TOUCH", "[MOTION] %d: %f %f (%u)\n", id, x, y, time);

	tp = nemotouch_get_touchpoint_by_id(touch, id);
	if (tp == NULL)
		return;

	tp->grab->interface->motion(tp->grab, time, tp->gid, x, y);
}

void nemotouch_notify_frame(struct nemotouch *touch, int id)
{
	struct touchpoint *tp;

	if (touch == NULL)
		return;

	if (touch->is_logging != 0)
		nemolog_message("TOUCH", "[FRAME] %d:\n", id);

	tp = nemotouch_get_touchpoint_by_id(touch, id);
	if (tp == NULL)
		return;

	tp->grab->interface->frame(tp->grab, touch->frame_count);
}

void nemotouch_notify_frames(struct nemotouch *touch)
{
	struct touchpoint *tp;

	if (touch->is_logging != 0)
		nemolog_message("TOUCH", "[FRAME]\n");

	touch->frame_count++;

	wl_list_for_each(tp, &touch->touchpoint_list, link) {
		tp->grab->interface->frame(tp->grab, touch->frame_count);
	}
}

void nemotouch_flush_tuio(struct tuionode *node)
{
	struct nemotouch *touch = node->touch;
	struct touchpoint *tp, *tnext;
	struct wl_list touchpoint_list;
	uint32_t msecs = time_current_msecs();
	float x, y;
	int i;

	wl_list_init(&touchpoint_list);
	wl_list_insert_list(&touchpoint_list, &touch->touchpoint_list);
	wl_list_init(&touch->touchpoint_list);

	for (i = 0; i < node->alive.index; i++) {
		if (node->base.screen != NULL) {
			nemoscreen_transform_to_global(node->base.screen,
					node->taps[i].f[0] * node->base.screen->width,
					node->taps[i].f[1] * node->base.screen->height,
					&x, &y);
		} else {
			nemoinput_transform_to_global(&node->base,
					node->taps[i].f[0] * node->base.width,
					node->taps[i].f[1] * node->base.height,
					&x, &y);
		}

		tp = nemotouch_get_touchpoint_list_by_id(touch, &touchpoint_list, node->taps[i].id);
		if (tp == NULL) {
			tp = nemotouch_create_touchpoint(touch, node->taps[i].id);
			if (tp == NULL)
				return;

			tp->grab_x = x;
			tp->grab_y = y;

			tp->grab->interface->down(tp->grab, msecs, tp->gid, x, y);

			tp->grab_serial = wl_display_get_serial(node->compz->display);
			tp->grab_time = msecs;

			nemocompz_run_touch_binding(touch->seat->compz, tp, msecs);
		} else {
			tp->grab->interface->motion(tp->grab, msecs, tp->gid, x, y);

			wl_list_remove(&tp->link);
			wl_list_insert(&touch->touchpoint_list, &tp->link);
		}
	}

	wl_list_for_each_safe(tp, tnext, &touchpoint_list, link) {
		tp->grab->interface->up(tp->grab, msecs, tp->gid);

		nemotouch_destroy_touchpoint(touch, tp);
	}

	touch->frame_count++;

	wl_list_for_each(tp, &touch->touchpoint_list, link) {
		tp->grab->interface->frame(tp->grab, touch->frame_count);
	}
}

void touchpoint_start_grab(struct touchpoint *tp, struct touchpoint_grab *grab)
{
	if (tp->grab != &tp->default_grab)
		tp->grab->interface->cancel(tp->grab);

	tp->grab = grab;
	grab->touchpoint = tp;
}

void touchpoint_end_grab(struct touchpoint *tp)
{
	tp->grab = &tp->default_grab;
}

void touchpoint_cancel_grab(struct touchpoint *tp)
{
	tp->grab->interface->cancel(tp->grab);
}

void touchpoint_update_grab(struct touchpoint *tp)
{
	tp->grab_x = tp->x;
	tp->grab_y = tp->y;
}

struct touchnode *nemotouch_create_node(struct nemocompz *compz, const char *devnode)
{
	struct touchnode *node;
	uint32_t nodeid, screenid;

	node = (struct touchnode *)malloc(sizeof(struct touchnode));
	if (node == NULL)
		return NULL;
	memset(node, 0, sizeof(struct touchnode));

	node->compz = compz;

	node->touch = nemotouch_create(compz->seat, &node->base);
	if (node->touch == NULL)
		goto err1;

	node->base.devnode = strdup(devnode);

	if (nemoinput_get_config_screen(compz, node->base.devnode, &nodeid, &screenid) > 0)
		nemoinput_set_screen(&node->base, nemocompz_get_screen(compz, nodeid, screenid));
	else if (nemoinput_get_config_geometry(compz, node->base.devnode, &node->base) <= 0)
		nemoinput_set_geometry(&node->base,
				0, 0,
				nemocompz_get_scene_width(compz),
				nemocompz_get_scene_height(compz));

	wl_list_insert(compz->touch_list.prev, &node->link);

	return node;

err1:
	free(node);

	return NULL;
}

void nemotouch_destroy_node(struct touchnode *node)
{
	wl_list_remove(&node->link);

	if (node->base.screen != NULL)
		nemoinput_put_screen(&node->base);

	if (node->touch != NULL)
		nemotouch_destroy(node->touch);

	if (node->base.devnode != NULL)
		free(node->base.devnode);

	free(node);
}

struct touchnode *nemotouch_get_node_by_name(struct nemocompz *compz, const char *name)
{
	struct touchnode *node;

	wl_list_for_each(node, &compz->touch_list, link) {
		if (strcmp(node->base.devnode, name) == 0)
			return node;
	}

	return NULL;
}

struct touchtaps *nemotouch_create_taps(int max)
{
	struct touchtaps *taps;

	taps = (struct touchtaps *)malloc(sizeof(struct touchtaps));
	if (taps == NULL)
		return NULL;

	taps->ids = (uint64_t *)malloc(sizeof(uint64_t) * max);
	if (taps->ids == NULL)
		goto err1;

	taps->points = (double *)malloc(sizeof(double[2]) * max);
	if (taps->points == NULL)
		goto err2;

	taps->ntaps = 0;
	taps->staps = max;

	return taps;

err2:
	free(taps->ids);

err1:
	free(taps);

	return NULL;
}

void nemotouch_destroy_taps(struct touchtaps *taps)
{
	free(taps->ids);
	free(taps->points);
	free(taps);
}

void nemotouch_attach_tap(struct touchtaps *taps, uint64_t id, double x, double y)
{
	taps->ids[taps->ntaps] = id;
	taps->points[taps->ntaps * 2 + 0] = x;
	taps->points[taps->ntaps * 2 + 1] = y;

	taps->ntaps++;
}

void nemotouch_flush_taps(struct touchnode *node, struct touchtaps *taps)
{
	struct nemotouch *touch = node->touch;
	struct touchpoint *tp, *tnext;
	struct wl_list touchpoint_list;
	uint32_t msecs = time_current_msecs();
	float x, y;
	int i;

	wl_list_init(&touchpoint_list);
	wl_list_insert_list(&touchpoint_list, &touch->touchpoint_list);
	wl_list_init(&touch->touchpoint_list);

	for (i = 0; i < taps->ntaps; i++) {
		if (node->base.screen != NULL) {
			nemoscreen_transform_to_global(node->base.screen,
					taps->points[i * 2 + 0] * node->base.screen->width,
					taps->points[i * 2 + 1] * node->base.screen->height,
					&x, &y);
		} else {
			nemoinput_transform_to_global(&node->base,
					taps->points[i * 2 + 0] * node->base.width,
					taps->points[i * 2 + 1] * node->base.height,
					&x, &y);
		}

		tp = nemotouch_get_touchpoint_list_by_id(touch, &touchpoint_list, taps->ids[i]);
		if (tp == NULL) {
			tp = nemotouch_create_touchpoint(touch, taps->ids[i]);
			if (tp == NULL)
				return;

			tp->grab_x = x;
			tp->grab_y = y;

			tp->grab->interface->down(tp->grab, msecs, tp->gid, x, y);

			tp->grab_serial = wl_display_get_serial(node->compz->display);
			tp->grab_time = msecs;

			nemocompz_run_touch_binding(touch->seat->compz, tp, msecs);
		} else {
			tp->grab->interface->motion(tp->grab, msecs, tp->gid, x, y);

			wl_list_remove(&tp->link);
			wl_list_insert(&touch->touchpoint_list, &tp->link);
		}
	}

	wl_list_for_each_safe(tp, tnext, &touchpoint_list, link) {
		tp->grab->interface->up(tp->grab, msecs, tp->gid);

		nemotouch_destroy_touchpoint(touch, tp);
	}

	touch->frame_count++;

	wl_list_for_each(tp, &touch->touchpoint_list, link) {
		tp->grab->interface->frame(tp->grab, touch->frame_count);
	}
}

void nemotouch_bypass_event(struct nemocompz *compz, int32_t touchid, float sx, float sy)
{
	struct nemoseat *seat = compz->seat;
	struct nemoview *view;
	struct touchpoint *tp;
	uint32_t time;
	float x, y;
	float tx, ty;

	tp = nemoseat_get_touchpoint_by_id(seat, touchid);
	if (tp == NULL || tp->focus == NULL)
		return;

	nemoview_transform_to_global(tp->focus, sx, sy, &x, &y);

	view = nemocompz_pick_view_below(compz, x, y, &tx, &ty, tp->focus);
	if (view != NULL) {
		time = time_current_msecs();

		nemocontent_touch_up(tp, tp->focus->content, time, tp->gid);

		touchpoint_set_focus(tp, view);

		nemocontent_touch_down(tp, tp->focus->content, time, touchid, tx, ty, x, y);

		tp->grab_serial = wl_display_get_serial(compz->display);
		tp->grab_time = time;

		nemocompz_run_touch_binding(compz, tp, time);
	}
}

void nemotouch_dump_touchpoint(struct nemotouch *touch)
{
	struct touchpoint *tp;

	nemolog_message("TOUCH", "dump '%s' touch's taps...\n", touch->node->devnode);

	wl_list_for_each(tp, &touch->touchpoint_list, link) {
		nemolog_message("TOUCH", "[%llu:%llu] %f %f\n", tp->id, tp->gid, tp->x, tp->y);
	}
}
