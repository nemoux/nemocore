#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemotool.h>
#include <nemocanvas.h>
#include <nemoegl.h>
#include <nemoshow.h>
#include <showhelper.h>
#include <nemomisc.h>

static void nemoshow_dispatch_tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
	struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);
	struct nemocanvas *canvas = NEMOSHOW_AT(show, canvas);
	uint32_t id = nemotale_node_get_id(node);

	if (id == 1) {
		if (type & NEMOTALE_DOWN_EVENT) {
			struct taletap *taps[16];
			int ntaps;

			ntaps = nemotale_get_node_taps(tale, node, taps, type);
			if (ntaps == 1) {
				nemocanvas_move(canvas, taps[0]->serial);
			} else if (ntaps == 2) {
				nemocanvas_pick(canvas,
						taps[0]->serial,
						taps[1]->serial,
						(1 << NEMO_SURFACE_PICK_TYPE_ROTATE) | (1 << NEMO_SURFACE_PICK_TYPE_SCALE));
			}
		} else if (nemotale_is_single_click(tale, event, type) != 0) {
			struct taletap *taps[16];
			int ntaps;

			ntaps = nemotale_get_node_taps(tale, node, taps, type);
			if (ntaps == 1) {
				static int direction = 0;
				struct showtransition *trans;
				struct showone *sequence;

				nemoshow_one_sets(
						nemoshow_search_one(show, "var0"),
						"d",
						"mine");
				nemoshow_one_dirty(
						nemoshow_search_one(show, "var0"));
				nemoshow_one_update(show,
						nemoshow_search_one(show, "var0"));

				trans = nemoshow_transition_create(
						nemoshow_search_one(show, "ease0"),
						800, 0,
						nemoshow_get_next_serial(show));

				if ((direction++ % 2) == 0) {
					sequence = nemoshow_search_one(show, "hour_hand_sequence");
				} else {
					sequence = nemoshow_search_one(show, "hour_hand_sequence_r");
				}
				nemoshow_update_one_expression(show, sequence);
				nemoshow_transition_attach_sequence(trans, sequence);

				nemoshow_attach_transition(show, trans);

				nemocanvas_dispatch_frame(canvas);
			} else if (ntaps == 2) {
			} else if (ntaps == 3) {
				struct nemotool *tool = nemocanvas_get_tool(canvas);

				nemotool_exit(tool);
			}
		}
	}
}

int main(int argc, char *argv[])
{
	struct nemotool *tool;
	struct nemoshow *show;

	tool = nemotool_create();
	if (tool == NULL)
		return -1;
	nemotool_connect_wayland(tool, NULL);

	show = nemoshow_create_on_tale(tool, 500, 500, nemoshow_dispatch_tale_event);
	if (show == NULL)
		return -1;
	nemoshow_load_xml(show, argv[1]);
	nemoshow_arrange_one(show);
	nemoshow_update_one(show);
	nemoshow_update_expression(show);

	nemoshow_set_scene(show,
			nemoshow_search_one(show, "scene0"));
	nemoshow_set_size(show, 500, 500);

	nemoshow_set_camera(show,
			nemoshow_search_one(show, "camera0"));

	nemoshow_render_one(show);

	nemocanvas_dispatch_frame(NEMOSHOW_AT(show, canvas));

	nemotool_run(tool);

	nemoshow_destroy_on_tale(show);

	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

	return 0;
}
