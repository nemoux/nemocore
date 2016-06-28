#ifndef	__DRM_BACKEND_H__
#define	__DRM_BACKEND_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <libudev.h>
#include <compz.h>
#include <backend.h>

struct drmbackend {
	struct nemobackend base;

	struct nemocompz *compz;

	int drmfd;

	struct udev *udev;
	struct udev_monitor *udev_monitor;
	struct wl_event_source *udev_monitor_source;
};

extern struct nemobackend *drmbackend_create(struct nemocompz *compz, const char *args);
extern void drmbackend_destroy(struct nemobackend *base);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
