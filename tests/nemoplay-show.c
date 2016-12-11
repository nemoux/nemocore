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

	struct playback_decoder *decoderback;
	struct playback_audio *audioback;
	struct playback_video *videoback;

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

	nemoplay_back_redraw_video(context->videoback);

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
		if (nemoshow_event_is_no_tap(show, event)) {
			if (nemoplay_get_state(context->play) == NEMOPLAY_STOP_STATE) {
				nemoplay_set_state(context->play, NEMOPLAY_PLAY_STATE);
			} else {
				nemoplay_set_state(context->play, NEMOPLAY_STOP_STATE);
			}
		}
	}
}

static void nemoplay_dispatch_canvas_resize(struct nemoshow *show, struct showone *canvas, int32_t width, int32_t height)
{
	struct playcontext *context = (struct playcontext *)nemoshow_get_userdata(show);

	nemoplay_back_set_video_texture(context->videoback,
			nemoshow_canvas_get_texture(context->canvas),
			nemoshow_canvas_get_viewport_width(context->canvas),
			nemoshow_canvas_get_viewport_height(context->canvas));
}

static void nemoplay_dispatch_video_update(struct nemoplay *play, void *data)
{
	struct playcontext *context = (struct playcontext *)data;

	nemoshow_canvas_damage_all(context->canvas);
	nemoshow_dispatch_frame(context->show);
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "audio",			required_argument,		NULL,		'a' },
		{ 0 }
	};

	struct playcontext *context;
	struct nemotool *tool;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct showone *one;
	struct nemoplay *play;
	char *mediapath = NULL;
	int on_audio = 1;
	int width, height;
	int opt;

	while (opt = getopt_long(argc, argv, "a:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'a':
				on_audio = strcmp(optarg, "off") != 0;
				break;

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

	if (on_audio == 0)
		nemoplay_revoke_audio(play);

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

	context->decoderback = nemoplay_back_create_decoder(play);
	context->audioback = nemoplay_back_create_audio_by_ao(play);
	context->videoback = nemoplay_back_create_video_by_timer(play, tool);
	nemoplay_back_set_video_texture(context->videoback,
			nemoshow_canvas_get_texture(canvas),
			width, height);
	nemoplay_back_set_video_update(context->videoback, nemoplay_dispatch_video_update);
	nemoplay_back_set_video_data(context->videoback, context);

	nemoshow_dispatch_frame(show);

	nemotool_run(tool);

	nemoplay_back_destroy_video(context->videoback);
	nemoplay_back_destroy_audio(context->audioback);
	nemoplay_back_destroy_decoder(context->decoderback);

	nemoshow_destroy_view(show);

err4:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

err3:
	nemoplay_set_state(play, NEMOPLAY_DONE_STATE);
	nemoplay_wait_thread(play);
	nemoplay_destroy(play);

err2:
	free(context);

err1:
	free(mediapath);

	return 0;
}
