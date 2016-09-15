#ifndef	__NEMOTALE_EVENT_H__
#define	__NEMOTALE_EVENT_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>
#include <linux/input.h>

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
	NEMOTALE_KEYBOARD_LAYOUT_EVENT = (1 << 14),
	NEMOTALE_TOUCH_DOWN_EVENT = (1 << 15),
	NEMOTALE_TOUCH_UP_EVENT = (1 << 16),
	NEMOTALE_TOUCH_MOTION_EVENT = (1 << 17),
	NEMOTALE_TOUCH_PRESSURE_EVENT = (1 << 18),
	NEMOTALE_POINTER_LONG_PRESS_EVENT = (1 << 19),
	NEMOTALE_TOUCH_LONG_PRESS_EVENT = (1 << 20),
	NEMOTALE_CANCEL_EVENT = (1 << 21),
	NEMOTALE_DOWN_EVENT = NEMOTALE_POINTER_DOWN_EVENT | NEMOTALE_TOUCH_DOWN_EVENT,
	NEMOTALE_UP_EVENT = NEMOTALE_POINTER_UP_EVENT | NEMOTALE_TOUCH_UP_EVENT,
	NEMOTALE_MOTION_EVENT = NEMOTALE_POINTER_MOTION_EVENT | NEMOTALE_TOUCH_MOTION_EVENT,
	NEMOTALE_POINTER_EVENT = NEMOTALE_POINTER_ENTER_EVENT | NEMOTALE_POINTER_LEAVE_EVENT | NEMOTALE_POINTER_DOWN_EVENT | NEMOTALE_POINTER_UP_EVENT | NEMOTALE_POINTER_MOTION_EVENT,
	NEMOTALE_TOUCH_EVENT = NEMOTALE_TOUCH_DOWN_EVENT | NEMOTALE_TOUCH_UP_EVENT | NEMOTALE_TOUCH_MOTION_EVENT | NEMOTALE_TOUCH_PRESSURE_EVENT,
	NEMOTALE_LONG_PRESS_EVENT = NEMOTALE_POINTER_LONG_PRESS_EVENT | NEMOTALE_TOUCH_LONG_PRESS_EVENT
} NemoTaleEventType;

struct nemotale;
struct talenode;

struct taletap {
	struct talenode *node;

	int done;

	uint32_t tag;
	void *data;

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
	float p;

	const char *name;

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
extern void nemotale_push_keyboard_layout_event(struct nemotale *tale, uint32_t serial, uint64_t device, const char *name);
extern void nemotale_push_touch_down_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, float x, float y, float gx, float gy);
extern void nemotale_push_touch_up_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time);
extern void nemotale_push_touch_motion_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, float x, float y, float gx, float gy);
extern void nemotale_push_touch_pressure_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, float p);

extern void nemotale_push_timer_event(struct nemotale *tale, uint32_t time);

static inline void nemotale_tap_set_tag(struct taletap *tap, uint32_t tag)
{
	tap->tag = tag;
}

static inline uint32_t nemotale_tap_get_tag(struct taletap *tap)
{
	return tap->tag;
}

static inline void nemotale_tap_set_data(struct taletap *tap, void *data)
{
	tap->data = data;
}

static inline void *nemotale_tap_get_data(struct taletap *tap)
{
	return tap->data;
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

static inline float nemotale_event_get_z(struct taleevent *event)
{
	return event->z;
}

static inline float nemotale_event_get_gx(struct taleevent *event)
{
	return event->gx;
}

static inline float nemotale_event_get_gy(struct taleevent *event)
{
	return event->gy;
}

static inline float nemotale_event_get_x_on(struct taleevent *event, int index)
{
	return event->taps[index]->x;
}

static inline float nemotale_event_get_y_on(struct taleevent *event, int index)
{
	return event->taps[index]->y;
}

static inline float nemotale_event_get_gx_on(struct taleevent *event, int index)
{
	return event->taps[index]->gx;
}

static inline float nemotale_event_get_gy_on(struct taleevent *event, int index)
{
	return event->taps[index]->gy;
}

static inline float nemotale_event_get_rotate(struct taleevent *event)
{
	return event->r;
}

static inline float nemotale_event_get_pressure(struct taleevent *event)
{
	return event->p;
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

static inline uint32_t nemotale_event_get_tag(struct taleevent *event)
{
	return nemotale_tap_get_tag(event->tap);
}

static inline void nemotale_event_set_data(struct taleevent *event, void *data)
{
	nemotale_tap_set_data(event->tap, data);
}

static inline void *nemotale_event_get_data(struct taleevent *event)
{
	return nemotale_tap_get_data(event->tap);
}

static inline const char *nemotale_event_get_name(struct taleevent *event)
{
	return event->name;
}

static inline void nemotale_event_set_done(struct taleevent *event)
{
	event->tap->done = 1;
}

static inline void nemotale_event_set_done_on(struct taleevent *event, int index)
{
	event->taps[index]->done = 1;
}

static inline void nemotale_event_set_done_all(struct taleevent *event)
{
	int i;

	for (i = 0; i < event->tapcount; i++) {
		event->taps[i]->done = 1;
	}
}

static inline int nemotale_event_is_done(struct taleevent *event)
{
	return event->tap->done;
}

static inline void nemotale_event_set_cancel(struct taleevent *event)
{
	event->type = NEMOTALE_CANCEL_EVENT;
}

static inline int nemotale_event_get_tapcount(struct taleevent *event)
{
	return event->tapcount;
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

static inline int nemotale_event_update_taps_by_node(struct nemotale *tale, struct talenode *node, struct taleevent *event)
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

static inline void nemotale_event_get_distant_tapindices(struct nemotale *tale, struct taleevent *event, int *index0, int *index1)
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

				*index0 = i;
				*index1 = j;
			}
		}
	}
}

static inline void nemotale_event_get_distant_tapserials(struct nemotale *tale, struct taleevent *event, uint32_t *serial0, uint32_t *serial1)
{
	int index0, index1;

	nemotale_event_get_distant_tapindices(tale, event, &index0, &index1);

	*serial0 = nemotale_event_get_serial_on(event, index0);
	*serial1 = nemotale_event_get_serial_on(event, index1);
}

static inline void nemotale_event_get_distant_tapdevices(struct nemotale *tale, struct taleevent *event, uint64_t *device0, uint64_t *device1)
{
	int index0, index1;

	nemotale_event_get_distant_tapindices(tale, event, &index0, &index1);

	*device0 = nemotale_event_get_device_on(event, index0);
	*device1 = nemotale_event_get_device_on(event, index1);
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

static inline void nemotale_event_transform_from_viewport(struct nemotale *tale, float sx, float sy, float *x, float *y)
{
	if (tale->viewport.enable != 0) {
		*x = sx * tale->viewport.rx;
		*y = sy * tale->viewport.ry;
	} else {
		*x = sx;
		*y = sy;
	}
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
