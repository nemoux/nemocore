#ifndef	__NEMO_INPUTMETHOD_H__
#define	__NEMO_INPUTMETHOD_H__

struct nemoseat;
struct textinput;
struct textbackend;
struct inputcontext;
struct nemokeyboard;

struct inputmethod {
	struct wl_resource *binding;
	struct wl_global *global;
	struct wl_listener destroy_listener;

	struct nemoseat *seat;
	struct textinput *model;
	struct inputcontext *context;

	struct wl_list link;

	struct wl_listener keyboard_focus_listener;

	struct textbackend *textbackend;
};

struct inputcontext {
	struct wl_resource *resource;

	struct textinput *model;
	struct inputmethod *inputmethod;

	struct wl_list link;

	struct wl_resource *keyboard;
};

extern struct inputmethod *inputmethod_create(struct nemoseat *seat, struct textbackend *textbackend);

extern void inputmethod_prepare_keyboard_grab(struct nemokeyboard *keyboard);
extern void inputmethod_end_keyboard_grab(struct inputcontext *context);

#endif
