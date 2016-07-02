#ifndef	__NEMO_KEYBOARD_H__
#define	__NEMO_KEYBOARD_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>
#include <keymap.h>

#include <input.h>

struct nemoseat;
struct nemofocus;

struct nemokeyboard_grab;

struct nemokeyboard_grab_interface {
	void (*key)(struct nemokeyboard_grab *grab, uint32_t time, uint32_t key, uint32_t state);
	void (*modifiers)(struct nemokeyboard_grab *grab, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group);
	void (*cancel)(struct nemokeyboard_grab *grab);
};

struct nemokeyboard_grab {
	const struct nemokeyboard_grab_interface *interface;
	struct nemokeyboard *keyboard;
};

struct nemokeyboard {
	struct nemoseat *seat;
	struct inputnode *node;

	uint32_t id;

	struct wl_signal destroy_signal;

	struct nemoview *focus;
	struct nemoview *focused;
	uint32_t focus_serial;
	struct wl_listener focus_resource_listener;
	struct wl_listener focus_view_listener;

	struct wl_list link;

	struct nemoxkb *xkb;

	struct nemokeyboard_grab *grab;
	struct nemokeyboard_grab default_grab;
	uint32_t grab_key;
	uint32_t grab_serial;
	uint32_t grab_time;
	
	void *binding;
};

extern int nemokeyboard_bind_wayland(struct wl_client *client, struct wl_resource *seat_resource, uint32_t id);
extern int nemokeyboard_bind_nemo(struct wl_client *client, struct wl_resource *seat_resource, uint32_t id);

extern struct nemokeyboard *nemokeyboard_create(struct nemoseat *seat, struct inputnode *node);
extern void nemokeyboard_destroy(struct nemokeyboard *keyboard);

extern void nemokeyboard_set_focus(struct nemokeyboard *keyboard, struct nemoview *view);

extern void nemokeyboard_notify_key(struct nemokeyboard *keyboard, uint32_t time, uint32_t key, enum wl_keyboard_key_state state);

extern void nemokeyboard_start_grab(struct nemokeyboard *keyboard, struct nemokeyboard_grab *grab);
extern void nemokeyboard_end_grab(struct nemokeyboard *keyboard);
extern void nemokeyboard_cancel_grab(struct nemokeyboard *keyboard);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
