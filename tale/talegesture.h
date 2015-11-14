#ifndef	__NEMOTALE_GESTURE_H__
#define	__NEMOTALE_GESTURE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <math.h>

#include <nemotale.h>
#include <talenode.h>
#include <taleevent.h>

static inline int nemotale_is_pointer_single_click(struct nemotale *tale, struct taleevent *event)
{
	struct taletap *tap = nemotale_pointer_get_tap(tale, event->device);

	if (nemotale_tap_has_state(tap, NEMOTALE_TAP_USED_STATE))
		return 0;

	if (tap != NULL && event->time - tap->grab_time < tale->single_click_duration) {
		return 1;
	}

	return 0;
}

static inline int nemotale_is_touch_single_click(struct nemotale *tale, struct taleevent *event)
{
	struct taletap *tap = event->tap;

	if (nemotale_tap_has_state(tap, NEMOTALE_TAP_USED_STATE))
		return 0;

	if (tap != NULL && event->time - tap->grab_time < tale->single_click_duration) {
		float dx = tap->grab_gx - event->gx;
		float dy = tap->grab_gy - event->gy;

		if (sqrtf(dx * dx + dy * dy) < tale->single_click_distance)
			return 1;
	}

	return 0;
}

static inline int nemotale_is_single_click(struct nemotale *tale, struct taleevent *event, uint32_t type)
{
	if (type & NEMOTALE_POINTER_UP_EVENT)
		return nemotale_is_pointer_single_click(tale, event);
	else if (type & NEMOTALE_TOUCH_UP_EVENT)
		return nemotale_is_touch_single_click(tale, event);

	return 0;
}

static inline int nemotale_is_single_tap(struct nemotale *tale, struct taleevent *event, uint32_t type)
{
	return event->tapcount == 1;
}

static inline int nemotale_is_double_taps(struct nemotale *tale, struct taleevent *event, uint32_t type)
{
#ifdef NEMOUX_WITH_TAP_MINIMUM_DISTANCE
	if (event->tapcount == 2) {
		double dx = event->taps[1]->gx - event->taps[0]->gx;
		double dy = event->taps[1]->gy - event->taps[0]->gy;

		if (sqrtf(dx * dx + dy * dy) >= tale->tap_minimum_distance)
			return 1;
	}

	return 0;
#else
	return event->tapcount == 2;
#endif
}

static inline int nemotale_is_triple_taps(struct nemotale *tale, struct taleevent *event, uint32_t type)
{
	return event->tapcount == 3;
}

static inline int nemotale_is_many_taps(struct nemotale *tale, struct taleevent *event, uint32_t type)
{
	return event->tapcount >= 2;
}

static inline int nemotale_is_close_event(struct nemotale *tale, struct taleevent *event, uint32_t type)
{
	if (event->tapcount >= 3 &&
			event->time - event->taps[0]->grab_time < tale->close_duration) {
		double cx = 0.0f;
		double cy = 0.0f;
		int count = 0;
		int i;

		for (i = 0; i < event->tapcount; i++) {
			cx += event->taps[i]->grab_gx;
			cy += event->taps[i]->grab_gy;
		}

		cx /= event->tapcount;
		cy /= event->tapcount;

		for (i = 0; i < event->tapcount; i++) {
			double dx0 = event->taps[i]->grab_gx - cx;
			double dy0 = event->taps[i]->grab_gy - cy;
			double dx1 = event->taps[i]->gx - cx;
			double dy1 = event->taps[i]->gy - cy;
			double d0 = sqrtf(dx0 * dx0 + dy0 * dy0);
			double d1 = sqrtf(dx1 * dx1 + dy1 * dy1);

			if (d0 > d1 && d0 - d1 > tale->close_distance && ++count >= 2)
				return 1;
		}
	}

	return 0;
}

static inline int nemotale_is_down_event(struct nemotale *tale, struct taleevent *event, uint32_t type)
{
	return type & NEMOTALE_DOWN_EVENT;
}

static inline int nemotale_is_motion_event(struct nemotale *tale, struct taleevent *event, uint32_t type)
{
	return type & NEMOTALE_MOTION_EVENT;
}

static inline int nemotale_is_up_event(struct nemotale *tale, struct taleevent *event, uint32_t type)
{
	return type & NEMOTALE_UP_EVENT;
}

static inline int nemotale_is_long_press(struct nemotale *tale, struct taleevent *event, uint32_t type)
{
	return type & NEMOTALE_LONG_PRESS_EVENT;
}

static inline int nemotale_is_touch_down(struct nemotale *tale, struct taleevent *event, uint32_t type)
{
	return type & NEMOTALE_TOUCH_DOWN_EVENT;
}

static inline int nemotale_is_touch_up(struct nemotale *tale, struct taleevent *event, uint32_t type)
{
	return type & NEMOTALE_TOUCH_UP_EVENT;
}

static inline int nemotale_is_touch_motion(struct nemotale *tale, struct taleevent *event, uint32_t type)
{
	return type & NEMOTALE_TOUCH_MOTION_EVENT;
}

static inline int nemotale_is_touch_long_press(struct nemotale *tale, struct taleevent *event, uint32_t type)
{
	return type & NEMOTALE_TOUCH_LONG_PRESS_EVENT;
}

static inline int nemotale_is_pointer_enter(struct nemotale *tale, struct taleevent *event, uint32_t type)
{
	return type & NEMOTALE_POINTER_ENTER_EVENT;
}

static inline int nemotale_is_pointer_leave(struct nemotale *tale, struct taleevent *event, uint32_t type)
{
	return type & NEMOTALE_POINTER_LEAVE_EVENT;
}

static inline int nemotale_is_pointer_left_down(struct nemotale *tale, struct taleevent *event, uint32_t type)
{
	return type & NEMOTALE_POINTER_LEFT_DOWN_EVENT;
}

static inline int nemotale_is_pointer_left_up(struct nemotale *tale, struct taleevent *event, uint32_t type)
{
	return type & NEMOTALE_POINTER_LEFT_UP_EVENT;
}

static inline int nemotale_is_pointer_right_down(struct nemotale *tale, struct taleevent *event, uint32_t type)
{
	return type & NEMOTALE_POINTER_RIGHT_DOWN_EVENT;
}

static inline int nemotale_is_pointer_right_up(struct nemotale *tale, struct taleevent *event, uint32_t type)
{
	return type & NEMOTALE_POINTER_RIGHT_UP_EVENT;
}

static inline int nemotale_is_pointer_motion(struct nemotale *tale, struct taleevent *event, uint32_t type)
{
	return type & NEMOTALE_POINTER_MOTION_EVENT;
}

static inline int nemotale_is_pointer_axis(struct nemotale *tale, struct taleevent *event, uint32_t type)
{
	return type & NEMOTALE_POINTER_AXIS_EVENT;
}

static inline int nemotale_is_pointer_button_down(struct nemotale *tale, struct taleevent *event, uint32_t type, uint32_t button)
{
	return (type & NEMOTALE_POINTER_DOWN_EVENT) && (event->value == button);
}

static inline int nemotale_is_pointer_button_up(struct nemotale *tale, struct taleevent *event, uint32_t type, uint32_t button)
{
	return (type & NEMOTALE_POINTER_UP_EVENT) && (event->value == button);
}

static inline int nemotale_is_pointer_long_press(struct nemotale *tale, struct taleevent *event, uint32_t type)
{
	return type & NEMOTALE_POINTER_LONG_PRESS_EVENT;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
