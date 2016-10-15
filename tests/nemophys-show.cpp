#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <nemoshow.h>
#include <showhelper.h>
#include <skiahelper.hpp>
#include <nemohelper.h>
#include <nemomisc.h>

struct physcontext {
	struct nemotool *tool;

	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;

	struct showone *canvas;
};

static void nemophys_dispatch_canvas_redraw(struct nemoshow *show, struct showone *one)
{
	SkBitmap bitmap;
	bitmap.setInfo(
			SkImageInfo::Make(
				nemoshow_canvas_get_viewport_width(one),
				nemoshow_canvas_get_viewport_height(one),
				kN32_SkColorType, kPremul_SkAlphaType));
	bitmap.setPixels(nemoshow_canvas_map(one));

	SkBitmapDevice device(bitmap);

	SkCanvas canvas(&device);
	canvas.clear(SK_ColorTRANSPARENT);

	SkPaint paint;
	paint.setStyle(SkPaint::kStrokeAndFill_Style);
	paint.setStrokeWidth(3.0f);
	paint.setColor(SK_ColorYELLOW);

	SkRect rect = SkRect::MakeXYWH(50, 50, 250, 250);

	canvas.drawRect(rect, paint);

	nemoshow_canvas_unmap(one);
}

static void nemophys_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, struct showevent *event)
{
	struct physcontext *context = (struct physcontext *)nemoshow_get_userdata(show);

	nemoshow_event_update_taps(show, canvas, event);

	if (nemoshow_event_is_touch_down(show, event) || nemoshow_event_is_touch_up(show, event)) {
		if (nemoshow_event_is_more_taps(show, event, 3)) {
			nemoshow_view_pick_distant(show, event, NEMOSHOW_VIEW_PICK_ALL_TYPE);

			nemoshow_event_set_cancel(event);

			nemoshow_dispatch_grab_all(show, event);
		}
	}
}

static void nemophys_dispatch_show_resize(struct nemoshow *show, int32_t width, int32_t height)
{
	struct physcontext *context = (struct physcontext *)nemoshow_get_userdata(show);

	nemoshow_view_resize(context->show, width, height);

	nemoshow_view_redraw(context->show);
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ 0 }
	};

	struct physcontext *context;
	struct nemotool *tool;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct showone *one;
	int width = 800;
	int height = 800;
	int opt;

	opterr = 0;

	while (opt = getopt_long(argc, argv, "", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			default:
				break;
		}
	}

	context = (struct physcontext *)malloc(sizeof(struct physcontext));
	if (context == NULL)
		goto err1;
	memset(context, 0, sizeof(struct physcontext));

	context->tool = tool = nemotool_create();
	if (tool == NULL)
		goto err2;
	nemotool_connect_wayland(tool, NULL);

	context->show = show = nemoshow_create_view(tool, width, height);
	if (show == NULL)
		goto err3;
	nemoshow_set_dispatch_resize(show, nemophys_dispatch_show_resize);
	nemoshow_set_userdata(show, context);

	context->scene = scene = nemoshow_scene_create();
	nemoshow_scene_set_width(scene, width);
	nemoshow_scene_set_height(scene, height);
	nemoshow_set_scene(show, scene);

	context->back = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_BACK_TYPE);
	nemoshow_canvas_set_fill_color(canvas, 0.0f, 0.0f, 0.0f, 1.0f);
	nemoshow_one_attach(scene, canvas);

	context->canvas = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_PIXMAN_TYPE);
	nemoshow_canvas_set_dispatch_redraw(canvas, nemophys_dispatch_canvas_redraw);
	nemoshow_canvas_set_dispatch_event(canvas, nemophys_dispatch_canvas_event);
	nemoshow_one_attach(scene, canvas);

	nemoshow_dispatch_frame(show);

	nemotool_run(tool);

	nemoshow_destroy_view(show);

err3:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

err2:
	free(context);

err1:
	return 0;
}
