#ifndef	__PIXMAN_CANVAS_H__
#define	__PIXMAN_CANVAS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <pixman.h>

struct nemorenderer;
struct nemobuffer;
struct nemocanvas;

extern void pixmanrenderer_prepare_buffer(struct nemorenderer *base, struct nemobuffer *buffer);
extern void pixmanrenderer_attach_canvas(struct nemorenderer *base, struct nemocanvas *canvas);
extern void pixmanrenderer_flush_canvas(struct nemorenderer *base, struct nemocanvas *canvas);
extern int pixmanrenderer_read_canvas(struct nemorenderer *base, struct nemocanvas *canvas, pixman_format_code_t format, void *pixels);
extern void *pixmanrenderer_get_canvas_buffer(struct nemorenderer *base, struct nemocanvas *canvas);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
