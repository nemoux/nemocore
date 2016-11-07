#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <shell.h>
#include <grab.h>
#include <busycursor.h>
#include <compz.h>
#include <seat.h>
#include <pointer.h>
#include <picker.h>
#include <view.h>
#include <nemomisc.h>

static void busy_cursor_grab_focus(struct nemopointer_grab *base)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(base, struct shellgrab, base.pointer);
	struct nemopointer *pointer = base->pointer;
	struct nemoview *view;
	float sx, sy;

	view = nemocompz_pick_view(pointer->seat->compz, pointer->x, pointer->y, &sx, &sy, NEMOVIEW_PICK_STATE);

	if (grab->bin == NULL || view == NULL || grab->bin->canvas != view->canvas) {
		nemoshell_end_pointer_shellgrab(grab);
		free(grab);
	}
}

static void busy_cursor_grab_motion(struct nemopointer_grab *base, uint32_t time, float x, float y)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(base, struct shellgrab, base.pointer);
	struct nemopointer *pointer = base->pointer;

	nemopointer_move(pointer, x, y);
}

static void busy_cursor_grab_axis(struct nemopointer_grab *base, uint32_t time, uint32_t axis, float value)
{
}

static void busy_cursor_grab_button(struct nemopointer_grab *base, uint32_t time, uint32_t button, uint32_t state)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(base, struct shellgrab, base.pointer);
	struct nemopointer *pointer = base->pointer;

	if (grab->bin == NULL || grab->bin->canvas != pointer->focus->canvas) {
		nemoshell_end_pointer_shellgrab(grab);
		free(grab);
	}
}

static void busy_cursor_grab_cancel(struct nemopointer_grab *base)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(base, struct shellgrab, base.pointer);

	nemoshell_end_pointer_shellgrab(grab);
	free(grab);
}

static const struct nemopointer_grab_interface busy_cursor_grab_interface = {
	busy_cursor_grab_focus,
	busy_cursor_grab_motion,
	busy_cursor_grab_axis,
	busy_cursor_grab_button,
	busy_cursor_grab_cancel
};

void nemoshell_start_busycursor_grab(struct shellbin *bin, struct nemopointer *pointer)
{
	struct shellgrab *grab;

	if (pointer->grab->interface == &busy_cursor_grab_interface)
		return;

	grab = (struct shellgrab *)malloc(sizeof(struct shellgrab));
	if (grab == NULL)
		return;

	nemoshell_start_pointer_shellgrab(bin->shell, grab, &busy_cursor_grab_interface, bin, pointer);
}

void nemoshell_end_busycursor_grab(struct nemocompz *compz, struct wl_client *client)
{
	struct wl_list *device_list = &compz->seat->pointer.device_list;
	struct nemopointer *pointer;
	struct shellgrab *grab;

	wl_list_for_each(pointer, device_list, link) {
		grab = (struct shellgrab *)container_of(pointer->grab, struct shellgrab, base.pointer);

		if (pointer->grab->interface == &busy_cursor_grab_interface) {
			if (grab->bin == NULL ||
					wl_resource_get_client(grab->bin->resource) == client) {
				nemoshell_end_pointer_shellgrab(grab);
				free(grab);
			}
		}
	}
}
