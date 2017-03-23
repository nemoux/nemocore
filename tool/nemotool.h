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
	NEMOTOOL_KEYBOARD_LAYOUT_EVENT = (1 << 9),
	NEMOTOOL_TOUCH_DOWN_EVENT = (1 << 10),
	NEMOTOOL_TOUCH_UP_EVENT = (1 << 11),
	NEMOTOOL_TOUCH_MOTION_EVENT = (1 << 12),
	NEMOTOOL_TOUCH_PRESSURE_EVENT = (1 << 13)
} NemoToolEventType;

typedef enum {
	NEMOMOD_SHIFT_MASK = 0x01,
	NEMOMOD_ALT_MASK = 0x02,
	NEMOMOD_CONTROL_MASK = 0x04
} NemoModMask;

struct nemotool;
struct eglcontext;

typedef void (*nemotool_dispatch_source_t)(void *data, const char *events);
typedef void (*nemotool_dispatch_idle_t)(void *data);
typedef void (*nemotool_dispatch_global_t)(struct nemotool *tool, uint32_t id, const char *interface, uint32_t version);

struct nemosource {
	nemotool_dispatch_source_t dispatch;
	void *data;

	int fd;

	struct nemolist link;
};

struct nemoidle {
	nemotool_dispatch_idle_t dispatch;
	void *data;

	struct nemolist link;
};

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

	const char *name;
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
	struct nemo_pointer *pointer;
	struct nemo_keyboard *keyboard;
	struct nemo_touch *touch;
	struct nemo_sound *sound;
	struct nemo_shell *shell;
	struct nemo_client *client;
	struct wl_shm *shm;
	uint32_t formats;
	clockid_t clock_id;

	uint32_t compositor_version;

	struct eglcontext *eglcontext;

	int epoll_fd;
	int display_fd;

	nemotool_dispatch_global_t dispatch_global;

	struct nemolist global_list;
	struct nemolist output_list;

	struct nemolist idle_list;
	struct nemolist source_list;

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
extern int nemotool_roundtrip(struct nemotool *tool);
extern void nemotool_exit(struct nemotool *tool);

extern int nemotool_watch_source(struct nemotool *tool, int fd, const char *events, nemotool_dispatch_source_t dispatch, void *data);
extern void nemotool_unwatch_source(struct nemotool *tool, int fd);
extern void nemotool_change_source(struct nemotool *tool, int fd, const char *events);
extern int nemotool_get_fd(struct nemotool *tool);

extern struct nemotool *nemotool_get_instance(void);

extern struct nemotool *nemotool_create(void);
extern void nemotool_destroy(struct nemotool *tool);

extern int nemotool_dispatch_idle(struct nemotool *tool, nemotool_dispatch_idle_t dispatch, void *data);

extern uint32_t nemotool_get_keysym(struct nemotool *tool, uint32_t code);

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

static inline uint32_t nemotool_get_compositor_version(struct nemotool *tool)
{
	return tool->compositor_version;
}

static inline int nemotool_is_running(struct nemotool *tool)
{
	return tool->running;
}

static inline void nemotool_set_dispatch_global(struct nemotool *tool, nemotool_dispatch_global_t dispatch)
{
	tool->dispatch_global = dispatch;
}

static inline uint32_t nemotool_get_modifiers(struct nemotool *tool)
{
	return tool->modifiers;
}

static inline int nemotool_has_control_key(struct nemotool *tool)
{
	return tool->modifiers & NEMOMOD_CONTROL_MASK;
}

static inline int nemotool_has_alt_key(struct nemotool *tool)
{
	return tool->modifiers & NEMOMOD_ALT_MASK;
}

static inline void nemotool_set_userdata(struct nemotool *tool, void *data)
{
	tool->userdata = data;
}

static inline void *nemotool_get_userdata(struct nemotool *tool)
{
	return tool->userdata;
}

extern void nemotool_keyboard_enter(struct nemotool *tool);
extern void nemotool_keyboard_leave(struct nemotool *tool);
extern void nemotool_keyboard_key(struct nemotool *tool, uint32_t time, uint32_t key, int pressed);
extern void nemotool_keyboard_layout(struct nemotool *tool, const char *name);

extern void nemotool_touch_bypass(struct nemotool *tool, int32_t id, float x, float y);

extern void nemotool_client_alive(struct nemotool *tool, uint32_t timeout);

static inline uint64_t nemoevent_get_device(struct nemoevent *event)
{
	return event->device;
}

static inline uint32_t nemoevent_get_serial(struct nemoevent *event)
{
	return event->serial;
}

static inline uint32_t nemoevent_get_time(struct nemoevent *event)
{
	return event->time;
}

static inline uint32_t nemoevent_get_value(struct nemoevent *event)
{
	return event->value;
}

static inline uint32_t nemoevent_get_state(struct nemoevent *event)
{
	return event->state;
}

static inline float nemoevent_get_canvas_x(struct nemoevent *event)
{
	return event->x;
}

static inline float nemoevent_get_canvas_y(struct nemoevent *event)
{
	return event->y;
}

static inline float nemoevent_get_canvas_z(struct nemoevent *event)
{
	return event->z;
}

static inline float nemoevent_get_rotate(struct nemoevent *event)
{
	return event->r;
}

static inline float nemoevent_get_pressure(struct nemoevent *event)
{
	return event->p;
}

static inline float nemoevent_get_global_x(struct nemoevent *event)
{
	return event->gx;
}

static inline float nemoevent_get_global_y(struct nemoevent *event)
{
	return event->gy;
}

static inline const char *nemoevent_get_name(struct nemoevent *event)
{
	return event->name;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
