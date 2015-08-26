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

	touchpoint_move(tp, x, y);

	if (tp->focus != NULL) {
		nemocontent_touch_down(tp, tp->focus->content, time, touchid, sx, sy);
	}
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

	touchpoint_move(tp, x, y);

	if (tp->focus != NULL) {
		float sx, sy;

		nemoview_transform_from_global(tp->focus, x, y, &sx, &sy);

		nemocontent_touch_motion(tp, tp->focus->content, time, touchid, sx, sy);
	}
}

static void default_touchpoint_grab_frame(struct touchpoint_grab *grab)
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

static const struct nemo_touch_interface nemo_touch_implementation = {
	nemo_touch_release
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

static void nemotouch_destroy_touchpoint(struct nemotouch *touch, struct touchpoint *tp);

static int nemotouch_dispatch_timeout(void *data)
{
	struct nemotouch *touch = (struct nemotouch *)data;
	struct nemocompz *compz = touch->seat->compz;
	struct touchpoint *tp, *tnext;
	uint32_t msecs;

	msecs = time_current_msecs();

	wl_list_for_each_safe(tp, tnext, &touch->touchpoint_list, link) {
		if (tp->grab_time + compz->touch_timeout < msecs) {
			tp->grab->interface->up(tp->grab, msecs, tp->gid);

			nemotouch_destroy_touchpoint(touch, tp);
		}
	}

	wl_event_source_timer_update(touch->timeout, compz->touch_timeout);

	return 1;
}

struct nemotouch *nemotouch_create(struct nemoseat *seat, struct inputnode *node)
{
	struct nemocompz *compz = seat->compz;
	struct nemotouch *touch;

	touch = (struct nemotouch *)malloc(sizeof(struct nemotouch));
	if (touch == NULL)
		return NULL;
	memset(touch, 0, sizeof(struct nemotouch));

	touch->seat = seat;
	touch->node = node;

	touch->touchpoint_hash = nemohash_create(8);
	if (touch->touchpoint_hash == NULL)
		goto err1;

	touch->timeout = wl_event_loop_add_timer(compz->loop, nemotouch_dispatch_timeout, touch);
	if (touch->timeout == NULL)
		goto err2;
	wl_event_source_timer_update(touch->timeout, compz->touch_timeout);

	wl_list_init(&touch->touchpoint_list);

	wl_list_insert(&seat->touch.device_list, &touch->link);

	return touch;

err2:
	nemohash_destroy(touch->touchpoint_hash);

err1:
	free(touch);

	return NULL;
}

void nemotouch_destroy(struct nemotouch *touch)
{
	wl_list_remove(&touch->link);

	if (touch->timeout != NULL)
		wl_event_source_remove(touch->timeout);

	if (touch->touchpoint_hash != NULL)
		nemohash_destroy(touch->touchpoint_hash);

	free(touch);
}

void touchpoint_move(struct touchpoint *tp, float x, float y)
{
	tp->x = x;
	tp->y = y;
}

static void nemotouch_destroy_touchpoint(struct nemotouch *touch, struct touchpoint *tp)
{
	wl_signal_emit(&tp->destroy_signal, tp);

	wl_list_remove(&tp->focus_resource_listener.link);
	wl_list_remove(&tp->focus_view_listener.link);

	wl_list_remove(&tp->link);

	nemohash_put_value(touch->touchpoint_hash, tp->id);

	free(tp);
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
	nemohash_set_value(touch->touchpoint_hash, id, (uint64_t)tp);

	return tp;
}

struct touchpoint *nemotouch_get_touchpoint_by_id(struct nemotouch *touch, uint64_t id)
{
	uint64_t value;

	if (nemohash_get_value(touch->touchpoint_hash, id, &value) != 0)
		return (struct touchpoint *)value;

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

	tp = nemotouch_get_touchpoint_by_id(touch, id);
	if (tp == NULL) {
		tp = nemotouch_create_touchpoint(touch, id);
		if (tp == NULL)
			return;
	}

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

	tp = nemotouch_get_touchpoint_by_id(touch, id);
	if (tp == NULL)
		return;

	tp->grab->interface->frame(tp->grab);
}

void nemotouch_flush_tuio(struct nemotouch *touch, struct tuionode *node)
{
	struct touchpoint *tp, *tnext;
	struct wl_list touchpoint_list;
	uint32_t msecs;
	float x, y;
	int i;

	msecs = time_current_msecs();

	wl_list_init(&touchpoint_list);
	wl_list_insert_list(&touchpoint_list, &touch->touchpoint_list);
	wl_list_init(&touch->touchpoint_list);

	for (i = 0; i < node->alive.index; i++) {
		tp = nemotouch_get_touchpoint_by_id(touch, node->taps[i].id);
		if (tp == NULL) {
			tp = nemotouch_create_touchpoint(touch, node->taps[i].id);
			if (tp == NULL)
				return;

			if (node->base.screen != NULL) {
				nemoscreen_transform_to_global(node->base.screen,
						node->taps[i].f[0], node->taps[i].f[1], &x, &y);
			} else {
				x = node->taps[i].f[0] + node->base.x;
				y = node->taps[i].f[1] + node->base.y;
			}

			tp->grab_x = x;
			tp->grab_y = y;

			tp->grab->interface->down(tp->grab, msecs, tp->gid, x, y);

			tp->grab_serial = wl_display_get_serial(node->compz->display);
			tp->grab_time = msecs;

			nemocompz_run_touch_binding(touch->seat->compz, tp, msecs);
		} else {
			if (node->base.screen != NULL) {
				nemoscreen_transform_to_global(node->base.screen,
						node->taps[i].f[0], node->taps[i].f[1], &x, &y);
			} else {
				x = node->taps[i].f[0] + node->base.x;
				y = node->taps[i].f[1] + node->base.y;
			}

			tp->grab->interface->motion(tp->grab, msecs, tp->gid, x, y);

			wl_list_remove(&tp->link);
			wl_list_insert(&touch->touchpoint_list, &tp->link);
		}
	}

	wl_list_for_each_safe(tp, tnext, &touchpoint_list, link) {
		tp->grab->interface->up(tp->grab, msecs, tp->gid);

		nemotouch_destroy_touchpoint(touch, tp);
	}
}

void touchpoint_start_grab(struct touchpoint *tp, struct touchpoint_grab *grab)
{
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
