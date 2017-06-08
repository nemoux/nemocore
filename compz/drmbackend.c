#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <time.h>
#include <gbm.h>
#include <drm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <wayland-server.h>

#include <drmbackend.h>
#include <drmnode.h>
#include <compz.h>
#include <session.h>
#include <nemomisc.h>
#include <nemolog.h>

static int drmbackend_add_udev(struct drmbackend *drm, struct udev_device *device)
{
	struct nemocompz *compz = drm->compz;
	struct drmnode *node;
	const char *devnode;
	uint32_t nodeid = 0;
	int fd;

	devnode = udev_device_get_devnode(device);

	nemolog_message("DRM", "add udev device %s\n", devnode);

	fd = nemosession_open(compz->session, devnode, O_RDWR | O_NONBLOCK);
	if (fd < 0) {
		nemolog_error("DRM", "failed to open device '%s'\n", devnode);
		return -1;
	}

	sscanf(devnode, "/dev/dri/card%d", &nodeid);

	node = drm_create_node(compz, nodeid, devnode, fd);
	if (node == NULL)
		goto err1;

#ifdef NEMOUX_DRM_PAGEFLIP_TIMEOUT
	node->pageflip_timeout = NEMOUX_DRM_PAGEFLIP_TIMEOUT;
#endif

	return 0;

err1:
	nemosession_close(compz->session, fd);

	return -1;
}

static int drmbackend_add_dev(struct drmbackend *drm, const char *devnode)
{
	struct nemocompz *compz = drm->compz;
	struct drmnode *node;
	uint32_t nodeid = 0;
	int fd;

	nemolog_message("DRM", "add device %s\n", devnode);

	fd = nemosession_open(compz->session, devnode, O_RDWR | O_NONBLOCK);
	if (fd < 0) {
		nemolog_error("DRM", "failed to open device '%s'\n", devnode);
		return -1;
	}

	sscanf(devnode, "/dev/dri/card%d", &nodeid);

	node = drm_create_node(compz, nodeid, devnode, fd);
	if (node == NULL)
		goto err1;

#ifdef NEMOUX_DRM_PAGEFLIP_TIMEOUT
	node->pageflip_timeout = NEMOUX_DRM_PAGEFLIP_TIMEOUT;
#endif

	return 0;

err1:
	nemosession_close(compz->session, fd);

	return -1;
}

static int drmbackend_scan_devs(struct drmbackend *drm)
{
	struct udev_enumerate *e;
	struct udev_list_entry *entry;
	struct udev_device *device;
	const char *path, *devtype;

	e = udev_enumerate_new(drm->udev);
	udev_enumerate_add_match_subsystem(e, "drm");
	udev_enumerate_add_match_sysname(e, "card[0-9]*");

	udev_enumerate_scan_devices(e);

	udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(e)) {
		path = udev_list_entry_get_name(entry);
		device = udev_device_new_from_syspath(drm->udev, path);
		if (device == NULL)
			continue;

		devtype = udev_device_get_devtype(device);
		if (devtype != NULL && !strcmp(devtype, "drm_minor")) {
			drmbackend_add_udev(drm, device);
		}

		udev_device_unref(device);
	}

	udev_enumerate_unref(e);

	return 0;
}

static int drmbackend_dispatch_udev(int fd, uint32_t mask, void *data)
{
	struct drmbackend *drm = (struct drmbackend *)data;
	struct udev_device *device;

	device = udev_monitor_receive_device(drm->udev_monitor);
	if (device == NULL)
		return -1;

	udev_device_unref(device);

	return 1;
}

static int drmbackend_monitor_devs(struct drmbackend *drm)
{
	int fd;

	drm->udev_monitor = udev_monitor_new_from_netlink(drm->udev, "udev");
	if (drm->udev_monitor == NULL)
		return -1;

	udev_monitor_filter_add_match_subsystem_devtype(drm->udev_monitor, "drm", NULL);

	if (udev_monitor_enable_receiving(drm->udev_monitor))
		goto err1;

	fd = udev_monitor_get_fd(drm->udev_monitor);
	drm->udev_monitor_source = wl_event_loop_add_fd(drm->compz->loop,
			fd,
			WL_EVENT_READABLE,
			drmbackend_dispatch_udev,
			drm);
	if (drm->udev_monitor_source == NULL)
		goto err1;

	return 0;

err1:
	udev_monitor_unref(drm->udev_monitor);
	drm->udev_monitor = NULL;

	return -1;
}

struct nemobackend *drmbackend_create(struct nemocompz *compz, const char *args)
{
	struct drmbackend *drm;

	drm = (struct drmbackend *)malloc(sizeof(struct drmbackend));
	if (drm == NULL)
		return NULL;
	memset(drm, 0, sizeof(struct drmbackend));

	drm->base.destroy = drmbackend_destroy;

	drm->compz = compz;
	drm->udev = udev_ref(compz->udev);

	if (args != NULL) {
		drmbackend_add_dev(drm, args);
	} else {
		drmbackend_monitor_devs(drm);
		drmbackend_scan_devs(drm);
	}

	wl_list_insert(&compz->backend_list, &drm->base.link);

	return &drm->base;
}

void drmbackend_destroy(struct nemobackend *base)
{
	struct drmbackend *drm = (struct drmbackend *)container_of(base, struct drmbackend, base);
	struct rendernode *node, *next;

	nemolog_message("DRM", "destroy drm backend\n");

	wl_list_remove(&base->link);

	wl_list_for_each_safe(node, next, &drm->compz->render_list, link) {
		drm_destroy_node((struct drmnode *)container_of(node, struct drmnode, base));
	}

	if (drm->udev_monitor_source != NULL)
		wl_event_source_remove(drm->udev_monitor_source);
	if (drm->udev_monitor != NULL)
		udev_monitor_unref(drm->udev_monitor);

	udev_unref(drm->udev);

	free(drm);
}
