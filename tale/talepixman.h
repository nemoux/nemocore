#ifndef	__NEMOTALE_PIXMAN_H__
#define	__NEMOTALE_PIXMAN_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>
#include <pixman.h>
#include <cairo.h>

#include <nemotale.h>
#include <talenode.h>
#include <nemolistener.h>

struct nemopmtale {
	pixman_image_t *image;
	cairo_surface_t *surface;
	void *data;
};

struct talepmnode {
	pixman_image_t *image;
	cairo_surface_t *surface;
	void *data;

	struct nemolistener destroy_listener;
};

extern struct nemotale *nemotale_create_pixman(void);
extern void nemotale_destroy_pixman(struct nemotale *tale);

extern int nemotale_attach_pixman(struct nemotale *tale, void *data, int32_t width, int32_t height, int32_t stride);
extern void nemotale_detach_pixman(struct nemotale *tale);
extern int nemotale_composite_pixman(struct nemotale *tale, pixman_region32_t *region);
extern int nemotale_composite_pixman_full(struct nemotale *tale);

extern struct talenode *nemotale_node_create_pixman(int32_t width, int32_t height);
extern int nemotale_node_resize_pixman(struct talenode *node, int32_t width, int32_t height);
extern void nemotale_node_fill_pixman(struct talenode *node, double r, double g, double b, double a);
extern int nemotale_node_set_viewport_pixman(struct talenode *node, int32_t width, int32_t height);

static inline pixman_image_t *nemotale_get_pixman(struct nemotale *tale)
{
	struct nemopmtale *context = (struct nemopmtale *)tale->pmcontext;

	return context->image;
}

static inline cairo_surface_t *nemotale_get_cairo(struct nemotale *tale)
{
	struct nemopmtale *context = (struct nemopmtale *)tale->pmcontext;

	return context->surface;
}

static inline void *nemotale_node_get_buffer(struct talenode *node)
{
	struct talepmnode *context = (struct talepmnode *)node->pmcontext;

	return context->data;
}

static inline pixman_image_t *nemotale_node_get_pixman(struct talenode *node)
{
	struct talepmnode *context = (struct talepmnode *)node->pmcontext;

	return context->image;
}

static inline cairo_surface_t *nemotale_node_get_cairo(struct talenode *node)
{
	struct talepmnode *context = (struct talepmnode *)node->pmcontext;

	return context->surface;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
