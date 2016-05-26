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

static void nemo_keyboard_key(struct wl_client *client, struct wl_resource *keyboard_resource, uint32_t time, uint32_t key, uint32_t state)
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

	wl_array_init(&keyboard->keys);

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

	nemoxkb_destroy(keyboard->xkb);

	wl_array_release(&keyboard->keys);

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

static void nemokeyboard_notify_modifiers(struct nemokeyboard *keyboard)
{
	uint32_t mods_depressed, mods_latched, mods_locked, mods_lookup, group;
	uint32_t leds = 0;
	int changed = 0;

	mods_depressed = xkb_state_serialize_mods(keyboard->xkb->state, XKB_STATE_DEPRESSED);
	mods_latched = xkb_state_serialize_mods(keyboard->xkb->state, XKB_STATE_LATCHED);
	mods_locked = xkb_state_serialize_mods(keyboard->xkb->state, XKB_STATE_LOCKED);
	group = xkb_state_serialize_group(keyboard->xkb->state, XKB_STATE_EFFECTIVE);

	if (mods_depressed != keyboard->modifiers.mods_depressed ||
			mods_latched != keyboard->modifiers.mods_latched ||
			mods_locked != keyboard->modifiers.mods_locked ||
			group != keyboard->modifiers.group)
		changed = 1;

	keyboard->modifiers.mods_depressed = mods_depressed;
	keyboard->modifiers.mods_latched = mods_latched;
	keyboard->modifiers.mods_locked = mods_locked;
	keyboard->modifiers.group = group;

	mods_lookup = mods_depressed | mods_latched;
	keyboard->modifiers_state = 0;
	if (mods_lookup & (1 << keyboard->xkb->xkbinfo->ctrl_mod))
		keyboard->modifiers_state |= MODIFIER_CTRL;
	if (mods_lookup & (1 << keyboard->xkb->xkbinfo->alt_mod))
		keyboard->modifiers_state |= MODIFIER_ALT;
	if (mods_lookup & (1 << keyboard->xkb->xkbinfo->super_mod))
		keyboard->modifiers_state |= MODIFIER_SUPER;
	if (mods_lookup & (1 << keyboard->xkb->xkbinfo->shift_mod))
		keyboard->modifiers_state |= MODIFIER_SHIFT;

	if (xkb_state_led_index_is_active(keyboard->xkb->state, keyboard->xkb->xkbinfo->num_led))
		leds |= LED_NUM_LOCK;
	if (xkb_state_led_index_is_active(keyboard->xkb->state, keyboard->xkb->xkbinfo->caps_led))
		leds |= LED_CAPS_LOCK;
	if (xkb_state_led_index_is_active(keyboard->xkb->state, keyboard->xkb->xkbinfo->scroll_led))
		leds |= LED_SCROLL_LOCK;
	keyboard->leds_state = leds;

	if (changed) {
		keyboard->grab->interface->modifiers(keyboard->grab,
				keyboard->modifiers.mods_depressed,
				keyboard->modifiers.mods_latched,
				keyboard->modifiers.mods_locked,
				keyboard->modifiers.group);
	}
}

static void nemokeyboard_update_modifiers(struct nemokeyboard *keyboard, uint32_t serial, uint32_t key, enum wl_keyboard_key_state state)
{
	enum xkb_key_direction direction;

	if (state == WL_KEYBOARD_KEY_STATE_PRESSED)
		direction = XKB_KEY_DOWN;
	else
		direction = XKB_KEY_UP;

	xkb_state_update_key(keyboard->xkb->state, key + 8, direction);

	nemokeyboard_notify_modifiers(keyboard);
}

void nemokeyboard_notify_key(struct nemokeyboard *keyboard, uint32_t time, uint32_t key, enum wl_keyboard_key_state state)
{
	if (keyboard == NULL)
		return;

	nemocompz_run_key_binding(keyboard->seat->compz, keyboard, time, key, state);

	keyboard->grab->interface->key(keyboard->grab, time, key, state);

	nemokeyboard_update_modifiers(keyboard, wl_display_get_serial(keyboard->seat->compz->display), key, state);
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
