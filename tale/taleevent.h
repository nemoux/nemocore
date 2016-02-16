#ifndef	__NEMOTALE_EVENT_H__
#define	__NEMOTALE_EVENT_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemolist.h>
#include <nemolistener.h>

#include <nemotale.h>
#include <talenode.h>

#define	NEMOTALE_EVENT_TAPS_MAX			(64)

typedef enum {
	NEMOTALE_POINTER_ENTER_EVENT = (1 << 0),
	NEMOTALE_POINTER_LEAVE_EVENT = (1 << 1),
	NEMOTALE_POINTER_LEFT_DOWN_EVENT = (1 << 2),
	NEMOTALE_POINTER_RIGHT_DOWN_EVENT = (1 << 3),
	NEMOTALE_POINTER_BUTTON_DOWN_EVENT = (1 << 4),
	NEMOTALE_POINTER_DOWN_EVENT = NEMOTALE_POINTER_LEFT_DOWN_EVENT | NEMOTALE_POINTER_RIGHT_DOWN_EVENT | NEMOTALE_POINTER_BUTTON_DOWN_EVENT,
	NEMOTALE_POINTER_LEFT_UP_EVENT = (1 << 5),
	NEMOTALE_POINTER_RIGHT_UP_EVENT = (1 << 6),
	NEMOTALE_POINTER_BUTTON_UP_EVENT = (1 << 7),
	NEMOTALE_POINTER_UP_EVENT = NEMOTALE_POINTER_LEFT_UP_EVENT | NEMOTALE_POINTER_RIGHT_UP_EVENT | NEMOTALE_POINTER_BUTTON_UP_EVENT,
	NEMOTALE_POINTER_MOTION_EVENT = (1 << 8),
	NEMOTALE_POINTER_AXIS_EVENT = (1 << 9),
	NEMOTALE_KEYBOARD_ENTER_EVENT = (1 << 10),
	NEMOTALE_KEYBOARD_LEAVE_EVENT = (1 << 11),
	NEMOTALE_KEYBOARD_DOWN_EVENT = (1 << 12),
	NEMOTALE_KEYBOARD_UP_EVENT = (1 << 13),
	NEMOTALE_TOUCH_DOWN_EVENT = (1 << 14),
	NEMOTALE_TOUCH_UP_EVENT = (1 << 15),
	NEMOTALE_TOUCH_MOTION_EVENT = (1 << 16),
	NEMOTALE_STICK_ENTER_EVENT = (1 << 17),
	NEMOTALE_STICK_LEAVE_EVENT = (1 << 18),
	NEMOTALE_STICK_TRANSLATE_EVENT = (1 << 19),
	NEMOTALE_STICK_ROTATE_EVENT = (1 << 20),
	NEMOTALE_STICK_BUTTON_DOWN_EVENT = (1 << 21),
	NEMOTALE_STICK_BUTTON_UP_EVENT = (1 << 22),
	NEMOTALE_POINTER_LONG_PRESS_EVENT = (1 << 23),
	NEMOTALE_TOUCH_LONG_PRESS_EVENT = (1 << 24),
	NEMOTALE_DOWN_EVENT = NEMOTALE_POINTER_DOWN_EVENT | NEMOTALE_TOUCH_DOWN_EVENT,
	NEMOTALE_UP_EVENT = NEMOTALE_POINTER_UP_EVENT | NEMOTALE_TOUCH_UP_EVENT,
	NEMOTALE_MOTION_EVENT = NEMOTALE_POINTER_MOTION_EVENT | NEMOTALE_TOUCH_MOTION_EVENT,
	NEMOTALE_POINTER_EVENT = NEMOTALE_POINTER_ENTER_EVENT | NEMOTALE_POINTER_LEAVE_EVENT | NEMOTALE_POINTER_DOWN_EVENT | NEMOTALE_POINTER_UP_EVENT | NEMOTALE_POINTER_MOTION_EVENT,
	NEMOTALE_TOUCH_EVENT = NEMOTALE_TOUCH_DOWN_EVENT | NEMOTALE_TOUCH_UP_EVENT | NEMOTALE_TOUCH_MOTION_EVENT,
	NEMOTALE_LONG_PRESS_EVENT = NEMOTALE_POINTER_LONG_PRESS_EVENT | NEMOTALE_TOUCH_LONG_PRESS_EVENT
} NemoTaleEventType;

typedef enum {
	NEMOTALE_TAP_USED_STATE = (1 << 0)
} NemoTaleTapState;

struct nemotale;
struct talenode;

struct taletap {
	struct talenode *node;

	uint32_t state;

	uint32_t tag;

	float x, y;
	float gx, gy;
	float dist;

	float grab_x, grab_y;
	float grab_gx, grab_gy;
	uint32_t grab_time;
	uint32_t grab_value;

	uint32_t serial;
	uint64_t device;

	struct nemolist link;
	struct nemolistener tale_destroy_listener;
	struct nemolistener node_destroy_listener;
};

struct taleevent {
	uint32_t type;

	uint64_t device;

	uint32_t serial;

	uint32_t time;
	uint32_t value;

	uint32_t duration;

	float x, y, z;
	float gx, gy;

	uint32_t axis;
	float r;

	struct taletap *tap;

	struct taletap *taps[NEMOTALE_EVENT_TAPS_MAX];
	int tapcount;
};

extern void nemotale_push_pointer_enter_event(struct nemotale *tale, uint32_t serial, uint64_t device, float x, float y);
extern void nemotale_push_pointer_leave_event(struct nemotale *tale, uint32_t serial, uint64_t device);
extern void nemotale_push_pointer_down_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, uint32_t button);
extern void nemotale_push_pointer_up_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, uint32_t button);
extern void nemotale_push_pointer_motion_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, float x, float y);
extern void nemotale_push_pointer_axis_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, uint32_t axis, float value);
extern void nemotale_push_keyboard_enter_event(struct nemotale *tale, uint32_t serial, uint64_t device);
extern void nemotale_push_keyboard_leave_event(struct nemotale *tale, uint32_t serial, uint64_t device);
extern void nemotale_push_keyboard_down_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, uint32_t key);
extern void nemotale_push_keyboard_up_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, uint32_t key);
extern void nemotale_push_touch_down_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, float x, float y, float gx, float gy);
extern void nemotale_push_touch_up_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time);
extern void nemotale_push_touch_motion_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, float x, float y, float gx, float gy);
extern void nemotale_push_stick_enter_event(struct nemotale *tale, uint32_t serial, uint64_t device);
extern void nemotale_push_stick_leave_event(struct nemotale *tale, uint32_t serial, uint64_t device);
extern void nemotale_push_stick_down_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, uint32_t button);
extern void nemotale_push_stick_up_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, uint32_t button);
extern void nemotale_push_stick_translate_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, float x, float y, float z);
extern void nemotale_push_stick_rotate_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, float x, float y, float z);

extern void nemotale_push_timer_event(struct nemotale *tale, uint32_t time);

static inline void nemotale_tap_set_state(struct taletap *tap, uint32_t state)
{
	tap->state |= state;
}

static inline void nemotale_tap_put_state(struct taletap *tap, uint32_t state)
{
	tap->state &= ~state;
}

static inline int nemotale_tap_has_state(struct taletap *tap, uint32_t state)
{
	return tap->state & state;
}

static inline void nemotale_tap_set_tag(struct taletap *tap, uint32_t tag)
{
	tap->tag = tag;
}

static inline uint32_t nemotale_tap_get_tag(struct taletap *tap)
{
	return tap->tag;
}

static inline struct taletap *nemotale_pointer_get_tap(struct nemotale *tale, uint64_t device)
{
	struct taletap *tap;

	nemolist_for_each(tap, &tale->ptap_list, link) {
		if (tap->device == device)
			return tap;
	}

	return NULL;
}

static inline struct taletap *nemotale_touch_get_tap(struct nemotale *tale, uint64_t device)
{
	struct taletap *tap;

	nemolist_for_each_reverse(tap, &tale->tap_list, link) {
		if (tap->device == device)
			return tap;
	}

	return NULL;
}

static inline float nemotale_event_get_x(struct taleevent *event)
{
	return event->x;
}

static inline float nemotale_event_get_y(struct taleevent *event)
{
	return event->y;
}

static inline float nemotale_event_get_gx(struct taleevent *event)
{
	return event->gx;
}

static inline float nemotale_event_get_gy(struct taleevent *event)
{
	return event->gy;
}

static inline float nemotale_event_get_r(struct taleevent *event)
{
	return event->r;
}

static inline uint32_t nemotale_event_get_axis(struct taleevent *event)
{
	return event->axis;
}

static inline uint32_t nemotale_event_get_serial(struct taleevent *event)
{
	return event->serial;
}

static inline uint32_t nemotale_event_get_serial_on(struct taleevent *event, int index)
{
	return event->taps[index]->serial;
}

static inline uint32_t nemotale_event_get_time(struct taleevent *event)
{
	return event->time;
}

static inline uint32_t nemotale_event_get_value(struct taleevent *event)
{
	return event->value;
}

static inline uint32_t nemotale_event_get_duration(struct taleevent *event)
{
	return event->duration;
}

static inline uint64_t nemotale_event_get_device(struct taleevent *event)
{
	return event->device;
}

static inline uint64_t nemotale_event_get_device_on(struct taleevent *event, int index)
{
	return event->taps[index]->device;
}

static inline float nemotale_event_get_grab_x(struct taleevent *event)
{
	return event->tap->grab_x;
}

static inline float nemotale_event_get_grab_y(struct taleevent *event)
{
	return event->tap->grab_y;
}

static inline float nemotale_event_get_grab_gx(struct taleevent *event)
{
	return event->tap->grab_gx;
}

static inline float nemotale_event_get_grab_gy(struct taleevent *event)
{
	return event->tap->grab_gy;
}

static inline float nemotale_event_get_grab_x_on(struct taleevent *event, int index)
{
	return event->taps[index]->grab_x;
}

static inline float nemotale_event_get_grab_y_on(struct taleevent *event, int index)
{
	return event->taps[index]->grab_y;
}

static inline float nemotale_event_get_grab_gx_on(struct taleevent *event, int index)
{
	return event->taps[index]->grab_gx;
}

static inline float nemotale_event_get_grab_gy_on(struct taleevent *event, int index)
{
	return event->taps[index]->grab_gy;
}

static inline uint32_t nemotale_event_get_grab_time(struct taleevent *event)
{
	return event->tap->grab_time;
}

static inline uint32_t nemotale_event_get_grab_time_on(struct taleevent *event, int index)
{
	return event->taps[index]->grab_time;
}

static inline void nemotale_event_set_tag(struct taleevent *event, uint32_t tag)
{
	nemotale_tap_set_tag(event->tap, tag);
}

static inline void nemotale_event_set_used(struct taleevent *event)
{
	nemotale_tap_set_state(event->tap, NEMOTALE_TAP_USED_STATE);
}

static inline int nemotale_event_update_taps(struct nemotale *tale, struct taleevent *event)
{
	struct taletap *tap;
	int count = 0;

	if (event->type & NEMOTALE_POINTER_EVENT) {
		nemolist_for_each(tap, &tale->ptap_list, link) {
			event->taps[count++] = tap;
		}
	} else if (event->type & NEMOTALE_TOUCH_EVENT) {
		nemolist_for_each(tap, &tale->tap_list, link) {
			event->taps[count++] = tap;
		}
	}

	return (event->tapcount = count);
}

static inline int nemotale_event_update_node_taps(struct nemotale *tale, struct talenode *node, struct taleevent *event)
{
	struct taletap *tap;
	int count = 0;

	if (event->type & NEMOTALE_POINTER_EVENT) {
		nemolist_for_each(tap, &tale->ptap_list, link) {
			if (tap->node == node) {
				event->taps[count++] = tap;
			}
		}
	} else if (event->type & NEMOTALE_TOUCH_EVENT) {
		nemolist_for_each(tap, &tale->tap_list, link) {
			if (tap->node == node) {
				event->taps[count++] = tap;
			}
		}
	}

	return (event->tapcount = count);
}

static inline int nemotale_event_update_taps_by_tag(struct nemotale *tale, struct taleevent *event, uint32_t tag)
{
	struct taletap *tap;
	int count = 0;

	if (event->type & NEMOTALE_POINTER_EVENT) {
		nemolist_for_each(tap, &tale->ptap_list, link) {
			if (tap->tag == tag)
				event->taps[count++] = tap;
		}
	} else if (event->type & NEMOTALE_TOUCH_EVENT) {
		nemolist_for_each(tap, &tale->tap_list, link) {
			if (tap->tag == tag)
				event->taps[count++] = tap;
		}
	}

	return (event->tapcount = count);
}

static inline void nemotale_event_get_distant_taps_devices(struct nemotale *tale, struct taleevent *event, uint64_t *device0, uint64_t *device1)
{
	struct taletap *tap0, *tap1;
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

				*device0 = tap0->device;
				*device1 = tap1->device;
			}
		}
	}
}

static inline void nemotale_event_get_distant_taps_serials(struct nemotale *tale, struct taleevent *event, uint32_t *serial0, uint32_t *serial1)
{
	struct taletap *tap0, *tap1;
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

				*serial0 = tap0->serial;
				*serial1 = tap1->serial;
			}
		}
	}
}

static inline void nemotale_event_transform_to_viewport(struct nemotale *tale, float x, float y, float *sx, float *sy)
{
	if (tale->viewport.enable != 0) {
		*sx = x * tale->viewport.sx;
		*sy = y * tale->viewport.sy;
	} else {
		*sx = x;
		*sy = y;
	}
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
