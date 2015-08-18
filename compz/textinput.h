#ifndef	__NEMO_TEXTINPUT_H__
#define	__NEMO_TEXTINPUT_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <pixman.h>

struct nemocompz;
struct nemocanvas;
struct inputmethod;

struct textmanager {
	struct wl_global *global;
	struct wl_listener destroy_listener;

	struct nemocompz *compz;
};

struct textinput {
	struct nemocompz *compz;

	struct wl_resource *resource;

	struct wl_list inputmethod_list;

	struct nemocanvas *canvas;

	pixman_box32_t cursor;
	int input_panel_visible;
};

extern struct textmanager *textinput_create_manager(struct nemocompz *compz);

extern void textinput_deactivate_input_method(struct textinput *textinput, struct inputmethod *inputmethod);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
