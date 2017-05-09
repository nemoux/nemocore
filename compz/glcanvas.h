#ifndef	__GL_CANVAS_H__
#define	__GL_CANVAS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <EGL/egl.h>

#include <pixman.h>

struct nemorenderer;
struct nemobuffer;
struct nemocanvas;

extern void glrenderer_prepare_buffer(struct nemorenderer *base, struct nemobuffer *buffer);
extern void glrenderer_attach_canvas(struct nemorenderer *base, struct nemocanvas *canvas);
extern void glrenderer_flush_canvas(struct nemorenderer *base, struct nemocanvas *canvas);
extern int glrenderer_read_canvas(struct nemorenderer *base, struct nemocanvas *canvas, pixman_format_code_t format, void *pixels);
extern void *glrenderer_get_canvas_buffer(struct nemorenderer *base, struct nemocanvas *canvas);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
