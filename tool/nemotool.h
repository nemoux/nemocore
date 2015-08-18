#ifndef	__NEMOTOOL_H__
#define	__NEMOTOOL_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <time.h>
#include <sys/types.h>

#include <xkbcommon/xkbcommon.h>
#include <wayland-client.h>
#include <wayland-presentation-timing-client-protocol.h>
#include <wayland-nemo-seat-client-protocol.h>
#include <wayland-nemo-sound-client-protocol.h>
#include <wayland-nemo-shell-client-protocol.h>

#include <nemolist.h>
#include <nemolistener.h>

typedef enum {
	NEMOTOOL_POINTER_ENTER_EVENT = (1 << 0),
	NEMOTOOL_POINTER_LEAVE_EVENT = (1 << 1),
	NEMOTOOL_POINTER_MOTION_EVENT = (1 << 2),
	NEMOTOOL_POINTER_BUTTON_EVENT = (1 << 3),
	NEMOTOOL_POINTER_AXIS_EVENT = (1 << 4),
	NEMOTOOL_KEYBOARD_ENTER_EVENT = (1 << 5),
	NEMOTOOL_KEYBOARD_LEAVE_EVENT = (1 << 6),
	NEMOTOOL_KEYBOARD_KEY_EVENT = (1 << 7),
	NEMOTOOL_KEYBOARD_MODIFIERS_EVENT = (1 << 8),
	NEMOTOOL_TOUCH_DOWN_EVENT = (1 << 9),
	NEMOTOOL_TOUCH_UP_EVENT = (1 << 10),
	NEMOTOOL_TOUCH_MOTION_EVENT = (1 << 11)
} NemoToolEventType;

typedef enum {
	NEMO_MOD_SHIFT_MASK = 0x01,
	NEMO_MOD_ALT_MASK = 0x02,
	NEMO_MOD_CONTROL_MASK = 0x04
} NemoModMask;

struct nemotask;
struct nemotool;

typedef void (*nemotask_dispatch_t)(struct nemotask *task, uint32_t events);

struct nemotask {
	nemotask_dispatch_t dispatch;
	struct nemolist link;
};

struct nemoqueue {
	struct nemotool *tool;

	struct wl_event_queue *queue;
};

struct nemoevent {
	uint64_t device;

	uint32_t serial;

	uint32_t time;
	uint32_t value;
	uint32_t state;

	float x, y;
};

struct nemoglobal {
	uint32_t name;
	uint32_t version;
	char *interface;

	struct nemolist link;
};

struct nemotool {
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_compositor *compositor;
	struct wl_subcompositor *subcompositor;
	struct presentation *presentation;
	struct nemo_seat *seat;
	struct nemo_sound *sound;
	struct nemo_pointer *pointer;
	struct nemo_keyboard *keyboard;
	struct nemo_touch *touch;
	struct nemo_shell *shell;
	struct wl_shm *shm;
	uint32_t formats;
	clockid_t clock_id;

	int epoll_fd;

	int display_fd;
	uint32_t display_events;
	struct nemotask display_task;

	struct nemolist global_list;
	struct nemolist output_list;

	struct nemolist idle_list;

	int running;

	struct {
		struct xkb_context *context;
		struct xkb_keymap *keymap;
		struct xkb_state *state;
		xkb_mod_mask_t control_mask;
		xkb_mod_mask_t alt_mask;
		xkb_mod_mask_t shift_mask;
	} xkb;

	uint32_t modifiers;

	void *userdata;
};

extern void nemotool_connect_wayland(struct nemotool *tool, const char *name);
extern void nemotool_disconnect_wayland(struct nemotool *tool);

extern void nemotool_dispatch(struct nemotool *tool);
extern void nemotool_run(struct nemotool *tool);
extern void nemotool_flush(struct nemotool *tool);
extern void nemotool_exit(struct nemotool *tool);

extern void nemotool_watch_fd(struct nemotool *tool, int fd, uint32_t events, struct nemotask *task);
extern void nemotool_unwatch_fd(struct nemotool *tool, int fd);
extern int nemotool_get_fd(struct nemotool *tool);

extern struct nemotool *nemotool_create(void);
extern void nemotool_destroy(struct nemotool *tool);

extern void nemotool_idle_task(struct nemotool *tool, struct nemotask *task);
extern void nemotool_remove_task(struct nemotool *tool, struct nemotask *task);

static inline struct wl_display *nemotool_get_display(struct nemotool *tool)
{
	return tool->display;
}

static inline struct wl_shm *nemotool_get_shm(struct nemotool *tool)
{
	return tool->shm;
}

static inline struct presentation *nemotool_get_presentation(struct nemotool *tool)
{
	return tool->presentation;
}

static inline uint32_t nemotool_get_formats(struct nemotool *tool)
{
	if (tool->formats == 0)
		nemotool_dispatch(tool);

	return tool->formats;
}

static inline clockid_t nemotool_get_clock_id(struct nemotool *tool)
{
	return tool->clock_id;
}

static inline int nemotool_is_running(struct nemotool *tool)
{
	return tool->running;
}

static inline void nemotool_set_userdata(struct nemotool *tool, void *data)
{
	tool->userdata = data;
}

static inline void *nemotool_get_userdata(struct nemotool *tool)
{
	return tool->userdata;
}

extern struct nemoqueue *nemotool_create_queue(struct nemotool *tool);
extern void nemotool_destroy_queue(struct nemoqueue *queue);

extern int nemotool_dispatch_queue(struct nemoqueue *queue);

extern uint32_t nemotool_get_keysym(struct nemotool *tool, uint32_t code);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
