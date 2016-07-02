#ifndef	__NEMO_INPUTMETHOD_H__
#define	__NEMO_INPUTMETHOD_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct nemoseat;
struct textinput;
struct inputcontext;
struct nemokeyboard;

struct inputmethod {
	struct wl_resource *binding;
	struct wl_global *global;
	struct wl_listener destroy_listener;

	struct nemoseat *seat;
	struct textinput *model;
	struct inputcontext *context;
};

struct inputcontext {
	struct wl_resource *resource;

	struct textinput *model;
	struct inputmethod *inputmethod;

	struct wl_resource *keyboard;
};

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
