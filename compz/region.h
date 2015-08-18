#ifndef	__NEMO_REGION_H__
#define	__NEMO_REGION_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <pixman.h>

struct nemoregion {
	struct wl_resource *resource;
	pixman_region32_t region;
};

extern struct nemoregion *nemoregion_create(struct wl_client *client, struct wl_resource *compositor_resource, uint32_t id);
extern void nemoregion_destroy(struct nemoregion *region);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
