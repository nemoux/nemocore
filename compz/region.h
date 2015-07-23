#ifndef	__NEMO_REGION_H__
#define	__NEMO_REGION_H__

#include <pixman.h>

struct nemoregion {
	struct wl_resource *resource;
	pixman_region32_t region;
};

extern struct nemoregion *nemoregion_create(struct wl_client *client, struct wl_resource *compositor_resource, uint32_t id);
extern void nemoregion_destroy(struct nemoregion *region);

#endif
