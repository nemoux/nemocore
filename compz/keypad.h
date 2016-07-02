#ifndef	__NEMO_KEYPAD_H__
#define	__NEMO_KEYPAD_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>
#include <keymap.h>
#include <keyboard.h>

struct nemoseat;
struct nemofocus;

struct nemokeypad_grab;

struct nemokeypad_grab_interface {
	void (*key)(struct nemokeypad_grab *grab, uint32_t time, uint32_t key, uint32_t state);
	void (*modifiers)(struct nemokeypad_grab *grab, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group);
	void (*cancel)(struct nemokeypad_grab *grab);
};

struct nemokeypad_grab {
	const struct nemokeypad_grab_interface *interface;
	struct nemokeypad *keypad;
};

struct nemokeypad {
	struct nemoseat *seat;

	uint32_t id;

	struct wl_signal destroy_signal;

	struct nemoview *focus;
	struct nemoview *focused;
	uint32_t focus_serial;
	struct wl_listener focus_resource_listener;
	struct wl_listener focus_view_listener;

	struct wl_list link;

	struct nemoxkb *xkb;

	struct nemokeypad_grab *grab;
	struct nemokeypad_grab default_grab;
	uint32_t grab_key;
	uint32_t grab_serial;
	uint32_t grab_time;
};

extern struct nemokeypad *nemokeypad_create(struct nemoseat *seat);
extern void nemokeypad_destroy(struct nemokeypad *keypad);

extern void nemokeypad_set_focus(struct nemokeypad *keypad, struct nemoview *view);

extern void nemokeypad_notify_key(struct nemokeypad *keypad, uint32_t time, uint32_t key, enum wl_keyboard_key_state state);

extern void nemokeypad_start_grab(struct nemokeypad *keypad, struct nemokeypad_grab *grab);
extern void nemokeypad_end_grab(struct nemokeypad *keypad);
extern void nemokeypad_cancel_grab(struct nemokeypad *keypad);

extern int nemokeypad_is_caps_on(struct nemokeypad *keypad);
extern int nemokeypad_is_shift_on(struct nemokeypad *keypad);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
