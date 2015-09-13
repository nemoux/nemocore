#ifndef	__NEMOTALE_GESTURE_H__
#define	__NEMOTALE_GESTURE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemotale.h>
#include <talenode.h>
#include <taleevent.h>

static inline int nemotale_pointer_is_single_click(struct nemotale *tale, struct taleevent *event)
{
	struct taletap *tap = nemotale_pointer_get_tap(tale, event->device);

	if (tap != NULL && event->time - tap->grab_time < 150) {
		return 1;
	}

	return 0;
}

static inline int nemotale_touch_is_single_click(struct nemotale *tale, struct taleevent *event)
{
	struct taletap *tap = nemotale_touch_get_tap(tale, event->device);

	if (tap != NULL && event->time - tap->grab_time < 150 && sqrtf(event->dx * event->dx + event->dy * event->dy) < 30) {
		return 1;
	}

	return 0;
}

static inline int nemotale_is_single_click(struct nemotale *tale, struct taleevent *event, uint32_t type)
{
	if (type & NEMOTALE_POINTER_UP_EVENT)
		return nemotale_pointer_is_single_click(tale, event);
	else if (type & NEMOTALE_TOUCH_UP_EVENT)
		return nemotale_touch_is_single_click(tale, event);

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
