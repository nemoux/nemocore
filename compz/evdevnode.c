#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <linux/input.h>
#include <mtdev-plumbing.h>
#include <wayland-server.h>
#include <wayland-nemo-seat-server-protocol.h>

#include <evdevnode.h>
#include <compz.h>
#include <seat.h>
#include <pointer.h>
#include <keyboard.h>
#include <touch.h>
#include <screen.h>
#include <nemomisc.h>
#include <nemolog.h>

#define BITS_PER_LONG						(sizeof(unsigned long) * 8)
#define NBITS(x)								((((x) - 1) / BITS_PER_LONG) + 1)
#define OFF(x)									((x) % BITS_PER_LONG)
#define BIT(x)									(1UL << OFF(x))
#define LONG(x)									((x) / BITS_PER_LONG)
#define TEST_BIT(array, bit)		((array[LONG(bit)] >> OFF(bit)) & 1)

static void evdev_transform_absolute(struct evdevnode *node, float *x, float *y)
{
	if (!node->abs.apply_calibration) {
		*x = node->abs.x;
		*y = node->abs.y;
	} else {
		*x = node->abs.x * node->abs.calibration[0] +
			node->abs.y * node->abs.calibration[1] +
			node->abs.calibration[2];

		*y = node->abs.x * node->abs.calibration[3] +
			node->abs.y * node->abs.calibration[4] +
			node->abs.calibration[5];
	}
}

static int evdev_flush_events(struct evdevnode *node, uint32_t time)
{
	float x, y;
	float cx, cy;
	int slot, id = 0;

	slot = node->mt.slot;
	if (slot >= EVDEV_MAX_SLOTS) {
		nemolog_error("EVDEV", "multitouch slot index is overflow!\n");
		return -1;
	}

	switch (node->pending_event) {
		case EVDEV_NONE:
			break;

		case EVDEV_RELATIVE_MOTION:
			nemopointer_notify_motion(node->pointer, time, node->rel.dx, node->rel.dy);
			node->rel.dx = 0;
			node->rel.dy = 0;
			break;

		case EVDEV_ABSOLUTE_MT_DOWN:
			if (node->base.screen != NULL) {
				nemoscreen_transform_to_global(node->base.screen,
						node->mt.slots[slot].x * node->base.screen->width,
						node->mt.slots[slot].y * node->base.screen->height,
						&x, &y);
			} else {
				nemoinput_transform_to_global(&node->base,
						node->mt.slots[slot].x * node->base.width,
						node->mt.slots[slot].y * node->base.height,
						&x, &y);
			}
			id = ++node->tapsequence;
			node->mt.slots[slot].seat_slot = id;
			nemotouch_notify_down(node->touch, time, id, x, y);
			break;

		case EVDEV_ABSOLUTE_MT_MOTION:
			if (node->base.screen != NULL) {
				nemoscreen_transform_to_global(node->base.screen,
						node->mt.slots[slot].x * node->base.screen->width,
						node->mt.slots[slot].y * node->base.screen->height,
						&x, &y);
			} else {
				nemoinput_transform_to_global(&node->base,
						node->mt.slots[slot].x * node->base.width,
						node->mt.slots[slot].y * node->base.height,
						&x, &y);
			}
			id = node->mt.slots[slot].seat_slot;
			nemotouch_notify_motion(node->touch, time, id, x, y);
			break;

		case EVDEV_ABSOLUTE_MT_UP:
			id = node->mt.slots[slot].seat_slot;
			nemotouch_notify_up(node->touch, time, id);
			break;

		case EVDEV_ABSOLUTE_MT_PRESSURE:
			id = node->mt.slots[slot].seat_slot;
			nemotouch_notify_pressure(node->touch, time, id, node->mt.slots[slot].p);
			break;

		case EVDEV_ABSOLUTE_TOUCH_DOWN:
			evdev_transform_absolute(node, &cx, &cy);
			if (node->base.screen != NULL) {
				nemoscreen_transform_to_global(node->base.screen,
						cx * node->base.screen->width,
						cy * node->base.screen->height,
						&x, &y);
			} else {
				nemoinput_transform_to_global(&node->base,
						cx * node->base.width,
						cy * node->base.height,
						&x, &y);
			}
			id = ++node->tapsequence;
			node->abs.seat_slot = id;
			nemotouch_notify_down(node->touch, time, id, x, y);
			break;

		case EVDEV_ABSOLUTE_MOTION:
			evdev_transform_absolute(node, &cx, &cy);
			if (node->base.screen != NULL) {
				nemoscreen_transform_to_global(node->base.screen,
						cx * node->base.screen->width,
						cy * node->base.screen->height,
						&x, &y);
			} else {
				nemoinput_transform_to_global(&node->base,
						cx * node->base.width,
						cy * node->base.height,
						&x, &y);
			}
			if (node->seat_caps & EVDEV_SEAT_TOUCH) {
				id = node->abs.seat_slot;
				nemotouch_notify_motion(node->touch, time, id, x, y);
			} else if (node->seat_caps & EVDEV_SEAT_POINTER) {
				nemopointer_notify_motion_absolute(node->pointer, time, x, y);
			}
			break;

		case EVDEV_ABSOLUTE_AXIS:
			if (node->seat_caps & EVDEV_SEAT_POINTER) {
				nemopointer_notify_axis(node->pointer, time, node->abs.axis, node->abs.r);
			}
			break;

		case EVDEV_ABSOLUTE_TOUCH_UP:
			id = node->abs.seat_slot;
			nemotouch_notify_up(node->touch, time, id);
			break;

		case EVDEV_ABSOLUTE_TRANSLATE:
			break;

		case EVDEV_ABSOLUTE_ROTATE:
			break;

		default:
			break;
	}

	node->pending_event = EVDEV_NONE;

	return id;
}

static void evdev_process_relative(struct evdevnode *node, struct input_event *e, uint32_t time)
{
	switch (e->code) {
		case REL_X:
			if (node->pending_event != EVDEV_RELATIVE_MOTION)
				evdev_flush_events(node, time);
			node->rel.dx += e->value;
			node->pending_event = EVDEV_RELATIVE_MOTION;
			break;

		case REL_Y:
			if (node->pending_event != EVDEV_RELATIVE_MOTION)
				evdev_flush_events(node, time);
			node->rel.dy += e->value;
			node->pending_event = EVDEV_RELATIVE_MOTION;
			break;

		case REL_WHEEL:
			evdev_flush_events(node, time);

			switch (e->value) {
				case -1:
				case 1:
					nemopointer_notify_axis(node->pointer, time, NEMO_POINTER_AXIS_ROTATE_X, -1 * e->value * 10);
					break;

				default:
					break;
			}
			break;

		case REL_HWHEEL:
			evdev_flush_events(node, time);

			switch (e->value) {
				case -1:
				case 1:
					nemopointer_notify_axis(node->pointer, time, NEMO_POINTER_AXIS_ROTATE_Y, e->value * 10);
					break;

				default:
					break;
			}
			break;

		default:
			break;
	}
}

static void evdev_process_absolute(struct evdevnode *node, struct input_event *e, uint32_t time)
{
	if (node->is_mt != 0) {
		if (node->mt.slot >= EVDEV_MAX_SLOTS) {
			nemolog_error("EVDEV", "multitouch slot index is overflow!\n");
			return;
		}

		switch (e->code) {
			case ABS_MT_SLOT:
				evdev_flush_events(node, time);
				node->mt.slot = e->value;
				break;

			case ABS_MT_TRACKING_ID:
				if (node->pending_event != EVDEV_NONE &&
						node->pending_event != EVDEV_ABSOLUTE_MT_MOTION)
					evdev_flush_events(node, time);
				if (e->value >= 0)
					node->pending_event = EVDEV_ABSOLUTE_MT_DOWN;
				else
					node->pending_event = EVDEV_ABSOLUTE_MT_UP;
				break;

			case ABS_MT_POSITION_X:
				node->mt.slots[node->mt.slot].x = (e->value - node->abs.min_x) / (node->abs.max_x - node->abs.min_x);
				if (node->pending_event == EVDEV_NONE)
					node->pending_event = EVDEV_ABSOLUTE_MT_MOTION;
				break;

			case ABS_MT_POSITION_Y:
				node->mt.slots[node->mt.slot].y = (e->value - node->abs.min_y) / (node->abs.max_y - node->abs.min_y);
				if (node->pending_event == EVDEV_NONE)
					node->pending_event = EVDEV_ABSOLUTE_MT_MOTION;
				break;

			case ABS_MT_PRESSURE:
				evdev_flush_events(node, time);
				node->mt.slots[node->mt.slot].p = (e->value - node->abs.min_p) / (node->abs.max_p - node->abs.min_p);
				node->pending_event = EVDEV_ABSOLUTE_MT_PRESSURE;
				break;
		}
	} else if (node->seat_caps & EVDEV_SEAT_POINTER) {
		switch (e->code) {
			case ABS_X:
				node->abs.x = (e->value - node->abs.min_x) / (node->abs.max_x - node->abs.min_x);
				if (node->pending_event == EVDEV_NONE)
					node->pending_event = EVDEV_ABSOLUTE_MOTION;
				break;

			case ABS_Y:
				node->abs.y = (e->value - node->abs.min_y) / (node->abs.max_y - node->abs.min_y);
				if (node->pending_event == EVDEV_NONE)
					node->pending_event = EVDEV_ABSOLUTE_MOTION;
				break;

			case ABS_RX:
				node->abs.r = (e->value - node->abs.min_rx) / (node->abs.max_rx - node->abs.min_rx);
				node->abs.axis = NEMO_POINTER_AXIS_ROTATE_X;
				if (node->pending_event == EVDEV_NONE)
					node->pending_event = EVDEV_ABSOLUTE_AXIS;
				break;

			case ABS_RY:
				node->abs.r = (e->value - node->abs.min_ry) / (node->abs.max_ry - node->abs.min_ry);
				node->abs.axis = NEMO_POINTER_AXIS_ROTATE_Y;
				if (node->pending_event == EVDEV_NONE)
					node->pending_event = EVDEV_ABSOLUTE_AXIS;
				break;

			case ABS_RZ:
				node->abs.r = (e->value - node->abs.min_rz) / (node->abs.max_rz - node->abs.min_rz);
				node->abs.axis = NEMO_POINTER_AXIS_ROTATE_Z;
				if (node->pending_event == EVDEV_NONE)
					node->pending_event = EVDEV_ABSOLUTE_AXIS;
				break;
		}
	}
}

static void evdev_process_key(struct evdevnode *node, struct input_event *e, uint32_t time)
{
	if (e->value == 2)
		return;

	switch (e->code) {
		case BTN_TOUCH:
		case BTN_STYLUS:
		case BTN_STYLUS2:
			if (!node->is_mt) {
				if (node->pending_event != EVDEV_NONE &&
						node->pending_event != EVDEV_ABSOLUTE_MOTION)
					evdev_flush_events(node, time);

				node->pending_event = e->value ? EVDEV_ABSOLUTE_TOUCH_DOWN : EVDEV_ABSOLUTE_TOUCH_UP;
			}
			break;

		case BTN_0:
		case BTN_1:
		case BTN_2:
		case BTN_LEFT:
		case BTN_RIGHT:
		case BTN_MIDDLE:
		case BTN_SIDE:
		case BTN_EXTRA:
		case BTN_FORWARD:
		case BTN_BACK:
		case BTN_TASK:
			evdev_flush_events(node, time);

			if (node->seat_caps & EVDEV_SEAT_POINTER) {
				nemopointer_notify_button(node->pointer, time, e->code, e->value ? WL_POINTER_BUTTON_STATE_PRESSED : WL_POINTER_BUTTON_STATE_RELEASED);
			}
			break;

		default:
			evdev_flush_events(node, time);

			nemokeyboard_notify_key(node->keyboard, time, e->code, e->value ? WL_KEYBOARD_KEY_STATE_PRESSED : WL_KEYBOARD_KEY_STATE_RELEASED);
			break;
	}
}

static void evdev_process_event(struct evdevnode *node, struct input_event *e, uint32_t time)
{
	switch (e->type) {
		case EV_REL:
			evdev_process_relative(node, e, time);
			break;

		case EV_ABS:
			evdev_process_absolute(node, e, time);
			break;

		case EV_KEY:
			evdev_process_key(node, e, time);
			break;

		case EV_SYN:
			evdev_flush_events(node, time);
			if (node->touch != NULL)
				nemotouch_notify_frames(node->touch);
			break;
	}
}

static inline void evdev_dispatch_events(struct evdevnode *node, struct input_event *events, int count)
{
	struct input_event *e;
	uint32_t time;

	for (e = events; e < events + count; e++) {
		time = e->time.tv_sec * 1000 + e->time.tv_usec / 1000;

		evdev_process_event(node, e, time);
	}
}

static inline void evdev_dispatch_event(struct evdevnode *node, struct input_event *e)
{
	evdev_process_event(node, e,
			e->time.tv_sec * 1000 + e->time.tv_usec / 1000);
}

static int evdev_dispatch_data(int fd, uint32_t mask, void *data)
{
	struct evdevnode *node = (struct evdevnode *)data;

	if (node->mtdev == NULL) {
		struct input_event events[128];
		int len;

		do {
			len = read(fd, &events, sizeof(events));
			if (len < 0 || len % sizeof(events[0]) != 0) {
				if (len < 0 && errno != EAGAIN && errno != EINTR) {
					nemolog_message("EVDEV", "%s node is died\n", node->base.devnode);
					wl_event_source_remove(node->source);
					node->source = NULL;
				}

				return 1;
			}

			evdev_dispatch_events(node, events, len / sizeof(events[0]));
		} while (len > 0);
	} else {
		struct input_event events[2];

		while (!mtdev_empty(node->mtdev)) {
			mtdev_get_event(node->mtdev, events);
			evdev_dispatch_event(node, events);
		}
	}

	return 1;
}

static int evdev_configure_node(struct evdevnode *node)
{
	struct input_absinfo absinfo;
	unsigned long ev_bits[NBITS(EV_MAX)];
	unsigned long abs_bits[NBITS(ABS_MAX)];
	unsigned long rel_bits[NBITS(REL_MAX)];
	unsigned long key_bits[NBITS(KEY_MAX)];
	int has_abs, has_rel, has_mt, has_z;
	int has_button, has_keyboard, has_touch;
	int i;

	has_rel = 0;
	has_abs = 0;
	has_mt = 0;
	has_z = 0;
	has_button = 0;
	has_keyboard = 0;
	has_touch = 0;

	ioctl(node->fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits);
	if (TEST_BIT(ev_bits, EV_ABS)) {
		ioctl(node->fd, EVIOCGBIT(EV_ABS, sizeof(abs_bits)), abs_bits);

		if (TEST_BIT(abs_bits, ABS_X)) {
			ioctl(node->fd, EVIOCGABS(ABS_X), &absinfo);
			node->abs.min_x = absinfo.minimum;
			node->abs.max_x = absinfo.maximum;
			has_abs = 1;
		}
		if (TEST_BIT(abs_bits, ABS_Y)) {
			ioctl(node->fd, EVIOCGABS(ABS_Y), &absinfo);
			node->abs.min_y = absinfo.minimum;
			node->abs.max_y = absinfo.maximum;
			has_abs = 1;
		}
		if (TEST_BIT(abs_bits, ABS_Z)) {
			ioctl(node->fd, EVIOCGABS(ABS_Z), &absinfo);
			node->abs.min_z = absinfo.minimum;
			node->abs.max_z = absinfo.maximum;
			has_abs = 1;
			has_z = 1;
		}
		if (TEST_BIT(abs_bits, ABS_RX)) {
			ioctl(node->fd, EVIOCGABS(ABS_RX), &absinfo);
			node->abs.min_rx = absinfo.minimum;
			node->abs.max_rx = absinfo.maximum;
			has_abs = 1;
		}
		if (TEST_BIT(abs_bits, ABS_RY)) {
			ioctl(node->fd, EVIOCGABS(ABS_RY), &absinfo);
			node->abs.min_ry = absinfo.minimum;
			node->abs.max_ry = absinfo.maximum;
			has_abs = 1;
		}
		if (TEST_BIT(abs_bits, ABS_RZ)) {
			ioctl(node->fd, EVIOCGABS(ABS_RZ), &absinfo);
			node->abs.min_rz = absinfo.minimum;
			node->abs.max_rz = absinfo.maximum;
			has_abs = 1;
		}
		if (TEST_BIT(abs_bits, ABS_MT_POSITION_X) &&
				TEST_BIT(abs_bits, ABS_MT_POSITION_Y)) {
			ioctl(node->fd, EVIOCGABS(ABS_MT_POSITION_X), &absinfo);
			node->abs.min_x = absinfo.minimum;
			node->abs.max_x = absinfo.maximum;
			ioctl(node->fd, EVIOCGABS(ABS_MT_POSITION_Y), &absinfo);
			node->abs.min_y = absinfo.minimum;
			node->abs.max_y = absinfo.maximum;
			node->is_mt = 1;
			has_touch = 1;
			has_mt = 1;

			if (!TEST_BIT(abs_bits, ABS_MT_SLOT)) {
				node->mtdev = mtdev_new_open(node->fd);
				if (node->mtdev == NULL) {
					nemolog_message("EVDEV", "failed to open mtdev '%s'\n", node->base.devnode);
					return -1;
				}
				node->mt.slot = node->mtdev->caps.slot.value;
			} else {
				ioctl(node->fd, EVIOCGABS(ABS_MT_SLOT), &absinfo);
				node->mt.slot = absinfo.value;
			}
		}
		if (TEST_BIT(abs_bits, ABS_MT_PRESSURE)) {
			ioctl(node->fd, EVIOCGABS(ABS_MT_PRESSURE), &absinfo);
			node->abs.min_p = absinfo.minimum;
			node->abs.max_p = absinfo.maximum;
		}
	}
	if (TEST_BIT(ev_bits, EV_REL)) {
		ioctl(node->fd, EVIOCGBIT(EV_REL, sizeof(rel_bits)), rel_bits);
		if (TEST_BIT(rel_bits, REL_X) || TEST_BIT(rel_bits, REL_Y))
			has_rel = 1;
	}
	if (TEST_BIT(ev_bits, EV_KEY)) {
		ioctl(node->fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits);
		if (TEST_BIT(key_bits, BTN_TOOL_FINGER) &&
				!TEST_BIT(key_bits, BTN_TOOL_PEN) &&
				(has_abs || has_mt)) {
			// add touchpad device
		}
		for (i = KEY_ESC; i < KEY_MAX; i++) {
			if (i >= BTN_MISC && i < KEY_OK)
				continue;
			if (TEST_BIT(key_bits, i)) {
				has_keyboard = 1;
				break;
			}
		}
		if (TEST_BIT(key_bits, BTN_TOUCH))
			has_touch = 1;
		for (i = BTN_MISC; i < BTN_JOYSTICK; i++) {
			if (TEST_BIT(key_bits, i)) {
				has_button = 1;
				break;
			}
		}
	}

	if ((has_abs || has_rel) && has_button && !has_z) {
		// add pointer device
		node->pointer = nemopointer_create(node->compz->seat, &node->base);
		if (node->pointer != NULL) {
			node->seat_caps |= EVDEV_SEAT_POINTER;
			nemolog_message("EVDEV", "%s [%s] is a pointer\n", node->devname, node->base.devnode);
		}
	}
	if (has_keyboard) {
		// add keyboard device
		node->keyboard = nemokeyboard_create(node->compz->seat, &node->base);
		if (node->keyboard != NULL) {
			node->seat_caps |= EVDEV_SEAT_KEYBOARD;
			nemolog_message("EVDEV", "%s [%s] is a keyboard\n", node->devname, node->base.devnode);
		}
	}
	if (has_touch && !has_button) {
		// add touch device
		node->touch = nemotouch_create(node->compz->seat, &node->base);
		if (node->touch != NULL) {
			node->seat_caps |= EVDEV_SEAT_TOUCH;
			nemolog_message("EVDEV", "%s [%s] is a touch\n", node->devname, node->base.devnode);
		}
	}

	return 0;
}

struct evdevnode *evdev_create_node(struct nemocompz *compz, const char *devpath, int fd)
{
	struct evdevnode *node;
	char devname[256] = "unknown";
	char devphys[256];
	uint32_t nodeid, screenid;

	node = (struct evdevnode *)malloc(sizeof(struct evdevnode));
	if (node == NULL)
		return NULL;
	memset(node, 0, sizeof(struct evdevnode));

	node->compz = compz;
	node->fd = fd;

	wl_list_init(&node->link);
	wl_list_init(&node->base.link);

	ioctl(node->fd, EVIOCGNAME(sizeof(devname)), devname);
	devname[sizeof(devname) - 1] = '\0';
	ioctl(node->fd, EVIOCGPHYS(sizeof(devphys)), devphys);
	devphys[sizeof(devphys) - 1] = '\0';

	node->base.devnode = strdup(devphys);

	node->devname = strdup(devname);
	node->devphys = strdup(devphys);
	node->devpath = strdup(devpath);

	node->pointer = NULL;
	node->keyboard = NULL;

	if (evdev_configure_node(node) < 0)
		goto err1;

	if (node->seat_caps == 0)
		goto err1;

	if (node->pointer != NULL)
		node->base.type |= NEMOINPUT_POINTER_TYPE;
	if (node->keyboard != NULL)
		node->base.type |= NEMOINPUT_KEYBOARD_TYPE;
	if (node->touch != NULL)
		node->base.type |= NEMOINPUT_TOUCH_TYPE;

	nemoinput_set_size(&node->base,
			nemocompz_get_scene_width(compz),
			nemocompz_get_scene_height(compz));

	node->source = wl_event_loop_add_fd(compz->loop,
			node->fd,
			WL_EVENT_READABLE,
			evdev_dispatch_data,
			node);
	if (node->source == NULL)
		goto err1;

	wl_list_insert(compz->evdev_list.prev, &node->link);
	wl_list_insert(compz->input_list.prev, &node->base.link);

	return node;

err1:
	evdev_destroy_node(node);

	return NULL;
}

void evdev_destroy_node(struct evdevnode *node)
{
	wl_list_remove(&node->link);
	wl_list_remove(&node->base.link);

	if (node->source != NULL)
		wl_event_source_remove(node->source);

	if (node->base.screen != NULL)
		nemoinput_put_screen(&node->base);

	if (node->pointer != NULL)
		nemopointer_destroy(node->pointer);
	if (node->keyboard != NULL)
		nemokeyboard_destroy(node->keyboard);
	if (node->touch != NULL)
		nemotouch_destroy(node->touch);

	close(node->fd);

	if (node->base.devnode != NULL)
		free(node->base.devnode);

	free(node->devname);
	free(node->devphys);
	free(node->devpath);
	free(node);
}
