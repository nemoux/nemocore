#ifndef	__NEMOTALE_EVENT_H__
#define	__NEMOTALE_EVENT_H__

#include <stdint.h>

#include <nemolist.h>
#include <nemolistener.h>

#include <nemotale.h>
#include <talenode.h>

#define	NEMOTALE_EVENT_TAPS_MAX			(16)

typedef enum {
	NEMOTALE_POINTER_ENTER_EVENT = (1 << 0),
	NEMOTALE_POINTER_LEAVE_EVENT = (1 << 1),
	NEMOTALE_POINTER_LEFT_DOWN_EVENT = (1 << 2),
	NEMOTALE_POINTER_RIGHT_DOWN_EVENT = (1 << 3),
	NEMOTALE_POINTER_MIDDLE_DOWN_EVENT = (1 << 4),
	NEMOTALE_POINTER_DOWN_EVENT = NEMOTALE_POINTER_LEFT_DOWN_EVENT | NEMOTALE_POINTER_RIGHT_DOWN_EVENT | NEMOTALE_POINTER_MIDDLE_DOWN_EVENT,
	NEMOTALE_POINTER_LEFT_UP_EVENT = (1 << 5),
	NEMOTALE_POINTER_RIGHT_UP_EVENT = (1 << 6),
	NEMOTALE_POINTER_MIDDLE_UP_EVENT = (1 << 7),
	NEMOTALE_POINTER_UP_EVENT = NEMOTALE_POINTER_LEFT_UP_EVENT | NEMOTALE_POINTER_RIGHT_UP_EVENT | NEMOTALE_POINTER_MIDDLE_UP_EVENT,
	NEMOTALE_POINTER_MOTION_EVENT = (1 << 8),
	NEMOTALE_KEYBOARD_ENTER_EVENT = (1 << 9),
	NEMOTALE_KEYBOARD_LEAVE_EVENT = (1 << 10),
	NEMOTALE_KEYBOARD_DOWN_EVENT = (1 << 11),
	NEMOTALE_KEYBOARD_UP_EVENT = (1 << 12),
	NEMOTALE_TOUCH_DOWN_EVENT = (1 << 13),
	NEMOTALE_TOUCH_UP_EVENT = (1 << 14),
	NEMOTALE_TOUCH_MOTION_EVENT = (1 << 15),
	NEMOTALE_DOWN_EVENT = NEMOTALE_POINTER_DOWN_EVENT | NEMOTALE_TOUCH_DOWN_EVENT,
	NEMOTALE_UP_EVENT = NEMOTALE_POINTER_UP_EVENT | NEMOTALE_TOUCH_UP_EVENT,
	NEMOTALE_MOTION_EVENT = NEMOTALE_POINTER_MOTION_EVENT | NEMOTALE_TOUCH_MOTION_EVENT,
	NEMOTALE_POINTER_EVENT = NEMOTALE_POINTER_ENTER_EVENT | NEMOTALE_POINTER_LEAVE_EVENT | NEMOTALE_POINTER_DOWN_EVENT | NEMOTALE_POINTER_UP_EVENT | NEMOTALE_POINTER_MOTION_EVENT,
	NEMOTALE_TOUCH_EVENT = NEMOTALE_TOUCH_DOWN_EVENT | NEMOTALE_TOUCH_UP_EVENT | NEMOTALE_TOUCH_MOTION_EVENT
} NemoTaleEventType;

struct nemotale;
struct talenode;

struct taletap {
	struct talenode *node;

	float x, y;

	float grab_x, grab_y;
	uint32_t grab_time;
	uint32_t grab_value;

	uint32_t serial;
	uint64_t device;

	struct nemolist link;
	struct nemolistener tale_destroy_listener;
	struct nemolistener node_destroy_listener;
};

struct taleevent {
	uint64_t device;

	uint32_t serial;

	uint32_t time;
	uint32_t value;

	float x, y;
	float dx, dy;

	struct taletap *taps[NEMOTALE_EVENT_TAPS_MAX];
	int tapcount;
};

extern void nemotale_push_pointer_enter_event(struct nemotale *tale, uint32_t serial, uint64_t device, float x, float y);
extern void nemotale_push_pointer_leave_event(struct nemotale *tale, uint32_t serial, uint64_t device);
extern void nemotale_push_pointer_down_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, uint32_t button);
extern void nemotale_push_pointer_up_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, uint32_t button);
extern void nemotale_push_pointer_motion_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, float x, float y);
extern void nemotale_push_keyboard_enter_event(struct nemotale *tale, uint32_t serial, uint64_t device);
extern void nemotale_push_keyboard_leave_event(struct nemotale *tale, uint32_t serial, uint64_t device);
extern void nemotale_push_keyboard_down_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, uint32_t key);
extern void nemotale_push_keyboard_up_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, uint32_t key);
extern void nemotale_push_touch_down_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, float x, float y);
extern void nemotale_push_touch_up_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, float dx, float dy);
extern void nemotale_push_touch_motion_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, float x, float y);

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

static inline int nemotale_event_update_taps(struct nemotale *tale, struct taleevent *event, uint32_t type)
{
	struct taletap *tap;
	int count = 0;

	if (type & NEMOTALE_POINTER_EVENT) {
		nemolist_for_each(tap, &tale->ptap_list, link) {
			event->taps[count++] = tap;
		}
	} else if (type & NEMOTALE_TOUCH_EVENT) {
		nemolist_for_each(tap, &tale->tap_list, link) {
			event->taps[count++] = tap;
		}
	}

	return event->tapcount = count;
}

static inline int nemotale_event_update_node_taps(struct nemotale *tale, struct talenode *node, struct taleevent *event, uint32_t type)
{
	struct taletap *tap;
	int count = 0;

	if (type & NEMOTALE_POINTER_EVENT) {
		nemolist_for_each(tap, &tale->ptap_list, link) {
			if (tap->node == node) {
				event->taps[count++] = tap;
			}
		}
	} else if (type & NEMOTALE_TOUCH_EVENT) {
		nemolist_for_each(tap, &tale->tap_list, link) {
			if (tap->node == node) {
				event->taps[count++] = tap;
			}
		}
	}

	return event->tapcount = count;
}

#endif
