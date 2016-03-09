#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <compz.h>
#include <canvas.h>
#include <content.h>
#include <view.h>
#include <seat.h>
#include <pointer.h>
#include <touch.h>
#include <datadevice.h>
#include <datadrag.h>
#include <dataselection.h>
#include <dataoffer.h>
#include <nemomisc.h>

static void datadevice_accept_client_source(struct nemodatasource *source, uint32_t time, const char *mime_type)
{
	wl_data_source_send_target(source->resource, mime_type);
}

static void datadevice_send_client_source(struct nemodatasource *source, const char *mime_type, int32_t fd)
{
	wl_data_source_send_send(source->resource, mime_type, fd);

	close(fd);
}

static void datadevice_cancel_client_source(struct nemodatasource *source)
{
	wl_data_source_send_cancelled(source->resource);
}

static void data_source_offer(struct wl_client *client, struct wl_resource *resource, const char *type)
{
	struct nemodatasource *source = (struct nemodatasource *)wl_resource_get_user_data(resource);
	char **p;

	p = wl_array_add(&source->mime_types, sizeof(char *));
	if (p != NULL)
		*p = strdup(type);
	if (p == NULL || *p == NULL)
		wl_resource_post_no_memory(resource);
}

static void data_source_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static struct wl_data_source_interface data_source_implementation = {
	data_source_offer,
	data_source_destroy
};

static void datadevice_unbind_data_source(struct wl_resource *resource)
{
	struct nemodatasource *source = (struct nemodatasource *)wl_resource_get_user_data(resource);
	char **p;

	wl_signal_emit(&source->destroy_signal, source);

	wl_array_for_each(p, &source->mime_types)
		free(*p);

	wl_array_release(&source->mime_types);

	free(source);
}

static void data_device_start_drag(struct wl_client *client, struct wl_resource *resource, struct wl_resource *source_resource, struct wl_resource *origin_resource, struct wl_resource *icon_resource, uint32_t serial)
{
	struct nemoseat *seat = (struct nemoseat *)wl_resource_get_user_data(resource);
	struct nemocanvas *origin = (struct nemocanvas *)wl_resource_get_user_data(origin_resource);
	struct nemodatasource *source;
	struct nemocanvas *icon = NULL;
	struct nemopointer *pointer;
	struct touchpoint *tp;
	int r = -1;

	pointer = nemoseat_get_pointer_by_grab_serial(seat, serial);
	if (pointer == NULL || pointer->button_count != 1 || pointer->grab_serial != serial ||
			pointer->focus == NULL || pointer->focus->canvas != origin)
		pointer = NULL;

	tp = nemoseat_get_touchpoint_by_grab_serial(seat, serial);
	if (tp == NULL || tp->grab_serial != serial || tp->focus == NULL || tp->focus->canvas != origin)
		tp = NULL;

	if (pointer == NULL && tp == NULL)
		return;

	if (source_resource == NULL) {
		wl_resource_post_no_memory(resource);
		return;
	}

	if (source_resource != NULL)
		source = (struct nemodatasource *)wl_resource_get_user_data(source_resource);
	if (icon_resource != NULL)
		icon = (struct nemocanvas *)wl_resource_get_user_data(icon_resource);
	if (icon != NULL && icon->configure != NULL) {
		wl_resource_post_error(icon_resource,
				WL_DISPLAY_ERROR_INVALID_OBJECT,
				"icon->configure is already set");
		return;
	}

	if (pointer != NULL) {
		r = datadrag_start_pointer_grab(pointer, source, icon, client);
	} else if (tp != NULL) {
		r = datadrag_start_touchpoint_grab(tp, source, icon, client);
	}

	if (r < 0) {
		wl_resource_post_no_memory(resource);
	}
}

static void data_device_set_selection(struct wl_client *client, struct wl_resource *resource, struct wl_resource *source_resource, uint32_t serial)
{
	if (source_resource == NULL)
		return;

	dataselection_set_selection(
			(struct nemoseat *)wl_resource_get_user_data(resource),
			(struct nemodatasource *)wl_resource_get_user_data(source_resource),
			serial);
}

static void data_device_release(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static const struct wl_data_device_interface data_device_implementation = {
	data_device_start_drag,
	data_device_set_selection,
	data_device_release
};

static void datadevice_unbind_data_device(struct wl_resource *resource)
{
	wl_list_remove(wl_resource_get_link(resource));
}

static void datadevice_manager_create_data_source(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
	struct nemodatasource *source;

	source = (struct nemodatasource *)malloc(sizeof(struct nemodatasource));
	if (source == NULL) {
		wl_resource_post_no_memory(resource);
		return;
	}
	memset(source, 0, sizeof(struct nemodatasource));

	wl_signal_init(&source->destroy_signal);

	source->accept = datadevice_accept_client_source;
	source->send = datadevice_send_client_source;
	source->cancel = datadevice_cancel_client_source;

	wl_array_init(&source->mime_types);

	source->resource = wl_resource_create(client, &wl_data_source_interface, 1, id);
	if (source->resource == NULL) {
		wl_resource_post_no_memory(resource);
		return;
	}

	wl_resource_set_implementation(source->resource, &data_source_implementation, source, datadevice_unbind_data_source);
}

static void datadevice_manager_get_data_device(struct wl_client *client, struct wl_resource *manager_resource, uint32_t id, struct wl_resource *seat_resource)
{
	struct nemoseat *seat = (struct nemoseat *)wl_resource_get_user_data(seat_resource);
	struct wl_resource *resource;

	resource = wl_resource_create(client, &wl_data_device_interface, 1, id);
	if (resource == NULL) {
		wl_resource_post_no_memory(manager_resource);
		return;
	}

	wl_list_insert(&seat->drag_resource_list, wl_resource_get_link(resource));
	wl_resource_set_implementation(resource, &data_device_implementation, seat, datadevice_unbind_data_device);
}

static const struct wl_data_device_manager_interface datadevice_manager_implementation = {
	datadevice_manager_create_data_source,
	datadevice_manager_get_data_device
};

static void datadevice_bind_manager(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct wl_resource *resource;

	resource = wl_resource_create(client, &wl_data_device_manager_interface, version, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource, &datadevice_manager_implementation, NULL, NULL);
}

int datadevice_manager_init(struct wl_display *display)
{
	if (!wl_global_create(display, &wl_data_device_manager_interface, 2, NULL, datadevice_bind_manager))
		return -1;

	return 0;
}

void datadevice_set_focus(struct nemoseat *seat, struct nemoview *view)
{
	struct wl_resource *device, *offer;
	struct nemodatasource *source;

	if (view == NULL || view->canvas == NULL || view->canvas->resource == NULL)
		return;

	device = wl_resource_find_for_client(&seat->drag_resource_list, wl_resource_get_client(view->canvas->resource));
	if (device == NULL)
		return;

	source = seat->selection.data_source;
	if (source != NULL) {
		offer = dataoffer_create(source, device);
		wl_data_device_send_selection(device, offer);
	}
}
