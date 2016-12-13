#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <linux/input.h>

#include <nemoshow.h>
#include <showone.h>
#include <showevent.h>
#include <showgrab.h>
#include <nemomisc.h>

static inline void nemoshow_dispatch_event(struct nemoshow *show, struct showone *one, struct showevent *event)
{
	if (nemoshow_dispatch_grab(show, event) == 0) {
		if (one != NULL) {
			struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

			if (canvas->dispatch_event != NULL)
				canvas->dispatch_event(show, one, event);
		} else if (show->dispatch_event != NULL) {
			show->dispatch_event(show, event);
		}
	}
}

static inline struct showtap *nemoshow_get_pointer_tap(struct nemoshow *show, uint64_t device)
{
	struct showtap *tap;

	nemolist_for_each(tap, &show->ptap_list, link) {
		if (tap->device == device)
			return tap;
	}

	return NULL;
}

static inline struct showtap *nemoshow_get_touch_tap(struct nemoshow *show, uint64_t device)
{
	struct showtap *tap;

	nemolist_for_each_reverse(tap, &show->tap_list, link) {
		if (tap->device == device)
			return tap;
	}

	return NULL;
}

static void showtap_handle_show_destroy(struct nemolistener *listener, void *data)
{
	struct showtap *tap = (struct showtap *)container_of(listener, struct showtap, show_destroy_listener);

	nemolist_remove(&tap->link);
	nemolist_init(&tap->link);
	nemolist_remove(&tap->show_destroy_listener.link);
	nemolist_init(&tap->show_destroy_listener.link);
}

static void showtap_handle_canvas_destroy(struct nemolistener *listener, void *data)
{
	struct showtap *tap = (struct showtap *)container_of(listener, struct showtap, one_destroy_listener);

	nemolist_remove(&tap->one_destroy_listener.link);
	nemolist_init(&tap->one_destroy_listener.link);

	tap->one = NULL;
}

static struct showtap *nemoshow_create_tap(struct nemoshow *show, uint64_t device)
{
	struct showtap *tap;

	tap = (struct showtap *)malloc(sizeof(struct showtap));
	if (tap == NULL)
		return NULL;

	tap->done = 0;

	tap->device = device;
	tap->one = NULL;

	tap->dist = 0.0f;

	nemolist_init(&tap->link);

	tap->show_destroy_listener.notify = showtap_handle_show_destroy;
	nemolist_init(&tap->show_destroy_listener.link);

	tap->one_destroy_listener.notify = showtap_handle_canvas_destroy;
	nemolist_init(&tap->one_destroy_listener.link);

	return tap;
}

static void nemoshow_destroy_tap(struct showtap *tap)
{
	nemolist_remove(&tap->link);
	nemolist_remove(&tap->show_destroy_listener.link);
	nemolist_remove(&tap->one_destroy_listener.link);

	free(tap);
}

static void nemoshow_remove_tap(struct showtap *tap)
{
	nemolist_remove(&tap->link);
	nemolist_init(&tap->link);
}

void nemoshow_push_pointer_enter_event(struct nemoshow *show, uint32_t serial, uint64_t device, float x, float y)
{
	struct showevent event;
	struct showtap *tap;

	nemoshow_transform_from_viewport(show, x, y, &x, &y);

	tap = nemoshow_get_pointer_tap(show, device);
	if (tap == NULL) {
		tap = nemoshow_create_tap(show, device);
		if (tap == NULL)
			return;

		nemolist_insert_tail(&show->ptap_list, &tap->link);
		nemosignal_add(&show->destroy_signal, &tap->show_destroy_listener);
	}

	tap->x = x;
	tap->y = y;

	event.type = NEMOSHOW_POINTER_ENTER_EVENT;
	event.x = x;
	event.y = y;
	event.device = device;
	event.serial = serial;
	event.tap = tap;

	nemoshow_dispatch_event(show, NULL, &event);
}

void nemoshow_push_pointer_leave_event(struct nemoshow *show, uint32_t serial, uint64_t device)
{
	struct showevent event;
	struct showtap *tap;

	tap = nemoshow_get_pointer_tap(show, device);
	if (tap == NULL)
		return;

	event.type = NEMOSHOW_POINTER_LEAVE_EVENT;
	event.device = device;
	event.serial = serial;
	event.tap = tap;

	nemoshow_dispatch_event(show, NULL, &event);

	nemoshow_destroy_tap(tap);
}

void nemoshow_push_pointer_down_event(struct nemoshow *show, uint32_t serial, uint64_t device, uint32_t time, uint32_t button)
{
	struct showevent event;
	struct showtap *tap;
	uint32_t type;
	float sx, sy;

	tap = nemoshow_get_pointer_tap(show, device);
	if (tap == NULL)
		return;

	if (button == BTN_LEFT)
		type = NEMOSHOW_POINTER_LEFT_DOWN_EVENT;
	else if (button == BTN_RIGHT)
		type = NEMOSHOW_POINTER_RIGHT_DOWN_EVENT;
	else
		type = NEMOSHOW_POINTER_BUTTON_DOWN_EVENT;

	tap->one = nemoshow_pick_canvas(show, tap->x, tap->y, &sx, &sy);
	if (tap->one != NULL) {
		nemolist_remove(&tap->one_destroy_listener.link);

		nemosignal_add(&tap->one->destroy_signal, &tap->one_destroy_listener);
	}

	event.x = tap->x;
	event.y = tap->y;

	tap->grab_time = time;
	tap->grab_value = button;
	tap->grab_x = tap->x;
	tap->grab_y = tap->y;
	tap->serial = serial;

	event.type = type;
	event.device = device;
	event.serial = serial;
	event.time = time;
	event.value = button;
	event.tap = tap;

	nemoshow_dispatch_event(show, tap->one, &event);
}

void nemoshow_push_pointer_up_event(struct nemoshow *show, uint32_t serial, uint64_t device, uint32_t time, uint32_t button)
{
	struct showevent event;
	struct showtap *tap;
	uint64_t value;
	uint32_t type;

	tap = nemoshow_get_pointer_tap(show, device);
	if (tap == NULL)
		return;

	if (button == BTN_LEFT)
		type = NEMOSHOW_POINTER_LEFT_UP_EVENT;
	else if (button == BTN_RIGHT)
		type = NEMOSHOW_POINTER_RIGHT_UP_EVENT;
	else
		type = NEMOSHOW_POINTER_BUTTON_UP_EVENT;

	event.type = type;
	event.x = tap->x;
	event.y = tap->y;
	event.time = time;
	event.value = button;
	event.device = device;
	event.serial = serial;
	event.tap = tap;

	nemoshow_dispatch_event(show, tap->one, &event);

	if (tap->one != NULL) {
		nemolist_remove(&tap->one_destroy_listener.link);
		nemolist_init(&tap->one_destroy_listener.link);

		tap->one = NULL;
	}
}

void nemoshow_push_pointer_motion_event(struct nemoshow *show, uint32_t serial, uint64_t device, uint32_t time, float x, float y)
{
	struct showevent event;
	struct showtap *tap;

	nemoshow_transform_from_viewport(show, x, y, &x, &y);

	tap = nemoshow_get_pointer_tap(show, device);
	if (tap == NULL)
		return;

	tap->x = x;
	tap->y = y;

	event.type = NEMOSHOW_POINTER_MOTION_EVENT;
	event.device = device;
	event.time = time;
	event.x = x;
	event.y = y;
	event.tap = tap;

	nemoshow_dispatch_event(show, tap->one, &event);
}

void nemoshow_push_pointer_axis_event(struct nemoshow *show, uint32_t serial, uint64_t device, uint32_t time, uint32_t axis, float value)
{
	struct showevent event;
	struct showtap *tap;

	tap = nemoshow_get_pointer_tap(show, device);
	if (tap == NULL)
		return;

	event.type = NEMOSHOW_POINTER_AXIS_EVENT;
	event.device = device;
	event.time = time;
	event.axis = axis;
	event.r = value;
	event.tap = tap;

	nemoshow_dispatch_event(show, tap->one, &event);
}

void nemoshow_push_keyboard_enter_event(struct nemoshow *show, uint32_t serial, uint64_t device)
{
	struct showevent event;

	event.type = NEMOSHOW_KEYBOARD_ENTER_EVENT;
	event.device = device;

	nemoshow_dispatch_event(show, show->keyboard.focus, &event);
}

void nemoshow_push_keyboard_leave_event(struct nemoshow *show, uint32_t serial, uint64_t device)
{
	struct showevent event;

	event.type = NEMOSHOW_KEYBOARD_LEAVE_EVENT;
	event.device = device;

	nemoshow_dispatch_event(show, show->keyboard.focus, &event);
}

void nemoshow_push_keyboard_down_event(struct nemoshow *show, uint32_t serial, uint64_t device, uint32_t time, uint32_t key)
{
	struct showevent event;

	event.type = NEMOSHOW_KEYBOARD_DOWN_EVENT;
	event.device = device;
	event.value = key;

	nemoshow_dispatch_event(show, show->keyboard.focus, &event);
}

void nemoshow_push_keyboard_up_event(struct nemoshow *show, uint32_t serial, uint64_t device, uint32_t time, uint32_t key)
{
	struct showevent event;

	event.type = NEMOSHOW_KEYBOARD_UP_EVENT;
	event.device = device;
	event.value = key;

	nemoshow_dispatch_event(show, show->keyboard.focus, &event);
}

void nemoshow_push_keyboard_layout_event(struct nemoshow *show, uint32_t serial, uint64_t device, const char *name)
{
	struct showevent event;

	event.type = NEMOSHOW_KEYBOARD_LAYOUT_EVENT;
	event.device = device;
	event.name = name;

	nemoshow_dispatch_event(show, show->keyboard.focus, &event);
}

void nemoshow_push_touch_down_event(struct nemoshow *show, uint32_t serial, uint64_t device, uint32_t time, float x, float y, float gx, float gy)
{
	struct showevent event;
	struct showtap *tap;
	uint64_t value;
	float sx, sy;

	nemoshow_transform_from_viewport(show, x, y, &x, &y);

	tap = nemoshow_create_tap(show, device);
	if (tap == NULL)
		return;

	nemolist_insert_tail(&show->tap_list, &tap->link);
	nemosignal_add(&show->destroy_signal, &tap->show_destroy_listener);

	tap->x = x;
	tap->y = y;
	tap->gx = gx;
	tap->gy = gy;
	tap->serial = serial;

	tap->one = nemoshow_pick_canvas(show, tap->x, tap->y, &sx, &sy);
	if (tap->one != NULL) {
		nemosignal_add(&tap->one->destroy_signal, &tap->one_destroy_listener);
	}

	tap->grab_time = time;
	tap->grab_x = x;
	tap->grab_y = y;
	tap->grab_gx = gx;
	tap->grab_gy = gy;

	event.type = NEMOSHOW_TOUCH_DOWN_EVENT;
	event.device = device;
	event.serial = serial;
	event.time = time;
	event.x = x;
	event.y = y;
	event.gx = gx;
	event.gy = gy;
	event.tap = tap;

	nemoshow_dispatch_event(show, tap->one, &event);
}

void nemoshow_push_touch_up_event(struct nemoshow *show, uint32_t serial, uint64_t device, uint32_t time)
{
	struct showevent event;
	struct showtap *tap;

	tap = nemoshow_get_touch_tap(show, device);
	if (tap == NULL)
		return;

	event.type = NEMOSHOW_TOUCH_UP_EVENT;
	event.x = tap->x;
	event.y = tap->y;
	event.gx = tap->gx;
	event.gy = tap->gy;
	event.device = device;
	event.serial = serial;
	event.time = time;
	event.tap = tap;

	nemoshow_remove_tap(tap);

	nemoshow_dispatch_event(show, tap->one, &event);

	nemoshow_destroy_tap(tap);
}

void nemoshow_push_touch_motion_event(struct nemoshow *show, uint32_t serial, uint64_t device, uint32_t time, float x, float y, float gx, float gy)
{
	struct showevent event;
	struct showtap *tap;

	nemoshow_transform_from_viewport(show, x, y, &x, &y);

	tap = nemoshow_get_touch_tap(show, device);
	if (tap == NULL)
		return;

	tap->dist += sqrtf((gx - tap->gx) * (gx - tap->gx) + (gy - tap->gy) * (gy - tap->gy));

	tap->x = x;
	tap->y = y;
	tap->gx = gx;
	tap->gy = gy;

	event.type = NEMOSHOW_TOUCH_MOTION_EVENT;
	event.device = device;
	event.serial = tap->serial;
	event.time = time;
	event.x = x;
	event.y = y;
	event.gx = gx;
	event.gy = gy;
	event.tap = tap;

	nemoshow_dispatch_event(show, tap->one, &event);
}

void nemoshow_push_touch_pressure_event(struct nemoshow *show, uint32_t serial, uint64_t device, uint32_t time, float p)
{
	struct showevent event;
	struct showtap *tap;

	tap = nemoshow_get_touch_tap(show, device);
	if (tap == NULL)
		return;

	event.type = NEMOSHOW_TOUCH_PRESSURE_EVENT;
	event.device = device;
	event.serial = tap->serial;
	event.time = time;
	event.p = p;
	event.tap = tap;

	nemoshow_dispatch_event(show, tap->one, &event);
}

int nemoshow_event_update_taps(struct nemoshow *show, struct showone *one, struct showevent *event)
{
	struct showtap *tap;
	int count = 0;

	if (one == NULL) {
		if (event->type & NEMOSHOW_POINTER_EVENT) {
			nemolist_for_each(tap, &show->ptap_list, link) {
				event->taps[count++] = tap;
			}
		} else if (event->type & NEMOSHOW_TOUCH_EVENT) {
			nemolist_for_each(tap, &show->tap_list, link) {
				event->taps[count++] = tap;
			}
		}
	} else {
		struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

		if (event->type & NEMOSHOW_POINTER_EVENT) {
			nemolist_for_each(tap, &show->ptap_list, link) {
				if (tap->one == one) {
					event->taps[count++] = tap;
				}
			}
		} else if (event->type & NEMOSHOW_TOUCH_EVENT) {
			nemolist_for_each(tap, &show->tap_list, link) {
				if (tap->one == one) {
					event->taps[count++] = tap;
				}
			}
		}
	}

	return (event->tapcount = count);
}

int nemoshow_event_update_taps_by_tag(struct nemoshow *show, struct showevent *event, uint32_t tag)
{
	struct showtap *tap;
	int count = 0;

	if (event->type & NEMOSHOW_POINTER_EVENT) {
		nemolist_for_each(tap, &show->ptap_list, link) {
			if (tap->tag == tag)
				event->taps[count++] = tap;
		}
	} else if (event->type & NEMOSHOW_TOUCH_EVENT) {
		nemolist_for_each(tap, &show->tap_list, link) {
			if (tap->tag == tag)
				event->taps[count++] = tap;
		}
	}

	return (event->tapcount = count);
}

void nemoshow_event_get_distant_tapindices(struct nemoshow *show, struct showevent *event, int *index0, int *index1)
{
	struct showtap *tap0, *tap1;
	float dm = 0.0f;
	float dd;
	float dx, dy;
	int i, j;

	for (i = 0; i < event->tapcount - 1; i++) {
		tap0 = event->taps[i];

		for (j = i + 1; j < event->tapcount; j++) {
			tap1 = event->taps[j];

			dx = tap1->x - tap0->x;
			dy = tap1->y - tap0->y;
			dd = sqrtf(dx * dx + dy * dy);

			if (dd > dm) {
				dm = dd;

				*index0 = i;
				*index1 = j;
			}
		}
	}
}

void nemoshow_event_get_distant_tapserials(struct nemoshow *show, struct showevent *event, uint32_t *serial0, uint32_t *serial1)
{
	int index0, index1;

	nemoshow_event_get_distant_tapindices(show, event, &index0, &index1);

	*serial0 = nemoshow_event_get_serial_on(event, index0);
	*serial1 = nemoshow_event_get_serial_on(event, index1);
}

void nemoshow_event_get_distant_tapdevices(struct nemoshow *show, struct showevent *event, uint64_t *device0, uint64_t *device1)
{
	int index0, index1;

	nemoshow_event_get_distant_tapindices(show, event, &index0, &index1);

	*device0 = nemoshow_event_get_device_on(event, index0);
	*device1 = nemoshow_event_get_device_on(event, index1);
}

static inline int nemoshow_event_is_pointer_single_click(struct nemoshow *show, struct showevent *event)
{
	struct showtap *tap = nemoshow_get_pointer_tap(show, event->device);

	if (tap->done != 0)
		return 0;

	if (tap != NULL && event->time - tap->grab_time < show->single_click_duration)
		return 1;

	return 0;
}

static inline int nemoshow_event_is_touch_single_click(struct nemoshow *show, struct showevent *event)
{
	struct showtap *tap = event->tap;

	if (tap->done != 0)
		return 0;

	if (tap != NULL && event->time - tap->grab_time < show->single_click_duration) {
		if (tap->dist <= show->single_click_distance)
			return 1;
	}

	return 0;
}

int nemoshow_event_is_single_click(struct nemoshow *show, struct showevent *event)
{
	if (event->type & NEMOSHOW_POINTER_UP_EVENT)
		return nemoshow_event_is_pointer_single_click(show, event);
	else if (event->type & NEMOSHOW_TOUCH_UP_EVENT)
		return nemoshow_event_is_touch_single_click(show, event);

	return 0;
}
