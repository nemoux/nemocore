#ifndef	__NEMOUX_SHOW_HELPER_H__
#define	__NEMOUX_SHOW_HELPER_H__

#include <nemotool.h>
#include <nemocanvas.h>

#include <nemotale.h>
#include <talenode.h>
#include <talepixman.h>
#include <talegl.h>
#include <taleevent.h>
#include <talegrab.h>
#include <talegesture.h>

#include <nemoshow.h>

struct showcontext {
	struct nemotool *tool;

	struct eglcontext *egl;
	struct eglcanvas *eglcanvas;

	struct nemocanvas *canvas;

	struct nemotale *tale;

	int32_t width, height;
};

#define NEMOSHOW_AT(show, at)			(((struct showcontext *)nemoshow_get_context(show))->at)

extern struct nemoshow *nemoshow_create_on_tale(struct nemotool *tool, int32_t width, int32_t height, nemotale_dispatch_event_t dispatch);
extern void nemoshow_destroy_on_tale(struct nemoshow *show);

#endif
