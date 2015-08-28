#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemoshow.h>
#include <showeasy.h>
#include <showhelper.h>
#include <nemomisc.h>

#include <showcanvas.hpp>

static void nemoshow_dispatch_tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
	struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);
	struct nemocanvas *canvas = NEMOSHOW_AT(show, canvas);
	uint32_t id = nemotale_node_get_id(node);

	if (id == 1) {
		if (nemotale_is_down_event(tale, event, type)) {
			nemotale_event_update_node_taps(tale, node, event, type);

			if (event->tapcount == 1) {
				nemocanvas_move(canvas, event->taps[0]->serial);
			} else if (event->tapcount == 2) {
				nemocanvas_pick(canvas,
						event->taps[0]->serial,
						event->taps[1]->serial,
						(1 << NEMO_SURFACE_PICK_TYPE_ROTATE) | (1 << NEMO_SURFACE_PICK_TYPE_SCALE));
			}
		} else if (nemotale_is_single_click(tale, event, type) != 0) {
			nemotale_event_update_node_taps(tale, node, event, type);

			if (event->tapcount == 1) {
				nemoshow_attach_transition_easy(show,
						nemoshow_transition_create_easy(
							show,
							nemoshow_search_one(show, "ease0"),
							6000, 0,
							"hour_hand_sequence",
							"min_hand_sequence",
							NULL),
						nemoshow_transition_create_easy(
							show,
							nemoshow_search_one(show, "ease0"),
							6000, 1000,
							"hour_hand_sequence_r",
							"min_hand_sequence_r",
							NULL),
						NULL);

				nemocanvas_dispatch_frame(canvas);
			} else if (event->tapcount == 2) {
				struct showone *var;

				var = nemoshow_search_one(show, "var0");
				nemoshow_one_sets(var, "d", "mine");
				nemoshow_one_dirty(var, NEMOSHOW_SHAPE_DIRTY);
				nemoshow_one_update(show, var);

				nemocanvas_dispatch_frame(canvas);
			} else if (event->tapcount == 3) {
				struct nemotool *tool = nemocanvas_get_tool(canvas);

				nemotool_exit(tool);
			}
		}
	}
}

static void nemoshow_custom_canvas_dispatch_render(struct nemoshow *show, struct showone *one)
{
	SkCanvas *canvas = nemoshow_canvas_get_skia_canvas(one);
	SkPaint paint;

	canvas->save();

	canvas->clear(SK_ColorTRANSPARENT);

	canvas->scale(
			nemoshow_canvas_get_viewport_sx(one),
			nemoshow_canvas_get_viewport_sy(one));

	paint.setStyle(SkPaint::kStroke_Style);
	paint.setStrokeWidth(5.0f);
	paint.setColor(SK_ColorYELLOW);

	canvas->drawRect(
			SkRect::MakeXYWH(50, 50, 50, 50),
			paint);

	canvas->restore();
}

int main(int argc, char *argv[])
{
	struct nemotool *tool;
	struct nemoshow *show;

	tool = nemotool_create();
	if (tool == NULL)
		return -1;
	nemotool_connect_wayland(tool, NULL);

	show = nemoshow_create_on_tale(tool, 512, 512, nemoshow_dispatch_tale_event);
	if (show == NULL)
		return -1;
	nemoshow_load_xml(show, argv[1]);
	nemoshow_arrange_one(show);
	nemoshow_update_one(show);
	nemoshow_update_symbol(show, "hour", 5.0f);
	nemoshow_update_symbol(show, "min", 10.0f);
	nemoshow_update_symbol(show, "sec", 15.0f);
	nemoshow_update_expression(show);

	nemoshow_set_scene(show,
			nemoshow_search_one(show, "scene0"));
	nemoshow_set_size(show, 512, 512);

	nemoshow_set_camera(show,
			nemoshow_search_one(show, "camera0"));

	nemoshow_canvas_set_dispatch_render(
			nemoshow_search_one(show, "custom"),
			nemoshow_custom_canvas_dispatch_render);

	nemoshow_render_one(show);

	nemocanvas_dispatch_frame(NEMOSHOW_AT(show, canvas));

	nemocanvas_set_min_size(NEMOSHOW_AT(show, canvas), 512, 512);
	nemocanvas_set_max_size(NEMOSHOW_AT(show, canvas), 1024, 1024);

	nemotool_run(tool);

	nemoshow_destroy_on_tale(show);

	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

	return 0;
}
