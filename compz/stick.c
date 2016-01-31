#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <assert.h>
#include <wayland-server.h>
#include <wayland-nemo-seat-server-protocol.h>

#include <stick.h>
#include <seat.h>
#include <compz.h>
#include <view.h>
#include <content.h>
#include <canvas.h>
#include <actor.h>
#include <layer.h>
#include <nemomisc.h>

static void nemo_stick_release(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static const struct nemo_stick_interface nemo_stick_implementation = {
	nemo_stick_release
};

static void nemostick_unbind_nemo(struct wl_resource *resource)
{
	wl_list_remove(wl_resource_get_link(resource));
}

int nemostick_bind_nemo(struct wl_client *client, struct wl_resource *seat_resource, uint32_t id)
{
	struct nemoseat *seat = (struct nemoseat *)wl_resource_get_user_data(seat_resource);
	struct nemostick *stick;
	struct wl_resource *resource;

	resource = wl_resource_create(client, &nemo_stick_interface, wl_resource_get_version(seat_resource), id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return -1;
	}

	wl_resource_set_implementation(resource, &nemo_stick_implementation, seat, nemostick_unbind_nemo);

	wl_list_insert(&seat->stick.nemo_resource_list, wl_resource_get_link(resource));

	return 0;
}

static void nemostick_handle_focus_resource_destroy(struct wl_listener *listener, void *data)
{
	struct nemostick *stick = (struct nemostick *)container_of(listener, struct nemostick, focus_resource_listener);

	stick->focus = NULL;

	nemostick_set_focus(stick, NULL);
}

static void nemostick_handle_focus_view_destroy(struct wl_listener *listener, void *data)
{
	struct nemostick *stick = (struct nemostick *)container_of(listener, struct nemostick, focus_view_listener);

	stick->focus = NULL;

	nemostick_set_focus(stick, NULL);
}

struct nemostick *nemostick_create(struct nemoseat *seat, struct inputnode *node)
{
	struct nemostick *stick;

	stick = (struct nemostick *)malloc(sizeof(struct nemostick));
	if (stick == NULL)
		return NULL;
	memset(stick, 0, sizeof(struct nemostick));

	stick->seat = seat;
	stick->node = node;
	stick->id = ++seat->compz->stick_ids;

	wl_signal_init(&stick->destroy_signal);

	wl_list_init(&stick->focus_resource_listener.link);
	stick->focus_resource_listener.notify = nemostick_handle_focus_resource_destroy;
	wl_list_init(&stick->focus_view_listener.link);
	stick->focus_view_listener.notify = nemostick_handle_focus_view_destroy;

	wl_list_insert(&seat->stick.device_list, &stick->link);

	return stick;

err1:
	free(stick);

	return NULL;
}

void nemostick_destroy(struct nemostick *stick)
{
	wl_signal_emit(&stick->destroy_signal, stick);

	wl_list_remove(&stick->focus_resource_listener.link);
	wl_list_remove(&stick->focus_view_listener.link);

	wl_list_remove(&stick->link);

	free(stick);
}

void nemostick_set_focus(struct nemostick *stick, struct nemoview *view)
{
	if ((stick->focus == NULL && view) ||
			(stick->focus && view == NULL) ||
			(stick->focus && stick->focus->content != view->content)) {
		if (stick->focus != NULL)
			nemocontent_stick_leave(stick, stick->focus->content);
		if (view != NULL)
			nemocontent_stick_enter(stick, view->content);
	}

	wl_list_remove(&stick->focus_view_listener.link);
	wl_list_init(&stick->focus_view_listener.link);
	wl_list_remove(&stick->focus_resource_listener.link);
	wl_list_init(&stick->focus_resource_listener.link);

	if (view != NULL)
		wl_signal_add(&view->destroy_signal, &stick->focus_view_listener);
	if (view != NULL && view->canvas != NULL && view->canvas->resource != NULL)
		wl_resource_add_destroy_listener(view->canvas->resource, &stick->focus_resource_listener);

	stick->focused = stick->focus;
	stick->focus = view;

	wl_signal_emit(&stick->seat->stick.focus_signal, stick);
}

void nemostick_notify_translate(struct nemostick *stick, uint32_t time, float x, float y, float z)
{
	if (stick == NULL || stick->focus == NULL || stick->focus->content == NULL)
		return;

	nemocontent_stick_translate(stick, stick->focus->content, time, x, y, z);
}

void nemostick_notify_rotate(struct nemostick *stick, uint32_t time, float rx, float ry, float rz)
{
	if (stick == NULL || stick->focus == NULL || stick->focus->content == NULL)
		return;

	nemocontent_stick_rotate(stick, stick->focus->content, time, rx, ry, rz);
}

void nemostick_notify_button(struct nemostick *stick, uint32_t time, int32_t button, enum wl_pointer_button_state state)
{
	if (stick == NULL || stick->focus == NULL || stick->focus->content == NULL)
		return;

	nemocontent_stick_button(stick, stick->focus->content, time, button, state);
}
