#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <linux/input.h>
#include <wayland-server.h>

#include <keypad.h>
#include <keymap.h>
#include <seat.h>
#include <compz.h>
#include <view.h>
#include <canvas.h>
#include <content.h>
#include <nemomisc.h>

static void default_keypad_grab_key(struct nemokeypad_grab *grab, uint32_t time, uint32_t key, uint32_t state)
{
	struct nemokeypad *keypad = grab->keypad;

	if (keypad->focus != NULL) {
		nemocontent_keypad_key(keypad, keypad->focus->content, time, key, state);
	}
}

static void default_keypad_grab_modifiers(struct nemokeypad_grab *grab, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
	struct nemokeypad *keypad = grab->keypad;

	if (keypad->focus != NULL) {
		nemocontent_keypad_modifiers(keypad, keypad->focus->content, mods_depressed, mods_latched, mods_locked, group);
	}
}

static void default_keypad_grab_cancel(struct nemokeypad_grab *grab)
{
}

static const struct nemokeypad_grab_interface default_keypad_grab_interface = {
	default_keypad_grab_key,
	default_keypad_grab_modifiers,
	default_keypad_grab_cancel
};

static void nemokeypad_handle_focus_resource_destroy(struct wl_listener *listener, void *data)
{
	struct nemokeypad *keypad = (struct nemokeypad *)container_of(listener, struct nemokeypad, focus_resource_listener);

	nemokeypad_set_focus(keypad, NULL);
}

static void nemokeypad_handle_focus_view_destroy(struct wl_listener *listener, void *data)
{
	struct nemokeypad *keypad = (struct nemokeypad *)container_of(listener, struct nemokeypad, focus_view_listener);

	nemokeypad_set_focus(keypad, NULL);
}

struct nemokeypad *nemokeypad_create(struct nemoseat *seat)
{
	struct nemokeypad *keypad;

	keypad = (struct nemokeypad *)malloc(sizeof(struct nemokeypad));
	if (keypad == NULL)
		return NULL;
	memset(keypad, 0, sizeof(struct nemokeypad));

	keypad->seat = seat;
	keypad->id = ++seat->compz->keyboard_ids;

	wl_signal_init(&keypad->destroy_signal);

	keypad->xkb = nemoxkb_create();
	if (keypad->xkb == NULL)
		goto err1;

	keypad->default_grab.interface = &default_keypad_grab_interface;
	keypad->default_grab.keypad = keypad;
	keypad->grab = &keypad->default_grab;

	wl_list_init(&keypad->focus_resource_listener.link);
	keypad->focus_resource_listener.notify = nemokeypad_handle_focus_resource_destroy;
	wl_list_init(&keypad->focus_view_listener.link);
	keypad->focus_view_listener.notify = nemokeypad_handle_focus_view_destroy;

	wl_list_insert(&seat->keypad.device_list, &keypad->link);

	return keypad;

err1:
	free(keypad);

	return NULL;
}

void nemokeypad_destroy(struct nemokeypad *keypad)
{
	wl_signal_emit(&keypad->destroy_signal, keypad);

	nemokeypad_set_focus(keypad, NULL);

	nemoxkb_destroy(keypad->xkb);

	wl_list_remove(&keypad->focus_resource_listener.link);
	wl_list_remove(&keypad->focus_view_listener.link);

	wl_list_remove(&keypad->link);

	free(keypad);
}

void nemokeypad_set_focus(struct nemokeypad *keypad, struct nemoview *view)
{
	if (keypad->focus == view)
		return;

	if (keypad->focus != view) {
		if (keypad->focus != NULL) {
			if (--keypad->focus->keyboard_count == 0) {
				nemocontent_keypad_leave(keypad, keypad->focus->content);
			}
		}
		if (view != NULL) {
			if (++view->keyboard_count == 1) {
				nemocontent_keypad_enter(keypad, view->content);
			}
		}
	}

	wl_list_remove(&keypad->focus_view_listener.link);
	wl_list_init(&keypad->focus_view_listener.link);
	wl_list_remove(&keypad->focus_resource_listener.link);
	wl_list_init(&keypad->focus_resource_listener.link);

	if (view != NULL)
		wl_signal_add(&view->destroy_signal, &keypad->focus_view_listener);
	if (view != NULL && view->canvas != NULL && view->canvas->resource != NULL)
		wl_resource_add_destroy_listener(view->canvas->resource, &keypad->focus_resource_listener);

	keypad->focused = keypad->focus;
	keypad->focus = view;

	wl_signal_emit(&keypad->seat->keypad.focus_signal, keypad);
}

void nemokeypad_notify_key(struct nemokeypad *keypad, uint32_t time, uint32_t key, enum wl_keyboard_key_state state)
{
	if (keypad == NULL)
		return;

	keypad->grab->interface->key(keypad->grab, time, key, state);

	if (nemoxkb_update_key(keypad->xkb, key, state == WL_KEYBOARD_KEY_STATE_PRESSED ? XKB_KEY_DOWN : XKB_KEY_UP) != 0)
		keypad->grab->interface->modifiers(keypad->grab,
				keypad->xkb->mods_depressed,
				keypad->xkb->mods_latched,
				keypad->xkb->mods_locked,
				keypad->xkb->group);
}

void nemokeypad_start_grab(struct nemokeypad *keypad, struct nemokeypad_grab *grab)
{
	keypad->grab = grab;
	grab->keypad = keypad;
}

void nemokeypad_end_grab(struct nemokeypad *keypad)
{
	keypad->grab = &keypad->default_grab;
}

void nemokeypad_cancel_grab(struct nemokeypad *keypad)
{
	keypad->grab->interface->cancel(keypad->grab);
}

int nemokeypad_is_caps_on(struct nemokeypad *keypad)
{
	return xkb_state_mod_index_is_active(keypad->xkb->state, keypad->xkb->xkbinfo->caps_mod, XKB_STATE_MODS_EFFECTIVE);
}

int nemokeypad_is_shift_on(struct nemokeypad *keypad)
{
	return xkb_state_mod_index_is_active(keypad->xkb->state, keypad->xkb->xkbinfo->shift_mod, XKB_STATE_MODS_EFFECTIVE);
}
