#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemoshow.h>
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
			} else if (event->tapcount == 2) {
			} else if (event->tapcount == 3) {
				struct nemotool *tool = nemocanvas_get_tool(canvas);

				nemotool_exit(tool);
			}
		}
	}
}

static void nemoshow_custom_canvas_dispatch_render(struct nemoshow *show, struct showone *one)
{
	SkCanvas *canvas = NEMOSHOW_CANVAS_CC(NEMOSHOW_CANVAS(one), canvas);
	SkPaint paint;

	canvas->save();

	canvas->clear(SK_ColorTRANSPARENT);

	canvas->scale(
			NEMOSHOW_CANVAS_AT(one, viewport.sx),
			NEMOSHOW_CANVAS_AT(one, viewport.sy));

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

	nemoshow_canvas_set_dispatch_render(
			nemoshow_search_one(show, "custom"),
			nemoshow_custom_canvas_dispatch_render);

	nemoshow_render_one(show);

	nemocanvas_dispatch_frame(NEMOSHOW_AT(show, canvas));

	nemotool_run(tool);

	nemoshow_destroy_on_tale(show);

	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

	return 0;
}
