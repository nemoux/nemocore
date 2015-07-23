#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <dataselection.h>
#include <datadevice.h>
#include <dataoffer.h>
#include <seat.h>
#include <keyboard.h>
#include <view.h>
#include <canvas.h>
#include <nemomisc.h>

static void dataselection_handle_data_source_destroy(struct wl_listener *listener, void *data)
{
	struct nemoseat *seat = (struct nemoseat *)container_of(listener, struct nemoseat, selection.data_source_listener);
	struct wl_resource *device;
	struct nemoview *focus = seat->selection.focus;

	seat->selection.data_source = NULL;

	if (focus && focus->canvas && focus->canvas->resource) {
		device = wl_resource_find_for_client(&seat->drag_resource_list, wl_resource_get_client(focus->canvas->resource));
		if (device) {
			wl_data_device_send_selection(device, NULL);
		}

		seat->selection.focus = NULL;
	}

	wl_signal_emit(&seat->selection.signal, seat);
}

void dataselection_set_selection(struct nemoseat *seat, struct nemodatasource *source, uint32_t serial)
{
	struct nemokeyboard *keyboard;
	struct wl_resource *device, *offer;
	struct nemoview *focus = NULL;

	if (seat->selection.data_source &&
			seat->selection.serial - serial < UINT32_MAX / 2)
		return;

	if (seat->selection.data_source) {
		seat->selection.data_source->cancel(seat->selection.data_source);
		wl_list_remove(&seat->selection.data_source_listener.link);
		seat->selection.data_source = NULL;
	}

	seat->selection.data_source = source;
	seat->selection.serial = serial;

	keyboard = nemoseat_get_keyboard_by_focus_serial(seat, serial);
	if (keyboard != NULL) {
		focus = keyboard->focus;
		if (focus && focus->canvas && focus->canvas->resource) {
			device = wl_resource_find_for_client(&seat->drag_resource_list, wl_resource_get_client(focus->canvas->resource));
			if (device && source) {
				offer = dataoffer_create(seat->selection.data_source, device);
				wl_data_device_send_selection(device, offer);
			} else if (device) {
				wl_data_device_send_selection(device, NULL);
			}

			seat->selection.focus = focus;
		}
	}

	wl_signal_emit(&seat->selection.signal, seat);

	if (source != NULL) {
		seat->selection.data_source_listener.notify = dataselection_handle_data_source_destroy;
		wl_signal_add(&source->destroy_signal, &seat->selection.data_source_listener);
	}
}
