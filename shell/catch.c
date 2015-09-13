#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>
#include <wayland-nemo-shell-server-protocol.h>

#include <catch.h>
#include <shell.h>
#include <compz.h>
#include <seat.h>
#include <touch.h>
#include <view.h>
#include <canvas.h>
#include <actor.h>
#include <content.h>
#include <nemomisc.h>

static void nemoshell_catch_handle_view_button(struct nemopointer *pointer, struct nemocontent *content, uint32_t time, uint32_t button, void *data)
{
	struct nemoview *view = (struct nemoview *)data;

	if (view->canvas != NULL) {
		nemoshell_move_canvas_by_pointer(pointer, nemoshell_get_bin(view->canvas));
	} else {
		nemoshell_move_actor_by_pointer(pointer, view->actor);
	}
}

static void nemoshell_catch_handle_view_touch(struct touchpoint *tp, struct nemocontent *content, uint32_t time, void *data)
{
	struct nemoview *view = (struct nemoview *)data;
	struct touchpoint *tps[8];
	int tpcount;

	tpcount = nemoseat_get_touchpoint_by_focus(tp->touch->seat, view, tps, sizeof(tps) / sizeof(tps[0]));
	if (tpcount == 1) {
		if (view->canvas != NULL) {
			nemoshell_move_canvas_by_touchpoint(tps[0], nemoshell_get_bin(view->canvas));
		} else {
			nemoshell_move_actor_by_touchpoint(tps[0], view->actor);
		}
	} else if (tpcount == 2) {
		if (view->canvas != NULL) {
			struct shellbin *bin = nemoshell_get_bin(view->canvas);

			if (nemoshell_is_nemo_surface_for_canvas(bin->canvas))
				nemoshell_pick_canvas_by_touchpoint(tps[0], tps[1], (1 << NEMO_SURFACE_PICK_TYPE_ROTATE), bin);
			else
				nemoshell_pick_canvas_by_touchpoint_on_area(tps[0], tps[1], bin);
		} else {
			nemoshell_pick_actor_by_touchpoint(tps[0], tps[1], (1 << NEMO_SURFACE_PICK_TYPE_ROTATE), view->actor);
		}
	}
}

void nemoshell_catch_view(struct nemoview *view)
{
	nemocontent_set_button_handler(view->content, nemoshell_catch_handle_view_button, (void *)view);
	nemocontent_set_touch_handler(view->content, nemoshell_catch_handle_view_touch, (void *)view);
}

void nemoshell_uncatch_view(struct nemoview *view)
{
	nemocontent_set_button_handler(view->content, NULL, NULL);
	nemocontent_set_touch_handler(view->content, NULL, NULL);
}
