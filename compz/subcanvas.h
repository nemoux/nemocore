#ifndef	__NEMO_SUBCANVAS_H__
#define	__NEMO_SUBCANVAS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <pixman.h>

struct nemocanvas;

struct nemosubcanvas {
	struct wl_resource *resource;

	struct nemocanvas *canvas;
	struct wl_listener canvas_destroy_listener;

	struct nemocanvas *parent;
	struct wl_listener parent_destroy_listener;
	struct wl_list parent_link;

	struct nemoview *view;

	int has_cached_data;
	struct nemocanvas_state cached;
	struct nemobuffer_reference buffer_reference;

	int sync;
};

extern struct nemosubcanvas *nemosubcanvas_create(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface_resource, struct wl_resource *parent_resource);
extern void nemosubcanvas_destroy(struct nemosubcanvas *sub);

extern struct nemosubcanvas *nemosubcanvas_get_subcanvas(struct nemocanvas *canvas);
extern struct nemocanvas *nemosubcanvas_get_main_canvas(struct nemocanvas *canvas);

extern void nemosubcanvas_init_cache(struct nemosubcanvas *sub);
extern void nemosubcanvas_exit_cache(struct nemosubcanvas *sub);

extern void nemosubcanvas_link_canvas(struct nemosubcanvas *sub, struct nemocanvas *canvas);
extern void nemosubcanvas_link_parent(struct nemosubcanvas *sub, struct nemocanvas *parent);
extern void nemosubcanvas_unlink_parent(struct nemosubcanvas *sub);

extern void nemosubcanvas_commit_from_cache(struct nemosubcanvas *sub);
extern void nemosubcanvas_commit_to_cache(struct nemosubcanvas *sub);
extern int nemosubcanvas_is_sync(struct nemosubcanvas *sub);
extern void nemosubcanvas_commit(struct nemosubcanvas *sub);
extern void nemosubcanvas_commit_sync(struct nemosubcanvas *sub);
extern void nemosubcanvas_commit_parent(struct nemosubcanvas *sub, int sync);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
