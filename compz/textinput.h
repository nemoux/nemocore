#ifndef	__NEMO_TEXTINPUT_H__
#define	__NEMO_TEXTINPUT_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <pixman.h>

struct nemocompz;
struct nemocanvas;

struct textmanager {
	struct wl_global *global;
	struct wl_listener destroy_listener;

	struct nemocompz *compz;
};

struct textinput {
	struct nemocompz *compz;

	struct wl_resource *resource;

	struct nemocanvas *canvas;
};

extern struct textmanager *textinput_create_manager(struct nemocompz *compz);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
