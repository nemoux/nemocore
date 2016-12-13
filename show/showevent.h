#ifndef __NEMOSHOW_EVENT_H__
#define __NEMOSHOW_EVENT_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>
#include <linux/input.h>

#include <nemolist.h>
#include <nemolistener.h>

#define NEMOSHOW_EVENT_TAPS_MAX			(64)

typedef enum {
	NEMOSHOW_POINTER_ENTER_EVENT = (1 << 0),
	NEMOSHOW_POINTER_LEAVE_EVENT = (1 << 1),
	NEMOSHOW_POINTER_LEFT_DOWN_EVENT = (1 << 2),
	NEMOSHOW_POINTER_RIGHT_DOWN_EVENT = (1 << 3),
	NEMOSHOW_POINTER_BUTTON_DOWN_EVENT = (1 << 4),
	NEMOSHOW_POINTER_DOWN_EVENT = NEMOSHOW_POINTER_LEFT_DOWN_EVENT | NEMOSHOW_POINTER_RIGHT_DOWN_EVENT | NEMOSHOW_POINTER_BUTTON_DOWN_EVENT,
	NEMOSHOW_POINTER_LEFT_UP_EVENT = (1 << 5),
	NEMOSHOW_POINTER_RIGHT_UP_EVENT = (1 << 6),
	NEMOSHOW_POINTER_BUTTON_UP_EVENT = (1 << 7),
	NEMOSHOW_POINTER_UP_EVENT = NEMOSHOW_POINTER_LEFT_UP_EVENT | NEMOSHOW_POINTER_RIGHT_UP_EVENT | NEMOSHOW_POINTER_BUTTON_UP_EVENT,
	NEMOSHOW_POINTER_MOTION_EVENT = (1 << 8),
	NEMOSHOW_POINTER_AXIS_EVENT = (1 << 9),
	NEMOSHOW_KEYBOARD_ENTER_EVENT = (1 << 10),
	NEMOSHOW_KEYBOARD_LEAVE_EVENT = (1 << 11),
	NEMOSHOW_KEYBOARD_DOWN_EVENT = (1 << 12),
	NEMOSHOW_KEYBOARD_UP_EVENT = (1 << 13),
	NEMOSHOW_KEYBOARD_LAYOUT_EVENT = (1 << 14),
	NEMOSHOW_TOUCH_DOWN_EVENT = (1 << 15),
	NEMOSHOW_TOUCH_UP_EVENT = (1 << 16),
	NEMOSHOW_TOUCH_MOTION_EVENT = (1 << 17),
	NEMOSHOW_TOUCH_PRESSURE_EVENT = (1 << 18),
	NEMOSHOW_CANCEL_EVENT = (1 << 19),
	NEMOSHOW_DOWN_EVENT = NEMOSHOW_POINTER_DOWN_EVENT | NEMOSHOW_TOUCH_DOWN_EVENT,
	NEMOSHOW_UP_EVENT = NEMOSHOW_POINTER_UP_EVENT | NEMOSHOW_TOUCH_UP_EVENT,
	NEMOSHOW_MOTION_EVENT = NEMOSHOW_POINTER_MOTION_EVENT | NEMOSHOW_TOUCH_MOTION_EVENT,
	NEMOSHOW_POINTER_EVENT = NEMOSHOW_POINTER_ENTER_EVENT | NEMOSHOW_POINTER_LEAVE_EVENT | NEMOSHOW_POINTER_DOWN_EVENT | NEMOSHOW_POINTER_UP_EVENT | NEMOSHOW_POINTER_MOTION_EVENT,
	NEMOSHOW_TOUCH_EVENT = NEMOSHOW_TOUCH_DOWN_EVENT | NEMOSHOW_TOUCH_UP_EVENT | NEMOSHOW_TOUCH_MOTION_EVENT | NEMOSHOW_TOUCH_PRESSURE_EVENT,
} NemoShowEventType;

struct nemoshow;
struct showone;
struct showcanvas;

struct showtap {
	struct showone *one;

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
	struct nemolistener show_destroy_listener;
	struct nemolistener one_destroy_listener;
};

struct showevent {
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

	struct showtap *tap;

	struct showtap *taps[NEMOSHOW_EVENT_TAPS_MAX];
	int tapcount;
};

extern void nemoshow_push_pointer_enter_event(struct nemoshow *show, uint32_t serial, uint64_t device, float x, float y);
extern void nemoshow_push_pointer_leave_event(struct nemoshow *show, uint32_t serial, uint64_t device);
extern void nemoshow_push_pointer_down_event(struct nemoshow *show, uint32_t serial, uint64_t device, uint32_t time, uint32_t button);
extern void nemoshow_push_pointer_up_event(struct nemoshow *show, uint32_t serial, uint64_t device, uint32_t time, uint32_t button);
extern void nemoshow_push_pointer_motion_event(struct nemoshow *show, uint32_t serial, uint64_t device, uint32_t time, float x, float y);
extern void nemoshow_push_pointer_axis_event(struct nemoshow *show, uint32_t serial, uint64_t device, uint32_t time, uint32_t axis, float value);
extern void nemoshow_push_keyboard_enter_event(struct nemoshow *show, uint32_t serial, uint64_t device);
extern void nemoshow_push_keyboard_leave_event(struct nemoshow *show, uint32_t serial, uint64_t device);
extern void nemoshow_push_keyboard_down_event(struct nemoshow *show, uint32_t serial, uint64_t device, uint32_t time, uint32_t key);
extern void nemoshow_push_keyboard_up_event(struct nemoshow *show, uint32_t serial, uint64_t device, uint32_t time, uint32_t key);
extern void nemoshow_push_keyboard_layout_event(struct nemoshow *show, uint32_t serial, uint64_t device, const char *name);
extern void nemoshow_push_touch_down_event(struct nemoshow *show, uint32_t serial, uint64_t device, uint32_t time, float x, float y, float gx, float gy);
extern void nemoshow_push_touch_up_event(struct nemoshow *show, uint32_t serial, uint64_t device, uint32_t time);
extern void nemoshow_push_touch_motion_event(struct nemoshow *show, uint32_t serial, uint64_t device, uint32_t time, float x, float y, float gx, float gy);
extern void nemoshow_push_touch_pressure_event(struct nemoshow *show, uint32_t serial, uint64_t device, uint32_t time, float p);

extern int nemoshow_event_update_taps(struct nemoshow *show, struct showone *one, struct showevent *event);
extern int nemoshow_event_update_taps_by_tag(struct nemoshow *show, struct showevent *event, uint32_t tag);
extern void nemoshow_event_get_distant_tapindices(struct nemoshow *show, struct showevent *event, int *index0, int *index1);
extern void nemoshow_event_get_distant_tapserials(struct nemoshow *show, struct showevent *event, uint32_t *serial0, uint32_t *serial1);
extern void nemoshow_event_get_distant_tapdevices(struct nemoshow *show, struct showevent *event, uint64_t *device0, uint64_t *device1);

extern int nemoshow_event_is_single_click(struct nemoshow *show, struct showevent *event);

static inline void nemoshow_tap_set_tag(struct showtap *tap, uint32_t tag)
{
	tap->tag = tag;
}

static inline uint32_t nemoshow_tap_get_tag(struct showtap *tap)
{
	return tap->tag;
}

static inline void nemoshow_tap_set_data(struct showtap *tap, void *data)
{
	tap->data = data;
}

static inline void *nemoshow_tap_get_data(struct showtap *tap)
{
	return tap->data;
}

static inline float nemoshow_event_get_x(struct showevent *event)
{
	return event->x;
}

static inline float nemoshow_event_get_y(struct showevent *event)
{
	return event->y;
}

static inline float nemoshow_event_get_z(struct showevent *event)
{
	return event->z;
}

static inline float nemoshow_event_get_gx(struct showevent *event)
{
	return event->gx;
}

static inline float nemoshow_event_get_gy(struct showevent *event)
{
	return event->gy;
}

static inline float nemoshow_event_get_x_on(struct showevent *event, int index)
{
	return event->taps[index]->x;
}

static inline float nemoshow_event_get_y_on(struct showevent *event, int index)
{
	return event->taps[index]->y;
}

static inline float nemoshow_event_get_gx_on(struct showevent *event, int index)
{
	return event->taps[index]->gx;
}

static inline float nemoshow_event_get_gy_on(struct showevent *event, int index)
{
	return event->taps[index]->gy;
}

static inline float nemoshow_event_get_rotate(struct showevent *event)
{
	return event->r;
}

static inline float nemoshow_event_get_pressure(struct showevent *event)
{
	return event->p;
}

static inline uint32_t nemoshow_event_get_axis(struct showevent *event)
{
	return event->axis;
}

static inline uint32_t nemoshow_event_get_serial(struct showevent *event)
{
	return event->serial;
}

static inline uint32_t nemoshow_event_get_serial_on(struct showevent *event, int index)
{
	return event->taps[index]->serial;
}

static inline uint32_t nemoshow_event_get_time(struct showevent *event)
{
	return event->time;
}

static inline uint32_t nemoshow_event_get_value(struct showevent *event)
{
	return event->value;
}

static inline uint32_t nemoshow_event_get_duration(struct showevent *event)
{
	return event->duration;
}

static inline uint64_t nemoshow_event_get_device(struct showevent *event)
{
	return event->device;
}

static inline uint64_t nemoshow_event_get_device_on(struct showevent *event, int index)
{
	return event->taps[index]->device;
}

static inline float nemoshow_event_get_grab_x(struct showevent *event)
{
	return event->tap->grab_x;
}

static inline float nemoshow_event_get_grab_y(struct showevent *event)
{
	return event->tap->grab_y;
}

static inline float nemoshow_event_get_grab_gx(struct showevent *event)
{
	return event->tap->grab_gx;
}

static inline float nemoshow_event_get_grab_gy(struct showevent *event)
{
	return event->tap->grab_gy;
}

static inline float nemoshow_event_get_grab_x_on(struct showevent *event, int index)
{
	return event->taps[index]->grab_x;
}

static inline float nemoshow_event_get_grab_y_on(struct showevent *event, int index)
{
	return event->taps[index]->grab_y;
}

static inline float nemoshow_event_get_grab_gx_on(struct showevent *event, int index)
{
	return event->taps[index]->grab_gx;
}

static inline float nemoshow_event_get_grab_gy_on(struct showevent *event, int index)
{
	return event->taps[index]->grab_gy;
}

static inline uint32_t nemoshow_event_get_grab_time(struct showevent *event)
{
	return event->tap->grab_time;
}

static inline uint32_t nemoshow_event_get_grab_time_on(struct showevent *event, int index)
{
	return event->taps[index]->grab_time;
}

static inline void nemoshow_event_set_tag(struct showevent *event, uint32_t tag)
{
	nemoshow_tap_set_tag(event->tap, tag);
}

static inline uint32_t nemoshow_event_get_tag(struct showevent *event)
{
	return nemoshow_tap_get_tag(event->tap);
}

static inline void nemoshow_event_set_data(struct showevent *event, void *data)
{
	nemoshow_tap_set_data(event->tap, data);
}

static inline void *nemoshow_event_get_data(struct showevent *event)
{
	return nemoshow_tap_get_data(event->tap);
}

static inline const char *nemoshow_event_get_name(struct showevent *event)
{
	return event->name;
}

static inline void nemoshow_event_set_done(struct showevent *event)
{
	event->tap->done = 1;
}

static inline void nemoshow_event_set_done_on(struct showevent *event, int index)
{
	event->taps[index]->done = 1;
}

static inline void nemoshow_event_set_done_all(struct showevent *event)
{
	int i;

	for (i = 0; i < event->tapcount; i++) {
		event->taps[i]->done = 1;
	}
}

static inline int nemoshow_event_is_done(struct showevent *event)
{
	return event->tap->done;
}

static inline void nemoshow_event_set_type(struct showevent *event, uint32_t type)
{
	event->type = type;
}

static inline void nemoshow_event_set_cancel(struct showevent *event)
{
	event->type = NEMOSHOW_CANCEL_EVENT;
}

static inline int nemoshow_event_get_tapcount(struct showevent *event)
{
	return event->tapcount;
}

static inline int nemoshow_event_is_no_tap(struct nemoshow *show, struct showevent *event)
{
	return event->tapcount == 0;
}

static inline int nemoshow_event_is_single_tap(struct nemoshow *show, struct showevent *event)
{
	return event->tapcount == 1;
}

static inline int nemoshow_event_is_double_taps(struct nemoshow *show, struct showevent *event)
{
	return event->tapcount == 2;
}

static inline int nemoshow_event_is_triple_taps(struct nemoshow *show, struct showevent *event)
{
	return event->tapcount == 3;
}

static inline int nemoshow_event_is_many_taps(struct nemoshow *show, struct showevent *event)
{
	return event->tapcount >= 2;
}

static inline int nemoshow_event_is_more_taps(struct nemoshow *show, struct showevent *event, int tapcount)
{
	return event->tapcount >= tapcount;
}

static inline int nemoshow_event_is_no_tap_with_up(struct nemoshow *show, struct showevent *event)
{
	return (event->tapcount + !!(event->type & NEMOSHOW_TOUCH_UP_EVENT)) == 0;
}

static inline int nemoshow_event_is_single_tap_with_up(struct nemoshow *show, struct showevent *event)
{
	return (event->tapcount + !!(event->type & NEMOSHOW_TOUCH_UP_EVENT)) == 1;
}

static inline int nemoshow_event_is_double_taps_with_up(struct nemoshow *show, struct showevent *event)
{
	return (event->tapcount + !!(event->type & NEMOSHOW_TOUCH_UP_EVENT)) == 2;
}

static inline int nemoshow_event_is_triple_taps_with_up(struct nemoshow *show, struct showevent *event)
{
	return (event->tapcount + !!(event->type & NEMOSHOW_TOUCH_UP_EVENT)) == 3;
}

static inline int nemoshow_event_is_many_taps_with_up(struct nemoshow *show, struct showevent *event)
{
	return (event->tapcount + !!(event->type & NEMOSHOW_TOUCH_UP_EVENT)) == 2;
}

static inline int nemoshow_event_is_more_taps_with_up(struct nemoshow *show, struct showevent *event, int tapcount)
{
	return (event->tapcount + !!(event->type & NEMOSHOW_TOUCH_UP_EVENT)) >= tapcount;
}

static inline int nemoshow_event_is_down(struct nemoshow *show, struct showevent *event)
{
	return event->type & NEMOSHOW_DOWN_EVENT;
}

static inline int nemoshow_event_is_motion(struct nemoshow *show, struct showevent *event)
{
	return event->type & NEMOSHOW_MOTION_EVENT;
}

static inline int nemoshow_event_is_up(struct nemoshow *show, struct showevent *event)
{
	return event->type & NEMOSHOW_UP_EVENT;
}

static inline int nemoshow_event_is_cancel(struct nemoshow *show, struct showevent *event)
{
	return event->type & NEMOSHOW_CANCEL_EVENT;
}

static inline int nemoshow_event_is_touch_down(struct nemoshow *show, struct showevent *event)
{
	return event->type & NEMOSHOW_TOUCH_DOWN_EVENT;
}

static inline int nemoshow_event_is_touch_up(struct nemoshow *show, struct showevent *event)
{
	return event->type & NEMOSHOW_TOUCH_UP_EVENT;
}

static inline int nemoshow_event_is_touch_motion(struct nemoshow *show, struct showevent *event)
{
	return event->type & NEMOSHOW_TOUCH_MOTION_EVENT;
}

static inline int nemoshow_event_is_touch_pressure(struct nemoshow *show, struct showevent *event)
{
	return event->type & NEMOSHOW_TOUCH_PRESSURE_EVENT;
}

static inline int nemoshow_event_is_pointer_enter(struct nemoshow *show, struct showevent *event)
{
	return event->type & NEMOSHOW_POINTER_ENTER_EVENT;
}

static inline int nemoshow_event_is_pointer_leave(struct nemoshow *show, struct showevent *event)
{
	return event->type & NEMOSHOW_POINTER_LEAVE_EVENT;
}

static inline int nemoshow_event_is_pointer_left_down(struct nemoshow *show, struct showevent *event)
{
	return event->type & NEMOSHOW_POINTER_LEFT_DOWN_EVENT;
}

static inline int nemoshow_event_is_pointer_left_up(struct nemoshow *show, struct showevent *event)
{
	return event->type & NEMOSHOW_POINTER_LEFT_UP_EVENT;
}

static inline int nemoshow_event_is_pointer_right_down(struct nemoshow *show, struct showevent *event)
{
	return event->type & NEMOSHOW_POINTER_RIGHT_DOWN_EVENT;
}

static inline int nemoshow_event_is_pointer_right_up(struct nemoshow *show, struct showevent *event)
{
	return event->type & NEMOSHOW_POINTER_RIGHT_UP_EVENT;
}

static inline int nemoshow_event_is_pointer_motion(struct nemoshow *show, struct showevent *event)
{
	return event->type & NEMOSHOW_POINTER_MOTION_EVENT;
}

static inline int nemoshow_event_is_pointer_axis(struct nemoshow *show, struct showevent *event)
{
	return event->type & NEMOSHOW_POINTER_AXIS_EVENT;
}

static inline int nemoshow_event_is_pointer_button_down(struct nemoshow *show, struct showevent *event, uint32_t button)
{
	return (event->type & NEMOSHOW_POINTER_DOWN_EVENT) && (button == 0 || event->value == button);
}

static inline int nemoshow_event_is_pointer_button_up(struct nemoshow *show, struct showevent *event, uint32_t button)
{
	return (event->type & NEMOSHOW_POINTER_UP_EVENT) && (button == 0 || event->value == button);
}

static inline int nemoshow_event_is_keyboard_enter(struct nemoshow *show, struct showevent *event)
{
	return event->type & NEMOSHOW_KEYBOARD_ENTER_EVENT;
}

static inline int nemoshow_event_is_keyboard_leave(struct nemoshow *show, struct showevent *event)
{
	return event->type & NEMOSHOW_KEYBOARD_LEAVE_EVENT;
}

static inline int nemoshow_event_is_keyboard_down(struct nemoshow *show, struct showevent *event)
{
	return event->type & NEMOSHOW_KEYBOARD_DOWN_EVENT;
}

static inline int nemoshow_event_is_keyboard_up(struct nemoshow *show, struct showevent *event)
{
	return event->type & NEMOSHOW_KEYBOARD_UP_EVENT;
}

static inline int nemoshow_event_is_keyboard_layout(struct nemoshow *show, struct showevent *event)
{
	return event->type & NEMOSHOW_KEYBOARD_LAYOUT_EVENT;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
