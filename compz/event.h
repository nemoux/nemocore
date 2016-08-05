#ifndef __NEMO_EVENT_H__
#define __NEMO_EVENT_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

typedef enum {
	NEMOEVENT_POINTER_ENTER_TYPE = (1 << 0),
	NEMOEVENT_POINTER_LEAVE_TYPE = (1 << 1),
	NEMOEVENT_POINTER_MOTION_TYPE = (1 << 2),
	NEMOEVENT_POINTER_BUTTON_TYPE = (1 << 3),
	NEMOEVENT_POINTER_AXIS_TYPE = (1 << 4),
	NEMOEVENT_KEYBOARD_ENTER_TYPE = (1 << 5),
	NEMOEVENT_KEYBOARD_LEAVE_TYPE = (1 << 6),
	NEMOEVENT_KEYBOARD_KEY_TYPE = (1 << 7),
	NEMOEVENT_KEYBOARD_MODIFIERS_TYPE = (1 << 8),
	NEMOEVENT_TOUCH_DOWN_TYPE = (1 << 9),
	NEMOEVENT_TOUCH_UP_TYPE = (1 << 10),
	NEMOEVENT_TOUCH_MOTION_TYPE = (1 << 11),
	NEMOEVENT_TOUCH_PRESSURE_TYPE = (1 << 12),
	NEMOEVENT_STICK_ENTER_TYPE = (1 << 13),
	NEMOEVENT_STICK_LEAVE_TYPE = (1 << 14),
	NEMOEVENT_STICK_TRANSLATE_TYPE = (1 << 15),
	NEMOEVENT_STICK_ROTATE_TYPE = (1 << 16),
	NEMOEVENT_STICK_BUTTON_TYPE = (1 << 17),
} NemoEventType;

struct nemoevent {
	uint64_t device;

	uint32_t serial;

	uint32_t time;
	uint32_t value;
	uint32_t state;

	float x, y, z;
	float gx, gy;
	float r;
	float p;
};

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
