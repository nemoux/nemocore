#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <evdevbackend.h>
#include <evdevnode.h>
#include <compz.h>
#include <session.h>
#include <nemomisc.h>
#include <nemolog.h>

static int evdevbackend_add_device(struct evdevbackend *evdev, struct udev_device *device)
{
	struct nemocompz *compz = evdev->compz;
	struct evdevnode *node;
	const char *devnode;
	int fd;

	devnode = udev_device_get_devnode(device);

	fd = nemosession_open(compz->session, devnode, O_RDWR | O_NONBLOCK);
	if (fd < 0) {
		nemolog_error("EVDEV", "failed to open device '%s'\n", devnode);
		return -1;
	}

	node = evdev_create_node(compz, devnode, fd);
	if (node == NULL)
		goto err1;

	return 0;

err1:
	nemosession_close(compz->session, fd);

	return -1;
}

static int evdevbackend_remove_device(struct evdevbackend *evdev, struct udev_device *device)
{
	struct nemocompz *compz = evdev->compz;
	struct evdevnode *node;
	const char *devnode;

	devnode = udev_device_get_devnode(device);

	wl_list_for_each(node, &compz->evdev_list, link) {
		if (strcmp(node->devpath, devnode) == 0) {
			evdev_destroy_node(node);

			return 1;
		}
	}

	return 0;
}

static int evdevbackend_scan_devices(struct evdevbackend *evdev)
{
	struct udev_enumerate *e;
	struct udev_list_entry *entry;
	struct udev_device *device;
	const char *path, *sysname;

	e = udev_enumerate_new(evdev->udev);
	udev_enumerate_add_match_subsystem(e, "input");

	udev_enumerate_scan_devices(e);

	udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(e)) {
		path = udev_list_entry_get_name(entry);
		device = udev_device_new_from_syspath(evdev->udev, path);
		if (device == NULL)
			continue;

		sysname = udev_device_get_sysname(device);
		if (sysname != NULL && !strncmp("event", sysname, 5)) {
			evdevbackend_add_device(evdev, device);
		}

		udev_device_unref(device);
	}

	udev_enumerate_unref(e);

	return 0;
}

static int evdevbackend_dispatch_udev(int fd, uint32_t mask, void *data)
{
	struct evdevbackend *evdev = (struct evdevbackend *)data;
	struct udev_device *device;
	const char *action, *sysname, *devnode;

	device = udev_monitor_receive_device(evdev->udev_monitor);
	if (device == NULL)
		return 1;

	action = udev_device_get_action(device);
	if (action == NULL)
		goto out;
	sysname = udev_device_get_sysname(device);
	if (sysname == NULL || strncmp("event", sysname, 5) != 0)
		goto out;

	if (!strcmp(action, "add")) {
		evdevbackend_add_device(evdev, device);
	} else if (!strcmp(action, "remove")) {
		evdevbackend_remove_device(evdev, device);
	}

out:
	udev_device_unref(device);

	return 1;
}

static int evdevbackend_monitor_devices(struct evdevbackend *evdev)
{
	int fd;

	evdev->udev_monitor = udev_monitor_new_from_netlink(evdev->udev, "udev");
	if (evdev->udev_monitor == NULL)
		return -1;

	udev_monitor_filter_add_match_subsystem_devtype(evdev->udev_monitor, "input", NULL);

	if (udev_monitor_enable_receiving(evdev->udev_monitor))
		goto err1;

	fd = udev_monitor_get_fd(evdev->udev_monitor);
	evdev->udev_monitor_source = wl_event_loop_add_fd(evdev->compz->loop,
			fd,
			WL_EVENT_READABLE,
			evdevbackend_dispatch_udev,
			evdev);
	if (evdev->udev_monitor_source == NULL)
		goto err1;

	return 0;

err1:
	udev_monitor_unref(evdev->udev_monitor);
	evdev->udev_monitor = NULL;

	return -1;
}

struct nemobackend *evdevbackend_create(struct nemocompz *compz, const char *args)
{
	struct evdevbackend *evdev;

	evdev = (struct evdevbackend *)malloc(sizeof(struct evdevbackend));
	if (evdev == NULL)
		return NULL;
	memset(evdev, 0, sizeof(struct evdevbackend));

	evdev->base.destroy = evdevbackend_destroy;

	evdev->compz = compz;
	evdev->udev = udev_ref(compz->udev);

	if (args == NULL || strstr(args, "noplug") == NULL)
		evdevbackend_monitor_devices(evdev);
	if (args == NULL || strstr(args, "noscan") == NULL)
		evdevbackend_scan_devices(evdev);

	wl_list_insert(&compz->backend_list, &evdev->base.link);

	return &evdev->base;
}

void evdevbackend_destroy(struct nemobackend *base)
{
	struct evdevbackend *evdev = (struct evdevbackend *)container_of(base, struct evdevbackend, base);
	struct evdevnode *node, *next;

	wl_list_remove(&base->link);

	wl_list_for_each_safe(node, next, &evdev->compz->evdev_list, link) {
		evdev_destroy_node(node);
	}

	if (evdev->udev_monitor_source != NULL)
		wl_event_source_remove(evdev->udev_monitor_source);
	if (evdev->udev_monitor != NULL)
		udev_monitor_unref(evdev->udev_monitor);

	udev_unref(evdev->udev);

	free(evdev);
}
