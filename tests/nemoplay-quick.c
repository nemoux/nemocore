#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <nemoplay.h>
#include <playback.h>
#include <nemoshow.h>
#include <showhelper.h>
#include <nemolog.h>
#include <nemomisc.h>

struct playcontext {
	struct nemotool *tool;

	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;
	struct showone *canvas;

	struct nemoplay *play;
	struct playextractor *extractor;
	struct playshader *shader;
	struct playbox *box;
	int iframes;
	int direction;

	int32_t width;
	int32_t height;
};

static void nemoplay_dispatch_show_transform(struct nemoshow *show, int32_t visible, int32_t x, int32_t y, int32_t width, int32_t height)
{
}

static void nemoplay_dispatch_show_fullscreen(struct nemoshow *show, const char *id, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct playcontext *context = (struct playcontext *)nemoshow_get_userdata(show);

	if (id != NULL) {
		double ratio = ((double)width / nemoplay_get_video_aspectratio(context->play)) / (double)height;

		nemoshow_canvas_translate(context->canvas, 0.0f, context->height * (1.0f - ratio) / 2.0f);
		nemoshow_canvas_set_size(context->canvas, context->width, context->height * ratio);
	} else {
		nemoshow_canvas_translate(context->canvas, 0.0f, 0.0f);
		nemoshow_canvas_set_size(context->canvas, context->width, context->height);

		nemoshow_view_resize(context->show, width, width / nemoplay_get_video_aspectratio(context->play));
	}

	nemoshow_dispatch_frame(context->show);
}

static void nemoplay_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, struct showevent *event)
{
	struct playcontext *context = (struct playcontext *)nemoshow_get_userdata(show);

	if (nemoshow_event_is_touch_down(show, event) || nemoshow_event_is_touch_up(show, event)) {
		nemoshow_event_update_taps(show, canvas, event);

		if (nemoshow_event_is_single_tap(show, event)) {
			nemoshow_view_move(show, nemoshow_event_get_serial_on(event, 0));
		} else if (nemoshow_event_is_many_taps(show, event)) {
			nemoshow_view_pick_distant(show, event, NEMOSHOW_VIEW_PICK_ALL_TYPE);
		}
	}

	if (nemoshow_event_is_pointer_button_down(show, event, BTN_LEFT)) {
		nemoshow_view_move(show, nemoshow_event_get_serial(event));
	} else if (nemoshow_event_is_pointer_button_down(show, event, BTN_RIGHT)) {
		nemoshow_view_pick(show, nemoshow_event_get_serial(event), 0, NEMOSHOW_VIEW_PICK_ALL_TYPE);
	}

	if (nemoshow_event_is_single_click(show, event)) {
		context->direction = !context->direction;
	}
}

static void nemoplay_dispatch_canvas_resize(struct nemoshow *show, struct showone *canvas, int32_t width, int32_t height)
{
	struct playcontext *context = (struct playcontext *)nemoshow_get_userdata(show);
	struct playone *one;

	nemoplay_shader_set_viewport(context->shader,
			nemoshow_canvas_get_texture(context->canvas),
			width, height);

	one = nemoplay_box_get_one(context->box, context->iframes);
	if (one != NULL) {
		nemoplay_shader_update(context->shader, one);
		nemoplay_shader_dispatch(context->shader);
	}
}

static void nemoplay_dispatch_canvas_frame(void *userdata, uint32_t time, double t)
{
	struct playcontext *context = (struct playcontext *)userdata;
	struct playone *one;

	if (nemoplay_box_get_count(context->box) == 0)
		return;

	if (context->direction == 0) {
		context->iframes = (context->iframes + 1) % nemoplay_box_get_count(context->box);
	} else {
		context->iframes = (context->iframes + nemoplay_box_get_count(context->box) - 1) % nemoplay_box_get_count(context->box);
	}

	one = nemoplay_box_get_one(context->box, context->iframes);
	if (one != NULL) {
		nemoplay_shader_update(context->shader, one);
		nemoplay_shader_dispatch(context->shader);
	}
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ 0 }
	};

	struct playcontext *context;
	struct nemotool *tool;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct showone *one;
	struct showtransition *trans;
	struct nemoplay *play;
	struct playbox *box;
	struct playshader *shader;
	char *mediapath = NULL;
	int width, height;
	int opt;

	while (opt = getopt_long(argc, argv, "", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			default:
				break;
		}
	}

	if (optind < argc)
		mediapath = strdup(argv[optind]);

	if (mediapath == NULL)
		return 0;

	if (nemoplay_get_video_info(mediapath, &width, &height) <= 0)
		goto err1;

	context = (struct playcontext *)malloc(sizeof(struct playcontext));
	if (context == NULL)
		return -1;
	memset(context, 0, sizeof(struct playcontext));

	context->width = width;
	context->height = height;

	context->play = play = nemoplay_create();
	if (context->play == NULL)
		goto err2;
	nemoplay_load_media(play, mediapath);

	context->box = box = nemoplay_box_create(nemoplay_get_video_framecount(play));

	context->extractor = nemoplay_extractor_create(play, box);

	context->tool = tool = nemotool_create();
	if (tool == NULL)
		goto err3;
	nemotool_connect_wayland(tool, NULL);

	context->show = show = nemoshow_create_view(tool, width, height);
	if (show == NULL)
		goto err4;
	nemoshow_set_dispatch_transform(show, nemoplay_dispatch_show_transform);
	nemoshow_set_dispatch_fullscreen(show, nemoplay_dispatch_show_fullscreen);
	nemoshow_set_userdata(show, context);

	nemoshow_view_set_fullscreen_type(show, "pick");
	nemoshow_view_set_fullscreen_type(show, "pitch");

	nemoshow_view_set_anchor(show, -0.5f, -0.5f);

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
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_OPENGL_TYPE);
	nemoshow_canvas_set_dispatch_event(canvas, nemoplay_dispatch_canvas_event);
	nemoshow_canvas_set_dispatch_resize(canvas, nemoplay_dispatch_canvas_resize);
	nemoshow_one_attach(scene, canvas);

	context->shader = shader = nemoplay_shader_create();
	nemoplay_shader_set_format(shader,
			nemoplay_get_pixel_format(play));
	nemoplay_shader_set_texture(shader,
			nemoplay_get_video_width(play),
			nemoplay_get_video_height(play));

	trans = nemoshow_transition_create(NEMOSHOW_LINEAR_EASE, 18000, 0);
	nemoshow_transition_dirty_one(trans, canvas, NEMOSHOW_REDRAW_DIRTY);
	nemoshow_transition_set_dispatch_frame(trans, nemoplay_dispatch_canvas_frame);
	nemoshow_transition_set_userdata(trans, context);
	nemoshow_transition_set_repeat(trans, 0);
	nemoshow_attach_transition(show, trans);

	nemoshow_dispatch_frame(show);

	nemotool_run(tool);

	nemoshow_destroy_view(show);

err4:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

err3:
	nemoplay_extractor_destroy(context->extractor);

	nemoplay_box_destroy(box);
	nemoplay_destroy(play);

err2:
	free(context);

err1:
	free(mediapath);

	return 0;
}
