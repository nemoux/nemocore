#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <linux/input.h>
#include <wayland-server.h>
#include <wayland-nemo-seat-server-protocol.h>

#include <keyboard.h>
#include <keymap.h>
#include <seat.h>
#include <view.h>
#include <canvas.h>
#include <content.h>
#include <binding.h>
#include <nemomisc.h>

static void default_keyboard_grab_key(struct nemokeyboard_grab *grab, uint32_t time, uint32_t key, uint32_t state)
{
	struct nemokeyboard *keyboard = grab->keyboard;

	if (keyboard->focus != NULL) {
		nemocontent_keyboard_key(keyboard, keyboard->focus->content, time, key, state);
	}
}

static void default_keyboard_grab_modifiers(struct nemokeyboard_grab *grab, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
	struct nemokeyboard *keyboard = grab->keyboard;

	if (keyboard->focus != NULL) {
		nemocontent_keyboard_modifiers(keyboard, keyboard->focus->content, mods_depressed, mods_latched, mods_locked, group);
	}
}

static void default_keyboard_grab_cancel(struct nemokeyboard_grab *grab)
{
	struct nemokeyboard *keyboard = grab->keyboard;
}

static const struct nemokeyboard_grab_interface default_keyboard_grab_interface = {
	default_keyboard_grab_key,
	default_keyboard_grab_modifiers,
	default_keyboard_grab_cancel
};

static void wayland_keyboard_release(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static const struct wl_keyboard_interface wayland_keyboard_implementation = {
	wayland_keyboard_release
};

static void nemokeyboard_unbind_wayland(struct wl_resource *resource)
{
	wl_list_remove(wl_resource_get_link(resource));
}

int nemokeyboard_bind_wayland(struct wl_client *client, struct wl_resource *seat_resource, uint32_t id)
{
	struct nemoseat *seat = (struct nemoseat *)wl_resource_get_user_data(seat_resource);
	struct nemokeyboard *keyboard;
	struct wl_resource *resource;

	resource = wl_resource_create(client, &wl_keyboard_interface, wl_resource_get_version(seat_resource), id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return -1;
	}

	wl_resource_set_implementation(resource, &wayland_keyboard_implementation, seat, nemokeyboard_unbind_wayland);

	wl_list_insert(&seat->keyboard.resource_list, wl_resource_get_link(resource));

	keyboard = nemoseat_get_first_keyboard(seat);
	if (keyboard != NULL && keyboard->xkb != NULL) {
		wl_keyboard_send_keymap(resource, WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1, keyboard->xkb->xkbinfo->keymap_fd, keyboard->xkb->xkbinfo->keymap_size);
	}

	return 0;
}

static void nemo_keyboard_enter(struct wl_client *client, struct wl_resource *keyboard_resource)
{
	struct nemoseat *seat = (struct nemoseat *)wl_resource_get_user_data(keyboard_resource);
	struct nemocompz *compz = seat->compz;
	struct nemoview *view;
	struct wl_list *resource_list;
	struct wl_resource *resource;
	uint32_t serial;

	serial = wl_display_next_serial(compz->display);

	view = nemocompz_get_view_by_client(compz, client);
	if (view == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	if (view->xkb == NULL)
		view->xkb = nemoxkb_create();

	if (view->focus == NULL)
		return;

	resource_list = &seat->keyboard.resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == view->focus->client) {
			wl_keyboard_send_keymap(resource, WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1, view->xkb->xkbinfo->keymap_fd, view->xkb->xkbinfo->keymap_size);

			wl_keyboard_send_modifiers(resource, serial,
					view->xkb->mods_depressed,
					view->xkb->mods_latched,
					view->xkb->mods_locked,
					view->xkb->group);

			wl_keyboard_send_enter(resource, serial, view->focus->canvas->resource, &view->xkb->keys);
		}
	}

	resource_list = &seat->keyboard.nemo_resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == view->focus->client) {
			nemo_keyboard_send_keymap(resource, WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1, view->xkb->xkbinfo->keymap_fd, view->xkb->xkbinfo->keymap_size);

			nemo_keyboard_send_modifiers(resource, serial,
					view->focus->canvas->resource,
					wl_resource_get_id(keyboard_resource),
					view->xkb->mods_depressed,
					view->xkb->mods_latched,
					view->xkb->mods_locked,
					view->xkb->group);

			nemo_keyboard_send_enter(resource,
					serial,
					view->focus->canvas->resource,
					wl_resource_get_id(keyboard_resource),
					&view->xkb->keys);
		}
	}
}

static void nemo_keyboard_leave(struct wl_client *client, struct wl_resource *keyboard_resource)
{
	struct nemoseat *seat = (struct nemoseat *)wl_resource_get_user_data(keyboard_resource);
	struct nemocompz *compz = seat->compz;
	struct nemoview *view;
	struct wl_list *resource_list;
	struct wl_resource *resource;
	uint32_t serial;

	serial = wl_display_next_serial(compz->display);

	view = nemocompz_get_view_by_client(compz, client);
	if (view == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	if (view->focus == NULL)
		return;

	resource_list = &seat->keyboard.resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == view->focus->client) {
			wl_keyboard_send_leave(resource, serial, view->focus->canvas->resource);
		}
	}

	resource_list = &seat->keyboard.nemo_resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == view->focus->client) {
			nemo_keyboard_send_leave(resource,
					serial,
					view->focus->canvas->resource,
					wl_resource_get_id(keyboard_resource));
		}
	}
}

static void nemo_keyboard_key(struct wl_client *client, struct wl_resource *keyboard_resource, uint32_t time, uint32_t key, uint32_t state)
{
	struct nemoseat *seat = (struct nemoseat *)wl_resource_get_user_data(keyboard_resource);
	struct nemocompz *compz = seat->compz;
	struct nemoview *view;
	struct wl_list *resource_list;
	struct wl_resource *resource;
	uint32_t serial;
	int changed;

	view = nemocompz_get_view_by_client(compz, client);
	if (view == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	if (view->xkb == NULL)
		view->xkb = nemoxkb_create();

	changed = nemoxkb_update_key(view->xkb, key, state == NEMO_KEYBOARD_KEY_STATE_PRESSED ? XKB_KEY_DOWN : XKB_KEY_UP);

	if (view->focus == NULL)
		return;

	if (changed != 0) {
		serial = wl_display_next_serial(compz->display);

		resource_list = &seat->keyboard.resource_list;

		wl_resource_for_each(resource, resource_list) {
			if (wl_resource_get_client(resource) == view->focus->client) {
				wl_keyboard_send_modifiers(resource, serial,
						view->xkb->mods_depressed,
						view->xkb->mods_latched,
						view->xkb->mods_locked,
						view->xkb->group);
			}
		}

		resource_list = &seat->keyboard.nemo_resource_list;

		wl_resource_for_each(resource, resource_list) {
			if (wl_resource_get_client(resource) == view->focus->client) {
				nemo_keyboard_send_modifiers(resource, serial,
						view->focus->canvas->resource,
						wl_resource_get_id(keyboard_resource),
						view->xkb->mods_depressed,
						view->xkb->mods_latched,
						view->xkb->mods_locked,
						view->xkb->group);
			}
		}
	}

	serial = wl_display_next_serial(compz->display);

	resource_list = &seat->keyboard.resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == view->focus->client) {
			wl_keyboard_send_key(resource, serial, time, key, state);
		}
	}

	resource_list = &seat->keyboard.nemo_resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == view->focus->client) {
			nemo_keyboard_send_key(resource,
					serial, time,
					view->focus->canvas->resource,
					wl_resource_get_id(keyboard_resource),
					key, state);
		}
	}
}

static void nemo_keyboard_layout(struct wl_client *client, struct wl_resource *keyboard_resource, const char *name)
{
	struct nemoseat *seat = (struct nemoseat *)wl_resource_get_user_data(keyboard_resource);
	struct nemocompz *compz = seat->compz;
	struct nemoview *view;
	struct wl_list *resource_list;
	struct wl_resource *resource;
	uint32_t serial;

	serial = wl_display_next_serial(compz->display);

	view = nemocompz_get_view_by_client(compz, client);
	if (view == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	if (view->focus == NULL)
		return;

	resource_list = &seat->keyboard.nemo_resource_list;

	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_client(resource) == view->focus->client) {
			nemo_keyboard_send_layout(resource,
					serial,
					view->focus->canvas->resource,
					wl_resource_get_id(keyboard_resource),
					name);
		}
	}
}

static void nemo_keyboard_release(struct wl_client *client, struct wl_resource *keyboard_resource)
{
	wl_resource_destroy(keyboard_resource);
}

static const struct nemo_keyboard_interface nemo_keyboard_implementation = {
	nemo_keyboard_enter,
	nemo_keyboard_leave,
	nemo_keyboard_key,
	nemo_keyboard_layout,
	nemo_keyboard_release
};

static void nemokeyboard_unbind_nemo(struct wl_resource *resource)
{
	wl_list_remove(wl_resource_get_link(resource));
}

int nemokeyboard_bind_nemo(struct wl_client *client, struct wl_resource *seat_resource, uint32_t id)
{
	struct nemoseat *seat = (struct nemoseat *)wl_resource_get_user_data(seat_resource);
	struct nemokeyboard *keyboard;
	struct wl_resource *resource;

	resource = wl_resource_create(client, &nemo_keyboard_interface, wl_resource_get_version(seat_resource), id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return -1;
	}

	wl_resource_set_implementation(resource, &nemo_keyboard_implementation, seat, nemokeyboard_unbind_nemo);

	wl_list_insert(&seat->keyboard.nemo_resource_list, wl_resource_get_link(resource));

	keyboard = nemoseat_get_first_keyboard(seat);
	if (keyboard != NULL && keyboard->xkb != NULL) {
		nemo_keyboard_send_keymap(resource, WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1, keyboard->xkb->xkbinfo->keymap_fd, keyboard->xkb->xkbinfo->keymap_size);
	}

	return 0;
}

static void nemokeyboard_handle_focus_resource_destroy(struct wl_listener *listener, void *data)
{
	struct nemokeyboard *keyboard = (struct nemokeyboard *)container_of(listener, struct nemokeyboard, focus_resource_listener);

	keyboard->focus = NULL;

	nemokeyboard_set_focus(keyboard, NULL);
}

static void nemokeyboard_handle_focus_view_destroy(struct wl_listener *listener, void *data)
{
	struct nemokeyboard *keyboard = (struct nemokeyboard *)container_of(listener, struct nemokeyboard, focus_view_listener);

	keyboard->focus = NULL;

	nemokeyboard_set_focus(keyboard, NULL);
}

struct nemokeyboard *nemokeyboard_create(struct nemoseat *seat, struct inputnode *node)
{
	struct nemokeyboard *keyboard;

	keyboard = (struct nemokeyboard *)malloc(sizeof(struct nemokeyboard));
	if (keyboard == NULL)
		return NULL;
	memset(keyboard, 0, sizeof(struct nemokeyboard));

	keyboard->seat = seat;
	keyboard->node = node;
	keyboard->id = ++seat->compz->keyboard_ids;

	wl_signal_init(&keyboard->destroy_signal);

	keyboard->xkb = nemoxkb_create();
	if (keyboard->xkb == NULL)
		goto err1;

	keyboard->default_grab.interface = &default_keyboard_grab_interface;
	keyboard->default_grab.keyboard = keyboard;
	keyboard->grab = &keyboard->default_grab;

	wl_list_init(&keyboard->focus_resource_listener.link);
	keyboard->focus_resource_listener.notify = nemokeyboard_handle_focus_resource_destroy;
	wl_list_init(&keyboard->focus_view_listener.link);
	keyboard->focus_view_listener.notify = nemokeyboard_handle_focus_view_destroy;

	wl_list_insert(&seat->keyboard.device_list, &keyboard->link);

	return keyboard;

err1:
	free(keyboard);

	return NULL;
}

void nemokeyboard_destroy(struct nemokeyboard *keyboard)
{
	wl_signal_emit(&keyboard->destroy_signal, keyboard);

	nemokeyboard_set_focus(keyboard, NULL);

	nemoxkb_destroy(keyboard->xkb);

	wl_list_remove(&keyboard->focus_resource_listener.link);
	wl_list_remove(&keyboard->focus_view_listener.link);

	wl_list_remove(&keyboard->link);

	free(keyboard);
}

void nemokeyboard_set_focus(struct nemokeyboard *keyboard, struct nemoview *view)
{
	if (keyboard->focus != view) {
		if (keyboard->focus != NULL) {
			if (--keyboard->focus->keyboard_count == 0) {
				nemocontent_keyboard_leave(keyboard, keyboard->focus->content);
			}
		}
		if (view != NULL) {
			if (++view->keyboard_count == 1) {
				nemocontent_keyboard_enter(keyboard, view->content);
			}
		}
	}

	wl_list_remove(&keyboard->focus_view_listener.link);
	wl_list_init(&keyboard->focus_view_listener.link);
	wl_list_remove(&keyboard->focus_resource_listener.link);
	wl_list_init(&keyboard->focus_resource_listener.link);

	if (view != NULL)
		wl_signal_add(&view->destroy_signal, &keyboard->focus_view_listener);
	if (view != NULL && view->canvas != NULL && view->canvas->resource != NULL)
		wl_resource_add_destroy_listener(view->canvas->resource, &keyboard->focus_resource_listener);

	keyboard->focused = keyboard->focus;
	keyboard->focus = view;

	wl_signal_emit(&keyboard->seat->keyboard.focus_signal, keyboard);
}

void nemokeyboard_notify_key(struct nemokeyboard *keyboard, uint32_t time, uint32_t key, enum wl_keyboard_key_state state)
{
	if (keyboard == NULL)
		return;

	nemocompz_run_key_binding(keyboard->seat->compz, keyboard, time, key, state);

	keyboard->grab->interface->key(keyboard->grab, time, key, state);

	if (nemoxkb_update_key(keyboard->xkb, key, state == WL_KEYBOARD_KEY_STATE_PRESSED ? XKB_KEY_DOWN : XKB_KEY_UP) != 0)
		keyboard->grab->interface->modifiers(keyboard->grab,
				keyboard->xkb->mods_depressed,
				keyboard->xkb->mods_latched,
				keyboard->xkb->mods_locked,
				keyboard->xkb->group);
}

void nemokeyboard_start_grab(struct nemokeyboard *keyboard, struct nemokeyboard_grab *grab)
{
	keyboard->grab = grab;
	grab->keyboard = keyboard;
}

void nemokeyboard_end_grab(struct nemokeyboard *keyboard)
{
	keyboard->grab = &keyboard->default_grab;
}

void nemokeyboard_cancel_grab(struct nemokeyboard *keyboard)
{
	keyboard->grab->interface->cancel(keyboard->grab);
}
