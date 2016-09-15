#ifndef	__EVDEV_NODE_H__
#define	__EVDEV_NODE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <mtdev.h>

#include <compz.h>
#include <input.h>

#define	EVDEV_MAX_SLOTS			(128)

typedef enum {
	EVDEV_NONE = 0,
	EVDEV_ABSOLUTE_TOUCH_DOWN = 1,
	EVDEV_ABSOLUTE_MOTION = 2,
	EVDEV_ABSOLUTE_AXIS = 3,
	EVDEV_ABSOLUTE_TOUCH_UP = 4,
	EVDEV_ABSOLUTE_MT_DOWN = 5,
	EVDEV_ABSOLUTE_MT_MOTION = 6,
	EVDEV_ABSOLUTE_MT_UP = 7,
	EVDEV_ABSOLUTE_MT_PRESSURE = 8,
	EVDEV_RELATIVE_MOTION = 9,
	EVDEV_ABSOLUTE_TRANSLATE = 10,
	EVDEV_ABSOLUTE_ROTATE = 11
} EvdevEventType;

typedef enum {
	EVDEV_SEAT_POINTER = (1 << 0),
	EVDEV_SEAT_KEYBOARD = (1 << 1),
	EVDEV_SEAT_TOUCH = (1 << 2)
} EvdevSeatCapability;

struct nemopointer;
struct nemokeyboard;
struct nemotouch;
struct nemoscreen;

struct evdevnode {
	struct inputnode base;

	struct nemocompz *compz;

	int fd;
	struct wl_event_source *source;
	char *devname;
	char *devphys;
	char *devpath;

	struct wl_list link;

	uint32_t seat_caps;

	struct {
		float min_x, max_x, min_y, max_y, min_z, max_z;
		float min_rx, max_rx, min_ry, max_ry, min_rz, max_rz;
		float min_p, max_p;
		float x, y, z;
		float rx, ry, rz;
		float r;
		float p;
		uint32_t seat_slot;
		int axis;

		int apply_calibration;
		float calibration[6];
	} abs;

	struct {
		int slot;
		struct {
			float x, y;
			float p;
			uint32_t seat_slot;
		} slots[EVDEV_MAX_SLOTS];
	} mt;
	struct mtdev *mtdev;
	int is_mt;

	struct {
		int32_t dx, dy;
	} rel;

	uint64_t tapsequence;

	uint32_t pending_event;

	struct nemopointer *pointer;
	struct nemokeyboard *keyboard;
	struct nemotouch *touch;
};

extern struct evdevnode *evdev_create_node(struct nemocompz *compz, const char *devpath, int fd);
extern void evdev_destroy_node(struct evdevnode *node);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
