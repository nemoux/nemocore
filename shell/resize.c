#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <resize.h>
#include <shell.h>
#include <compz.h>
#include <seat.h>
#include <pointer.h>
#include <view.h>
#include <canvas.h>
#include <content.h>
#include <nemomisc.h>

static void resize_grab_focus(struct nemopointer_grab *base)
{
}

static void resize_grab_motion(struct nemopointer_grab *base, uint32_t time, float x, float y)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(base, struct shellgrab, base.pointer);
	struct shellgrab_resize *resize = (struct shellgrab_resize *)container_of(grab, struct shellgrab_resize, base);
	struct nemopointer *pointer = base->pointer;
	struct shellbin *bin = grab->bin;
	float fromx, fromy, tox, toy;
	int32_t width, height;

	nemopointer_move(pointer, x, y);

	if (bin != NULL) {
		nemoview_transform_from_global(bin->view, pointer->grab_x, pointer->grab_y, &fromx, &fromy);
		nemoview_transform_from_global(bin->view, pointer->x, pointer->y, &tox, &toy);

		width = resize->width;
		if (resize->edges & WL_SHELL_SURFACE_RESIZE_LEFT) {
			width += (fromx - tox);
		} else if (resize->edges & WL_SHELL_SURFACE_RESIZE_RIGHT) {
			width += (tox - fromx);
		}

		height = resize->height;
		if (resize->edges & WL_SHELL_SURFACE_RESIZE_TOP) {
			height += (fromy - toy);
		} else if (resize->edges & WL_SHELL_SURFACE_RESIZE_BOTTOM) {
			height += (toy - fromy);
		}

		bin->callback->send_configure(bin->canvas, width, height);
	}
}

static void resize_grab_axis(struct nemopointer_grab *base, uint32_t time, uint32_t axis, float value)
{
}

static void resize_grab_button(struct nemopointer_grab *base, uint32_t time, uint32_t button, uint32_t state)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(base, struct shellgrab, base.pointer);
	struct shellgrab_resize *resize = (struct shellgrab_resize *)container_of(grab, struct shellgrab_resize, base);
	struct nemopointer *pointer = base->pointer;

	if (pointer->focus != NULL) {
		nemocontent_pointer_button(pointer, pointer->focus->content, time, button, state);
	}

	if (pointer->button_count == 0 &&
			state == WL_POINTER_BUTTON_STATE_RELEASED) {
		if (grab->bin != NULL) {
			grab->bin->resize_edges = 0;

			nemoshell_send_bin_config(grab->bin);
		}

		nemoshell_end_pointer_shellgrab(grab);
		free(resize);
	}
}

static void resize_grab_cancel(struct nemopointer_grab *base)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(base, struct shellgrab, base.pointer);
	struct shellgrab_resize *resize = (struct shellgrab_resize *)container_of(grab, struct shellgrab_resize, base);

	if (grab->bin != NULL) {
		grab->bin->resize_edges = 0;

		nemoshell_send_bin_config(grab->bin);
	}

	nemoshell_end_pointer_shellgrab(grab);
	free(resize);
}

static const struct nemopointer_grab_interface resize_grab_interface = {
	resize_grab_focus,
	resize_grab_motion,
	resize_grab_axis,
	resize_grab_button,
	resize_grab_cancel
};

int nemoshell_resize_canvas_by_pointer(struct nemopointer *pointer, struct shellbin *bin, uint32_t edges)
{
	struct shellgrab_resize *resize;

	if (bin == NULL)
		return -1;

	if (nemoview_has_grab(bin->view) != 0)
		wl_signal_emit(&bin->ungrab_signal, bin);

	resize = (struct shellgrab_resize *)malloc(sizeof(struct shellgrab_resize));
	if (resize == NULL)
		return -1;
	memset(resize, 0, sizeof(struct shellgrab_resize));

	resize->edges = edges;

	resize->width = nemoshell_bin_get_geometry_width(bin);
	resize->height = nemoshell_bin_get_geometry_height(bin);

	bin->resize_edges = edges;

	nemoshell_send_bin_config(bin);

	nemoshell_start_pointer_shellgrab(bin->shell, &resize->base, &resize_grab_interface, bin, pointer);

	return 0;
}

int nemoshell_resize_canvas(struct nemoshell *shell, struct shellbin *bin, uint32_t serial, uint32_t edges)
{
	struct nemopointer *pointer;

	if (edges == 0 || edges > 15 || (edges & 3) == 3 || (edges & 12) == 12)
		return 0;

	pointer = nemoseat_get_pointer_by_grab_serial(shell->compz->seat, serial);
	if (pointer != NULL) {
		return nemoshell_resize_canvas_by_pointer(pointer, bin, edges);
	}

	return 0;
}
