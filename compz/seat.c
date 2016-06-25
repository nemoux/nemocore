#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>
#include <wayland-nemo-seat-server-protocol.h>

#include <seat.h>
#include <compz.h>
#include <pointer.h>
#include <keyboard.h>
#include <keypad.h>
#include <touch.h>
#include <stick.h>
#include <canvas.h>
#include <view.h>
#include <picker.h>
#include <binding.h>
#include <nemomisc.h>

static void wayland_seat_get_pointer(struct wl_client *client, struct wl_resource *seat_resource, uint32_t id)
{
	nemopointer_bind_wayland(client, seat_resource, id);
}

static void wayland_seat_get_keyboard(struct wl_client *client, struct wl_resource *seat_resource, uint32_t id)
{
	nemokeyboard_bind_wayland(client, seat_resource, id);
}

static void wayland_seat_get_touch(struct wl_client *client, struct wl_resource *seat_resource, uint32_t id)
{
	nemotouch_bind_wayland(client, seat_resource, id);
}

static const struct wl_seat_interface wayland_seat_implementation = {
	wayland_seat_get_pointer,
	wayland_seat_get_keyboard,
	wayland_seat_get_touch
};

static void nemoseat_unbind_wayland(struct wl_resource *resource)
{
	wl_list_remove(wl_resource_get_link(resource));
}

static void nemoseat_bind_wayland(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct nemoseat *seat = (struct nemoseat *)data;
	struct wl_resource *resource;

	resource = wl_resource_create(client, &wl_seat_interface, MIN(version, 4), id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_list_insert(&seat->resource_list, wl_resource_get_link(resource));
	wl_resource_set_implementation(resource, &wayland_seat_implementation, seat, nemoseat_unbind_wayland);

	wl_seat_send_capabilities(resource, WL_SEAT_CAPABILITY_POINTER | WL_SEAT_CAPABILITY_KEYBOARD | WL_SEAT_CAPABILITY_TOUCH);

	if (version >= 2)
		wl_seat_send_name(resource, "seat0");
}

static void nemo_seat_get_pointer(struct wl_client *client, struct wl_resource *seat_resource, uint32_t id)
{
	nemopointer_bind_nemo(client, seat_resource, id);
}

static void nemo_seat_get_keyboard(struct wl_client *client, struct wl_resource *seat_resource, uint32_t id)
{
	nemokeyboard_bind_nemo(client, seat_resource, id);
}

static void nemo_seat_get_touch(struct wl_client *client, struct wl_resource *seat_resource, uint32_t id)
{
	nemotouch_bind_nemo(client, seat_resource, id);
}

static void nemo_seat_get_stick(struct wl_client *client, struct wl_resource *seat_resource, uint32_t id)
{
	nemostick_bind_nemo(client, seat_resource, id);
}

static const struct nemo_seat_interface nemo_seat_implementation = {
	nemo_seat_get_pointer,
	nemo_seat_get_keyboard,
	nemo_seat_get_touch,
	nemo_seat_get_stick
};

static void nemoseat_unbind_nemo(struct wl_resource *resource)
{
	wl_list_remove(wl_resource_get_link(resource));
}

static void nemoseat_bind_nemo(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct nemoseat *seat = (struct nemoseat *)data;
	struct wl_resource *resource;

	resource = wl_resource_create(client, &nemo_seat_interface, MIN(version, 1), id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_list_insert(&seat->resource_list, wl_resource_get_link(resource));
	wl_resource_set_implementation(resource, &nemo_seat_implementation, seat, nemoseat_unbind_nemo);

	wl_seat_send_capabilities(resource, NEMO_SEAT_CAPABILITY_POINTER | NEMO_SEAT_CAPABILITY_KEYBOARD | NEMO_SEAT_CAPABILITY_TOUCH | NEMO_SEAT_CAPABILITY_STICK);

	wl_seat_send_name(resource, "seat0");
}

struct nemoseat *nemoseat_create(struct nemocompz *compz)
{
	struct nemoseat *seat;

	seat = (struct nemoseat *)malloc(sizeof(struct nemoseat));
	if (seat == NULL)
		return NULL;
	memset(seat, 0, sizeof(struct nemoseat));

	seat->compz = compz;

	wl_signal_init(&seat->destroy_signal);

	wl_list_init(&seat->resource_list);
	wl_list_init(&seat->drag_resource_list);

	wl_list_init(&seat->keyboard.resource_list);
	wl_list_init(&seat->keyboard.nemo_resource_list);
	wl_list_init(&seat->keyboard.device_list);
	wl_signal_init(&seat->keyboard.focus_signal);

	wl_list_init(&seat->keypad.device_list);
	wl_signal_init(&seat->keypad.focus_signal);

	wl_list_init(&seat->pointer.resource_list);
	wl_list_init(&seat->pointer.nemo_resource_list);
	wl_list_init(&seat->pointer.device_list);
	wl_signal_init(&seat->pointer.focus_signal);
	wl_signal_init(&seat->pointer.sprite_signal);

	wl_list_init(&seat->touch.resource_list);
	wl_list_init(&seat->touch.nemo_resource_list);
	wl_list_init(&seat->touch.device_list);
	wl_signal_init(&seat->touch.focus_signal);

	wl_list_init(&seat->stick.resource_list);
	wl_list_init(&seat->stick.nemo_resource_list);
	wl_list_init(&seat->stick.device_list);
	wl_signal_init(&seat->stick.focus_signal);

	wl_list_init(&seat->selection.data_source_listener.link);
	wl_signal_init(&seat->selection.signal);

	if (!wl_global_create(compz->display, &wl_seat_interface, 4, seat, nemoseat_bind_wayland))
		goto err1;
	if (!wl_global_create(compz->display, &nemo_seat_interface, 1, seat, nemoseat_bind_nemo))
		goto err1;

	return seat;

err1:
	free(seat);

	return NULL;
}

void nemoseat_destroy(struct nemoseat *seat)
{
	wl_signal_emit(&seat->destroy_signal, seat);

	free(seat);
}

struct nemopointer *nemoseat_get_first_pointer(struct nemoseat *seat)
{
	if (wl_list_empty(&seat->pointer.device_list))
		return NULL;

	return (struct nemopointer *)container_of(seat->pointer.device_list.next, struct nemopointer, link);
}

struct nemokeyboard *nemoseat_get_first_keyboard(struct nemoseat *seat)
{
	if (wl_list_empty(&seat->keyboard.device_list))
		return NULL;

	return (struct nemokeyboard *)container_of(seat->keyboard.device_list.next, struct nemokeyboard, link);
}

struct nemopointer *nemoseat_get_pointer_by_focus_serial(struct nemoseat *seat, uint32_t serial)
{
	struct nemopointer *pointer;

	wl_list_for_each(pointer, &seat->pointer.device_list, link) {
		if (pointer->focus_serial == serial)
			return pointer;
	}

	return NULL;
}

struct nemopointer *nemoseat_get_pointer_by_grab_serial(struct nemoseat *seat, uint32_t serial)
{
	struct nemopointer *pointer;

	wl_list_for_each(pointer, &seat->pointer.device_list, link) {
		if (pointer->grab_serial == serial)
			return pointer;
	}

	return NULL;
}

struct nemopointer *nemoseat_get_pointer_by_id(struct nemoseat *seat, uint64_t id)
{
	struct nemopointer *pointer;

	wl_list_for_each(pointer, &seat->pointer.device_list, link) {
		if (pointer->id == id)
			return pointer;
	}

	return NULL;
}

int nemoseat_get_pointer_by_view(struct nemoseat *seat, struct nemoview *view, struct nemopointer *ptrs[], int max)
{
	struct nemopointer *pointer;
	int ptrcount = 0;

	wl_list_for_each(pointer, &seat->pointer.device_list, link) {
		if (pointer->focus == view) {
			ptrs[ptrcount++] = pointer;

			if (ptrcount >= max)
				return ptrcount;
		}
	}

	return ptrcount;
}

struct nemokeyboard *nemoseat_get_keyboard_by_focus_serial(struct nemoseat *seat, uint32_t serial)
{
	struct nemokeyboard *keyboard;

	wl_list_for_each(keyboard, &seat->keyboard.device_list, link) {
		if (keyboard->focus_serial == serial)
			return keyboard;
	}

	return NULL;
}

struct nemokeyboard *nemoseat_get_keyboard_by_id(struct nemoseat *seat, uint64_t id)
{
	struct nemokeyboard *keyboard;

	wl_list_for_each(keyboard, &seat->keyboard.device_list, link) {
		if (keyboard->id == id)
			return keyboard;
	}

	return NULL;
}

int nemoseat_get_keyboard_by_view(struct nemoseat *seat, struct nemoview *view, struct nemokeyboard *kbds[], int max)
{
	struct nemokeyboard *keyboard;
	int kbdcount = 0;

	wl_list_for_each(keyboard, &seat->keyboard.device_list, link) {
		if (keyboard->focus == view) {
			kbds[kbdcount++] = keyboard;

			if (kbdcount >= max)
				return kbdcount;
		}
	}

	return kbdcount;
}

struct nemokeypad *nemoseat_get_keypad_by_id(struct nemoseat *seat, uint64_t id)
{
	struct nemokeypad *keypad;

	wl_list_for_each(keypad, &seat->keypad.device_list, link) {
		if (keypad->id == id)
			return keypad;
	}

	return NULL;
}

struct touchpoint *nemoseat_get_touchpoint_by_grab_serial(struct nemoseat *seat, uint32_t serial)
{
	struct nemotouch *touch;
	struct touchpoint *tp;

	wl_list_for_each(touch, &seat->touch.device_list, link) {
		wl_list_for_each(tp, &touch->touchpoint_list, link) {
			if (tp->state == TOUCHPOINT_UP_STATE)
				continue;

			if (tp->grab_serial == serial)
				return tp;
		}
	}

	return NULL;
}

struct touchpoint *nemoseat_get_touchpoint_by_id(struct nemoseat *seat, uint64_t id)
{
	struct nemotouch *touch;
	struct touchpoint *tp;

	wl_list_for_each(touch, &seat->touch.device_list, link) {
		wl_list_for_each(tp, &touch->touchpoint_list, link) {
			if (tp->state == TOUCHPOINT_UP_STATE)
				continue;

			if (tp->gid == id)
				return tp;
		}
	}

	return NULL;
}

int nemoseat_get_touchpoint_by_view(struct nemoseat *seat, struct nemoview *view, struct touchpoint *tps[], int max)
{
	struct nemotouch *touch;
	struct touchpoint *tp;
	int tpcount = 0;

	wl_list_for_each(touch, &seat->touch.device_list, link) {
		wl_list_for_each(tp, &touch->touchpoint_list, link) {
			if (tp->state == TOUCHPOINT_UP_STATE)
				continue;

			if (tp->focus == view) {
				tps[tpcount++] = tp;

				if (tpcount >= max)
					return tpcount;
			}
		}
	}

	return tpcount;
}

int nemoseat_put_touchpoint_by_view(struct nemoseat *seat, struct nemoview *view)
{
	struct nemocompz *compz = seat->compz;
	struct nemotouch *touch;
	struct touchpoint *tp, *tnext;
	uint32_t msecs = time_current_msecs();

	wl_list_for_each(touch, &seat->touch.device_list, link) {
		wl_list_for_each_safe(tp, tnext, &touch->touchpoint_list, link) {
			if (tp->state == TOUCHPOINT_UP_STATE)
				continue;

			if (tp->focus == view) {
				nemocontent_touch_up(tp, tp->focus->content, msecs, tp->gid);

				touchpoint_set_focus(tp, NULL);
			}
		}
	}

	return 0;
}

void nemoseat_bypass_touchpoint_by_view(struct nemoseat *seat, struct nemoview *view)
{
	struct nemocompz *compz = seat->compz;
	struct nemotouch *touch;
	struct touchpoint *tp;
	struct nemoview *pick;
	uint32_t msecs = time_current_msecs();
	float tx, ty;

	wl_list_for_each(touch, &seat->touch.device_list, link) {
		wl_list_for_each(tp, &touch->touchpoint_list, link) {
			if (tp->state == TOUCHPOINT_UP_STATE)
				continue;

			if (tp->focus == view) {
				pick = nemocompz_pick_view(compz, tp->x, tp->y, &tx, &ty, NEMOVIEW_PICK_STATE);
				if (pick != NULL) {
					nemocontent_touch_up(tp, tp->focus->content, msecs, tp->gid);

					touchpoint_set_focus(tp, pick);

					nemocontent_touch_down(tp, tp->focus->content, msecs, tp->gid, tx, ty, tp->x, tp->y);

					tp->grab_serial = wl_display_get_serial(compz->display);
					tp->grab_time = msecs;

					nemocompz_run_touch_binding(compz, tp, msecs);
				}
			}
		}
	}
}

void nemoseat_get_distant_touchpoint(struct nemoseat *seat, struct touchpoint *tps[], int ntps, struct touchpoint **tp0, struct touchpoint **tp1)
{
	struct nemotouch *touch;
	struct touchpoint *_tp0, *_tp1;
	float dm = 0.0f;
	float dd;
	float dx, dy;
	int i, j;

	for (i = 0; i < ntps - 1; i++) {
		_tp0 = tps[i];

		for (j = i + 1; j < ntps; j++) {
			_tp1 = tps[j];

			dx = _tp1->x - _tp0->x;
			dy = _tp1->y - _tp0->y;
			dd = sqrtf(dx * dx + dy * dy);

			if (dd > dm) {
				dm = dd;

				*tp0 = _tp0;
				*tp1 = _tp1;
			}
		}
	}
}

struct wl_resource *nemoseat_find_resource_for_view(struct wl_list *list, struct nemoview *view)
{
	if (view == NULL || view->canvas == NULL || view->canvas->resource == NULL)
		return NULL;

	return wl_resource_find_for_client(list, wl_resource_get_client(view->canvas->resource));
}

void nemoseat_set_keyboard_focus(struct nemoseat *seat, struct nemoview *view)
{
	struct nemokeyboard *keyboard;

	wl_list_for_each(keyboard, &seat->keyboard.device_list, link) {
		nemokeyboard_set_focus(keyboard, view);
	}
}

void nemoseat_set_pointer_focus(struct nemoseat *seat, struct nemoview *view)
{
	struct nemopointer *pointer;

	wl_list_for_each(pointer, &seat->pointer.device_list, link) {
		nemopointer_set_focus(pointer, view, 0, 0);
	}
}

void nemoseat_set_stick_focus(struct nemoseat *seat, struct nemoview *view)
{
	struct nemostick *stick;

	wl_list_for_each(stick, &seat->stick.device_list, link) {
		nemostick_set_focus(stick, view);
	}
}

void nemoseat_put_focus(struct nemoseat *seat, struct nemoview *view)
{
	struct nemopointer *pointer;
	struct nemokeyboard *keyboard;
	struct nemokeypad *keypad;
	struct nemotouch *touch;
	struct touchpoint *tp;
	struct nemostick *stick;

	wl_list_for_each(pointer, &seat->pointer.device_list, link) {
		if (pointer->focus == view) {
			nemopointer_set_focus(pointer, NULL, 0, 0);
		}
	}

	wl_list_for_each(keyboard, &seat->keyboard.device_list, link) {
		if (keyboard->focus == view) {
			nemokeyboard_set_focus(keyboard, NULL);
		}
	}

	wl_list_for_each(keypad, &seat->keypad.device_list, link) {
		if (keypad->focus == view) {
			nemokeypad_set_focus(keypad, NULL);
		}
	}

	wl_list_for_each(touch, &seat->touch.device_list, link) {
		wl_list_for_each(tp, &touch->touchpoint_list, link) {
			if (tp->focus == view) {
				touchpoint_set_focus(tp, NULL);
			}
		}
	}

	wl_list_for_each(stick, &seat->stick.device_list, link) {
		if (stick->focus == view) {
			nemostick_set_focus(stick, NULL);
		}
	}
}
