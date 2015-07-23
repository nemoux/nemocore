#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <compz.h>
#include <shell.h>
#include <seat.h>
#include <xserver.h>
#include <xmanager.h>
#include <nemomisc.h>

static void nemoxmanager_set_selection(struct wl_listener *listener, void *data)
{
	struct nemoseat *seat = (struct nemoseat *)data;
	struct nemoxmanager *xmanager = container_of(listener, struct nemoxmanager, selection_listener);
}

void nemoxmanager_init_selection(struct nemoxmanager *xmanager)
{
	struct nemoseat *seat = xmanager->xserver->compz->seat;
	uint32_t values[1], mask;

	xmanager->selection_request.requestor = XCB_NONE;

	values[0] = XCB_EVENT_MASK_PROPERTY_CHANGE;

	xmanager->selection_window = xcb_generate_id(xmanager->conn);

	xcb_create_window(xmanager->conn,
			XCB_COPY_FROM_PARENT,
			xmanager->selection_window,
			xmanager->screen->root,
			0, 0,
			10, 10,
			0,
			XCB_WINDOW_CLASS_INPUT_OUTPUT,
			xmanager->screen->root_visual,
			XCB_CW_EVENT_MASK, values);

	xcb_set_selection_owner(xmanager->conn,
			xmanager->selection_window,
			xmanager->atom.clipboard_manager,
			XCB_TIME_CURRENT_TIME);

	mask =
		XCB_XFIXES_SELECTION_EVENT_MASK_SET_SELECTION_OWNER |
		XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_WINDOW_DESTROY |
		XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_CLIENT_CLOSE;

	xcb_xfixes_select_selection_input(xmanager->conn, xmanager->selection_window, xmanager->atom.clipboard, mask);

	xmanager->selection_listener.notify = nemoxmanager_set_selection;
	wl_signal_add(&seat->selection.signal, &xmanager->selection_listener);

	nemoxmanager_set_selection(&xmanager->selection_listener, seat);
}

void nemoxmanager_exit_selection(struct nemoxmanager *xmanager)
{
}

int nemoxmanager_handle_selection_event(struct nemoxmanager *xmanager, xcb_generic_event_t *event)
{
	switch (event->response_type & ~0x80) {
		case XCB_SELECTION_NOTIFY:
			return 1;

		case XCB_PROPERTY_NOTIFY:
			break;

		case XCB_SELECTION_REQUEST:
			return 1;
	}

	switch (event->response_type - xmanager->xfixes->first_event) {
		case XCB_XFIXES_SELECTION_NOTIFY:
			return 1;
	}

	return 0;
}
