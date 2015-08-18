#ifndef	__NEMO_DATAOFFER_H__
#define	__NEMO_DATAOFFER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct nemodatasource;

struct nemodataoffer {
	struct wl_resource *resource;
	struct nemodatasource *source;
	struct wl_listener source_destroy_listener;
};

extern struct wl_resource *dataoffer_create(struct nemodatasource *source, struct wl_resource *target);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
