#ifndef	__NEMOUX_TALE_HELPER_H__
#define	__NEMOUX_TALE_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemotool.h>
#include <nemocanvas.h>

#include <nemotale.h>
#include <talenode.h>
#include <talepixman.h>
#include <talegl.h>
#include <taleevent.h>
#include <talegrab.h>
#include <talegesture.h>

extern void nemotale_attach_canvas(struct nemotale *tale, struct nemocanvas *canvas, nemotale_dispatch_event_t dispatch);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
