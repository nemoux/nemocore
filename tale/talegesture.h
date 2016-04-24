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

static inline int nemotale_event_is_pointer_single_click(struct nemotale *tale, struct taleevent *event)
{
	struct taletap *tap = nemotale_pointer_get_tap(tale, event->device);

	if (nemotale_tap_has_state(tap, NEMOTALE_TAP_USED_STATE))
		return 0;

	if (tap != NULL && event->time - tap->grab_time < tale->single_click_duration)
		return 1;

	return 0;
}

static inline int nemotale_event_is_touch_single_click(struct nemotale *tale, struct taleevent *event)
{
	struct taletap *tap = event->tap;

	if (nemotale_tap_has_state(tap, NEMOTALE_TAP_USED_STATE))
		return 0;

	if (tap != NULL && event->time - tap->grab_time < tale->single_click_duration) {
		if (tap->dist <= tale->single_click_distance)
			return 1;
	}

	return 0;
}

static inline int nemotale_event_is_single_click(struct nemotale *tale, struct taleevent *event)
{
	if (event->type & NEMOTALE_POINTER_UP_EVENT)
		return nemotale_event_is_pointer_single_click(tale, event);
	else if (event->type & NEMOTALE_TOUCH_UP_EVENT)
		return nemotale_event_is_touch_single_click(tale, event);

	return 0;
}

static inline int nemotale_event_is_no_tap(struct nemotale *tale, struct taleevent *event)
{
	return event->tapcount == 0;
}

static inline int nemotale_event_is_single_tap(struct nemotale *tale, struct taleevent *event)
{
	return event->tapcount == 1;
}

static inline int nemotale_event_is_double_taps(struct nemotale *tale, struct taleevent *event)
{
	return event->tapcount == 2;
}

static inline int nemotale_event_is_triple_taps(struct nemotale *tale, struct taleevent *event)
{
	return event->tapcount == 3;
}

static inline int nemotale_event_is_many_taps(struct nemotale *tale, struct taleevent *event)
{
	return event->tapcount >= 2;
}

static inline int nemotale_event_is_more_taps(struct nemotale *tale, struct taleevent *event, int tapcount)
{
	return event->tapcount >= tapcount;
}

static inline int nemotale_event_is_down(struct nemotale *tale, struct taleevent *event)
{
	return event->type & NEMOTALE_DOWN_EVENT;
}

static inline int nemotale_event_is_motion(struct nemotale *tale, struct taleevent *event)
{
	return event->type & NEMOTALE_MOTION_EVENT;
}

static inline int nemotale_event_is_up(struct nemotale *tale, struct taleevent *event)
{
	return event->type & NEMOTALE_UP_EVENT;
}

static inline int nemotale_event_is_long_press(struct nemotale *tale, struct taleevent *event)
{
	return event->type & NEMOTALE_LONG_PRESS_EVENT;
}

static inline int nemotale_event_is_touch_down(struct nemotale *tale, struct taleevent *event)
{
	return event->type & NEMOTALE_TOUCH_DOWN_EVENT;
}

static inline int nemotale_event_is_touch_up(struct nemotale *tale, struct taleevent *event)
{
	return event->type & NEMOTALE_TOUCH_UP_EVENT;
}

static inline int nemotale_event_is_touch_motion(struct nemotale *tale, struct taleevent *event)
{
	return event->type & NEMOTALE_TOUCH_MOTION_EVENT;
}

static inline int nemotale_event_is_touch_long_press(struct nemotale *tale, struct taleevent *event)
{
	return event->type & NEMOTALE_TOUCH_LONG_PRESS_EVENT;
}

static inline int nemotale_event_is_pointer_enter(struct nemotale *tale, struct taleevent *event)
{
	return event->type & NEMOTALE_POINTER_ENTER_EVENT;
}

static inline int nemotale_event_is_pointer_leave(struct nemotale *tale, struct taleevent *event)
{
	return event->type & NEMOTALE_POINTER_LEAVE_EVENT;
}

static inline int nemotale_event_is_pointer_left_down(struct nemotale *tale, struct taleevent *event)
{
	return event->type & NEMOTALE_POINTER_LEFT_DOWN_EVENT;
}

static inline int nemotale_event_is_pointer_left_up(struct nemotale *tale, struct taleevent *event)
{
	return event->type & NEMOTALE_POINTER_LEFT_UP_EVENT;
}

static inline int nemotale_event_is_pointer_right_down(struct nemotale *tale, struct taleevent *event)
{
	return event->type & NEMOTALE_POINTER_RIGHT_DOWN_EVENT;
}

static inline int nemotale_event_is_pointer_right_up(struct nemotale *tale, struct taleevent *event)
{
	return event->type & NEMOTALE_POINTER_RIGHT_UP_EVENT;
}

static inline int nemotale_event_is_pointer_motion(struct nemotale *tale, struct taleevent *event)
{
	return event->type & NEMOTALE_POINTER_MOTION_EVENT;
}

static inline int nemotale_event_is_pointer_axis(struct nemotale *tale, struct taleevent *event)
{
	return event->type & NEMOTALE_POINTER_AXIS_EVENT;
}

static inline int nemotale_event_is_pointer_button_down(struct nemotale *tale, struct taleevent *event, uint32_t button)
{
	return (event->type & NEMOTALE_POINTER_DOWN_EVENT) && (button == 0 || event->value == button);
}

static inline int nemotale_event_is_pointer_button_up(struct nemotale *tale, struct taleevent *event, uint32_t button)
{
	return (event->type & NEMOTALE_POINTER_UP_EVENT) && (button == 0 || event->value == button);
}

static inline int nemotale_event_is_pointer_long_press(struct nemotale *tale, struct taleevent *event)
{
	return event->type & NEMOTALE_POINTER_LONG_PRESS_EVENT;
}

static inline int nemotale_event_is_keyboard_enter(struct nemotale *tale, struct taleevent *event)
{
	return event->type & NEMOTALE_KEYBOARD_ENTER_EVENT;
}

static inline int nemotale_event_is_keyboard_leave(struct nemotale *tale, struct taleevent *event)
{
	return event->type & NEMOTALE_KEYBOARD_LEAVE_EVENT;
}

static inline int nemotale_event_is_keyboard_down(struct nemotale *tale, struct taleevent *event)
{
	return event->type & NEMOTALE_KEYBOARD_DOWN_EVENT;
}

static inline int nemotale_event_is_keyboard_up(struct nemotale *tale, struct taleevent *event)
{
	return event->type & NEMOTALE_KEYBOARD_UP_EVENT;
}

static inline int nemotale_event_is_stick_enter(struct nemotale *tale, struct taleevent *event)
{
	return event->type & NEMOTALE_STICK_ENTER_EVENT;
}

static inline int nemotale_event_is_stick_leave(struct nemotale *tale, struct taleevent *event)
{
	return event->type & NEMOTALE_STICK_LEAVE_EVENT;
}

static inline int nemotale_event_is_stick_translate(struct nemotale *tale, struct taleevent *event)
{
	return event->type & NEMOTALE_STICK_TRANSLATE_EVENT;
}

static inline int nemotale_event_is_stick_rotate(struct nemotale *tale, struct taleevent *event)
{
	return event->type & NEMOTALE_STICK_ROTATE_EVENT;
}

static inline int nemotale_event_is_stick_button_down(struct nemotale *tale, struct taleevent *event)
{
	return event->type & NEMOTALE_STICK_BUTTON_DOWN_EVENT;
}

static inline int nemotale_event_is_stick_button_up(struct nemotale *tale, struct taleevent *event)
{
	return event->type & NEMOTALE_STICK_BUTTON_UP_EVENT;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
