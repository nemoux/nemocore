#ifndef	__NEMO_TOUCH_H__
#define	__NEMO_TOUCH_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemohash.h>
#include <input.h>

struct nemoseat;
struct nemotouch;
struct tuionode;

struct touchpoint_grab;

struct touchpoint_grab_interface {
	void (*down)(struct touchpoint_grab *grab, uint32_t time, uint64_t touchid, float x, float y);
	void (*up)(struct touchpoint_grab *grab, uint32_t time, uint64_t touchid);
	void (*motion)(struct touchpoint_grab *grab, uint32_t time, uint64_t touchid, float x, float y);
	void (*frame)(struct touchpoint_grab *grab);
	void (*cancel)(struct touchpoint_grab *grab);
};

struct touchpoint_grab {
	const struct touchpoint_grab_interface *interface;
	struct touchpoint *touchpoint;
};

struct touchpoint {
	struct nemotouch *touch;

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

	void *binding;
};

struct touchtaps {
	uint64_t *ids;
	double *points;

	int ntaps, staps;
};

struct touchnode {
	struct inputnode base;

	struct nemocompz *compz;

	struct nemotouch *touch;

	struct wl_list link;
};

struct nemotouch {
	struct nemoseat *seat;
	struct inputnode *node;

	struct wl_list link;

	struct wl_list touchpoint_list;
	struct nemohash *touchpoint_hash;

	struct wl_event_source *timeout;
};

extern int nemotouch_bind_wayland(struct wl_client *client, struct wl_resource *seat_resource, uint32_t id);
extern int nemotouch_bind_nemo(struct wl_client *client, struct wl_resource *seat_resource, uint32_t id);

extern struct nemotouch *nemotouch_create(struct nemoseat *seat, struct inputnode *node);
extern void nemotouch_destroy(struct nemotouch *touch);

extern struct touchpoint *nemotouch_get_touchpoint_by_id(struct nemotouch *touch, uint64_t id);
extern struct touchpoint *nemotouch_get_touchpoint_by_serial(struct nemotouch *touch, uint32_t serial);

extern void nemotouch_notify_down(struct nemotouch *touch, uint32_t time, int id, float x, float y);
extern void nemotouch_notify_up(struct nemotouch *touch, uint32_t time, int id);
extern void nemotouch_notify_motion(struct nemotouch *touch, uint32_t time, int id, float x, float y);
extern void nemotouch_notify_frame(struct nemotouch *touch, int id);

extern void nemotouch_flush_tuio(struct tuionode *node);

extern void touchpoint_move(struct touchpoint *tp, float x, float y);

extern void touchpoint_set_focus(struct touchpoint *tp, struct nemoview *view);

extern void touchpoint_start_grab(struct touchpoint *tp, struct touchpoint_grab *grab);
extern void touchpoint_end_grab(struct touchpoint *tp);
extern void touchpoint_cancel_grab(struct touchpoint *tp);

extern void touchpoint_update_grab(struct touchpoint *tp);

extern struct touchnode *nemotouch_create_node(struct nemocompz *compz, const char *devnode);
extern void nemotouch_destroy_node(struct touchnode *node);

extern struct touchtaps *nemotouch_create_taps(int max);
extern void nemotouch_destroy_taps(struct touchtaps *taps);

extern void nemotouch_attach_tap(struct touchtaps *taps, uint64_t id, double x, double y);

extern void nemotouch_flush_taps(struct touchnode *node, struct touchtaps *taps);

extern void nemotouch_bypass_event(struct nemocompz *compz, int32_t touchid, float sx, float sy);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
