#ifndef	__NEMO_POINTER_H__
#define	__NEMO_POINTER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <input.h>

#define	NEMOPOINTER_DEFAULT_BINDING_MAX			(4)

struct nemoseat;
struct nemokeyboard;
struct nemoview;
struct nemobinding;

struct nemopointer_grab;

struct nemopointer_grab_interface {
	void (*focus)(struct nemopointer_grab *grab);
	void (*motion)(struct nemopointer_grab *grab, uint32_t time, float x, float y);
	void (*axis)(struct nemopointer_grab *grab, uint32_t time, uint32_t axis, float value);
	void (*button)(struct nemopointer_grab *grab, uint32_t time, uint32_t button, uint32_t state);
	void (*cancel)(struct nemopointer_grab *grab);
};

struct nemopointer_grab {
	const struct nemopointer_grab_interface *interface;
	struct nemopointer *pointer;
};

struct nemopointer {
	struct nemoseat *seat;
	struct inputnode *node;

	uint32_t id;

	struct wl_signal destroy_signal;

	struct nemoview *focus;
	struct nemoview *focused;
	uint32_t focus_serial;
	struct wl_listener focus_view_listener;
	struct wl_listener focus_resource_listener;

	struct nemokeyboard *keyboard;
	struct wl_listener keyboard_destroy_listener;

	struct wl_list link;

	struct nemopointer_grab *grab;
	struct nemopointer_grab default_grab;
	float grab_x, grab_y;
	uint32_t grab_button;
	uint32_t grab_serial;
	uint32_t grab_time;

	struct nemoview *sprite;
	struct wl_listener sprite_destroy_listener;
	int32_t hotspot_x, hotspot_y;

	float x, y;
	float sx, sy;
	uint32_t button_count;

	struct nemobinding *bindings[NEMOPOINTER_DEFAULT_BINDING_MAX];
};

extern int nemopointer_bind_wayland(struct wl_client *client, struct wl_resource *seat_resource, uint32_t id);
extern int nemopointer_bind_nemo(struct wl_client *client, struct wl_resource *seat_resource, uint32_t id);

extern struct nemopointer *nemopointer_create(struct nemoseat *seat, struct inputnode *node);
extern void nemopointer_destroy(struct nemopointer *pointer);

extern void nemopointer_move(struct nemopointer *pointer, float x, float y);
extern void nemopointer_set_focus(struct nemopointer *pointer, struct nemoview *view, float sx, float sy);

extern void nemopointer_notify_button(struct nemopointer *pointer, uint32_t time, int32_t button, enum wl_pointer_button_state state);
extern void nemopointer_notify_motion(struct nemopointer *pointer, uint32_t time, float dx, float dy);
extern void nemopointer_notify_motion_absolute(struct nemopointer *pointer, uint32_t time, float x, float y);
extern void nemopointer_notify_axis(struct nemopointer *pointer, uint32_t time, uint32_t axis, float value);

extern void nemopointer_set_keyboard(struct nemopointer *pointer, struct nemokeyboard *keyboard);
extern void nemopointer_set_keyboard_focus(struct nemopointer *pointer, struct nemoview *view);

extern void nemopointer_start_grab(struct nemopointer *pointer, struct nemopointer_grab *grab);
extern void nemopointer_end_grab(struct nemopointer *pointer);
extern void nemopointer_cancel_grab(struct nemopointer *pointer);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
