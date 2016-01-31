#ifndef	__NEMO_STICK_H__
#define	__NEMO_STICK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <input.h>

struct nemoseat;
struct nemoview;
struct nemoactor;

struct nemostick {
	struct nemoseat *seat;
	struct inputnode *node;

	uint32_t id;

	struct wl_signal destroy_signal;

	struct nemoview *focus;
	struct nemoview *focused;
	uint32_t focus_serial;
	struct wl_listener focus_view_listener;
	struct wl_listener focus_resource_listener;

	struct wl_list link;
};

extern int nemostick_bind_nemo(struct wl_client *client, struct wl_resource *seat_resource, uint32_t id);

extern struct nemostick *nemostick_create(struct nemoseat *seat, struct inputnode *node);
extern void nemostick_destroy(struct nemostick *stick);

extern void nemostick_set_focus(struct nemostick *stick, struct nemoview *view);

extern void nemostick_notify_translate(struct nemostick *stick, uint32_t time, float x, float y, float z);
extern void nemostick_notify_rotate(struct nemostick *stick, uint32_t time, float rx, float ry, float rz);
extern void nemostick_notify_button(struct nemostick *stick, uint32_t time, int32_t button, enum wl_pointer_button_state state);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
