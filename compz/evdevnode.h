#ifndef	__EVDEV_NODE_H__
#define	__EVDEV_NODE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <mtdev.h>

#include <compz.h>
#include <input.h>

#define	EVDEV_MAX_SLOTS			(16)

typedef enum {
	EVDEV_NONE = 0,
	EVDEV_ABSOLUTE_TOUCH_DOWN = 1,
	EVDEV_ABSOLUTE_MOTION = 2,
	EVDEV_ABSOLUTE_TOUCH_UP = 3,
	EVDEV_ABSOLUTE_MT_DOWN = 4,
	EVDEV_ABSOLUTE_MT_MOTION = 5,
	EVDEV_ABSOLUTE_MT_UP = 6,
	EVDEV_RELATIVE_MOTION = 7,
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

	struct wl_list link;

	uint32_t seat_caps;

	struct {
		int min_x, max_x, min_y, max_y;
		uint32_t seat_slot;
		int32_t x, y;

		int apply_calibration;
		float calibration[6];
	} abs;

	struct {
		int slot;
		struct {
			int32_t x, y;
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

extern struct evdevnode *evdev_create_node(struct nemocompz *compz, const char *path, int fd);
extern void evdev_destroy_node(struct evdevnode *node);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
