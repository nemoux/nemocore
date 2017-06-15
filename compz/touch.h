#ifndef	__NEMO_TOUCH_H__
#define	__NEMO_TOUCH_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <input.h>

typedef enum {
	TOUCHPOINT_DOWN_STATE = 0,
	TOUCHPOINT_MOTION_STATE = 1,
	TOUCHPOINT_UP_STATE = 2,
	TOUCHPOINT_LAST_STATE
} TouchPointState;

typedef enum {
	TOUCHPOINT_GRAB_FLAG = (1 << 0)
} TouchPointFlag;

struct nemoseat;
struct nemotouch;
struct tuio;

struct touchpoint_grab;

struct touchpoint_grab_interface {
	void (*down)(struct touchpoint_grab *grab, uint32_t time, uint64_t touchid, float x, float y);
	void (*up)(struct touchpoint_grab *grab, uint32_t time, uint64_t touchid);
	void (*motion)(struct touchpoint_grab *grab, uint32_t time, uint64_t touchid, float x, float y);
	void (*pressure)(struct touchpoint_grab *grab, uint32_t time, uint64_t touchid, float p);
	void (*frame)(struct touchpoint_grab *grab, uint32_t frameid);
	void (*cancel)(struct touchpoint_grab *grab);
};

struct touchpoint_grab {
	const struct touchpoint_grab_interface *interface;
	struct touchpoint *touchpoint;
};

struct touchpoint {
	struct nemotouch *touch;

	int state;

	uint32_t flags;

	uint64_t id;
	uint64_t gid;

	struct wl_list link;

	struct wl_signal destroy_signal;

	struct nemoview *focus;
	uint32_t focus_serial;
	struct wl_listener focus_view_listener;
	struct wl_listener focus_resource_listener;

	struct touchpoint_grab *grab;
	struct touchpoint_grab default_grab;

	float grab_x, grab_y;
	uint32_t grab_serial;
	uint32_t grab_time;

	float x, y;
	float p;

	void *binding;
};

struct touchnode;

struct touchnode {
	struct inputnode base;

	struct nemocompz *compz;

	struct nemotouch *touch;

	struct wl_list link;

	void *userdata;
};

struct nemotouch {
	struct nemoseat *seat;
	struct inputnode *node;

	uint32_t frame_count;

	struct wl_list link;

	struct wl_list touchpoint_list;
};

extern int nemotouch_bind_wayland(struct wl_client *client, struct wl_resource *seat_resource, uint32_t id);
extern int nemotouch_bind_nemo(struct wl_client *client, struct wl_resource *seat_resource, uint32_t id);

extern struct nemotouch *nemotouch_create(struct nemoseat *seat, struct inputnode *node);
extern void nemotouch_destroy(struct nemotouch *touch);

extern struct touchpoint *nemotouch_get_touchpoint_by_id(struct nemotouch *touch, uint64_t id);
extern struct touchpoint *nemotouch_get_touchpoint_list_by_id(struct nemotouch *touch, struct wl_list *list, uint64_t id);
extern struct touchpoint *nemotouch_get_touchpoint_by_serial(struct nemotouch *touch, uint32_t serial);

extern void nemotouch_notify_down(struct nemotouch *touch, uint32_t time, int id, float x, float y);
extern void nemotouch_notify_up(struct nemotouch *touch, uint32_t time, int id);
extern void nemotouch_notify_motion(struct nemotouch *touch, uint32_t time, int id, float x, float y);
extern void nemotouch_notify_pressure(struct nemotouch *touch, uint32_t time, int id, float p);
extern void nemotouch_notify_frame(struct nemotouch *touch, int id);
extern void nemotouch_notify_frames(struct nemotouch *touch);

extern void nemotouch_flush_tuio(struct tuio *tuio);

extern void touchpoint_down(struct touchpoint *tp, float x, float y);
extern void touchpoint_motion(struct touchpoint *tp, float x, float y);
extern void touchpoint_up(struct touchpoint *tp);
extern void touchpoint_pressure(struct touchpoint *tp, float p);

extern void touchpoint_set_focus(struct touchpoint *tp, struct nemoview *view);

extern void touchpoint_start_grab(struct touchpoint *tp, struct touchpoint_grab *grab);
extern void touchpoint_end_grab(struct touchpoint *tp);
extern void touchpoint_cancel_grab(struct touchpoint *tp);
extern void touchpoint_update_grab(struct touchpoint *tp);

extern struct touchnode *nemotouch_create_node(struct nemocompz *compz, const char *devnode);
extern void nemotouch_destroy_node(struct touchnode *node);
extern struct touchnode *nemotouch_get_node_by_name(struct nemocompz *compz, const char *name);

extern void nemotouch_bypass_event(struct nemocompz *compz, int32_t touchid, float sx, float sy);

extern void nemotouch_dump_touchpoint(struct nemotouch *touch);

static inline void *nemotouch_set_nodedata(struct touchnode *node, void *data)
{
	node->userdata = data;
}

static inline void *nemotouch_get_nodedata(struct touchnode *node)
{
	return node->userdata;
}

static inline void touchpoint_set_flags(struct touchpoint *tp, uint32_t flags)
{
	tp->flags |= flags;
}

static inline void touchpoint_put_flags(struct touchpoint *tp, uint32_t flags)
{
	tp->flags &= ~flags;
}

static inline int touchpoint_has_flags(struct touchpoint *tp, uint32_t flags)
{
	return tp->flags & flags;
}

static inline int touchpoint_has_flags_all(struct touchpoint *tp, uint32_t flags)
{
	return (tp->flags & flags) == flags;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
