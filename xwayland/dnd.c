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
#include <dnd.h>
#include <nemomisc.h>

void nemoxmanager_init_dnd(struct nemoxmanager *xmanager)
{
	uint32_t mask;

	mask =
		XCB_XFIXES_SELECTION_EVENT_MASK_SET_SELECTION_OWNER |
		XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_WINDOW_DESTROY |
		XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_CLIENT_CLOSE;
	xcb_xfixes_select_selection_input(xmanager->conn, xmanager->selection_window, xmanager->atom.xdnd_selection, mask);
}

void nemoxmanager_exit_dnd(struct nemoxmanager *xmanager)
{
}

int nemoxmanager_handle_dnd_event(struct nemoxmanager *xmanager, xcb_generic_event_t *event)
{
	switch (event->response_type - xmanager->xfixes->first_event) {
		case XCB_XFIXES_SELECTION_NOTIFY:
			return 1;
	}

	switch (EVENT_TYPE(event)) {
		case XCB_CLIENT_MESSAGE:
			return 0;
	}

	return 0;
}
