#ifndef	__NEMO_DATAOFFER_H__
#define	__NEMO_DATAOFFER_H__

struct nemodatasource;

struct nemodataoffer {
	struct wl_resource *resource;
	struct nemodatasource *source;
	struct wl_listener source_destroy_listener;
};

extern struct wl_resource *dataoffer_create(struct nemodatasource *source, struct wl_resource *target);

#endif
