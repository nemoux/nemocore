#ifndef	__EVDEV_BACKEND_H__
#define	__EVDEV_BACKEND_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <libudev.h>
#include <backend.h>

struct evdevbackend {
	struct nemobackend base;

	struct nemocompz *compz;

	struct udev *udev;
	struct udev_monitor *udev_monitor;
	struct wl_event_source *udev_monitor_source;
};

extern struct nemobackend *evdevbackend_create(struct nemocompz *compz, const char *args);
extern void evdevbackend_destroy(struct nemobackend *base);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
