#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>
#include <pthread.h>

#include <ao/ao.h>

#include <nemoplay.h>
#include <nemoshow.h>
#include <showhelper.h>
#include <nemoenvs.h>
#include <nemolog.h>
#include <nemomisc.h>

struct playcontext {
	struct nemotool *tool;

	struct nemoenvs *envs;

	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;
	struct showone *canvas;

	struct nemoplay *play;
	struct playshader *shader;

	struct nemotimer *timer;

	char *screenid;

	int32_t width;
	int32_t height;
};

static void nemoplay_dispatch_show_transform(struct nemoshow *show, int32_t visible, int32_t x, int32_t y, int32_t width, int32_t height)
{
}

static void nemoplay_dispatch_show_fullscreen(struct nemoshow *show, const char *id, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct playcontext *context = (struct playcontext *)nemoshow_get_userdata(show);

	if (context->screenid != NULL)
		free(context->screenid);

	if (id != NULL)
		context->screenid = strdup(id);
	else
		context->screenid = NULL;

	if (id != NULL) {
		double ratio = ((double)width / nemoplay_get_video_aspectratio(context->play)) / (double)height;

		nemoshow_canvas_translate(context->canvas, 0.0f, context->height * (1.0f - ratio) / 2.0f);
		nemoshow_canvas_set_size(context->show, context->canvas, context->width, context->height * ratio);
	} else {
		nemoshow_canvas_translate(context->canvas, 0.0f, 0.0f);
		nemoshow_canvas_set_size(context->show, context->canvas, context->width, context->height);

		nemoshow_view_resize(context->show, width, width / nemoplay_get_video_aspectratio(context->play));
	}

	if (nemoplay_get_frame(context->play) != 0) {
		nemoplay_shader_dispatch(context->shader);

		nemoshow_dispatch_frame(context->show);
	}
}

static void nemoplay_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, void *event)
{
	struct playcontext *context = (struct playcontext *)nemoshow_get_userdata(show);

	if (nemoshow_event_is_down(show, event) || nemoshow_event_is_up(show, event)) {
		nemoshow_event_update_taps(show, canvas, event);

		if (nemoshow_event_is_single_tap(show, event)) {
			nemoshow_view_move(show, nemoshow_event_get_serial_on(event, 0));
		} else if (nemoshow_event_is_double_taps(show, event)) {
			nemoshow_view_pick(show,
					nemoshow_event_get_serial_on(event, 0),
					nemoshow_event_get_serial_on(event, 1),
					NEMOSHOW_VIEW_PICK_ALL_TYPE);
		} else if (nemoshow_event_is_many_taps(show, event)) {
			nemoshow_view_pick_distant(show, event, NEMOSHOW_VIEW_PICK_ROTATE_TYPE | NEMOSHOW_VIEW_PICK_TRANSLATE_TYPE);
		}
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

static void nemoplay_dispatch_canvas_resize(struct nemoshow *show, struct showone *one, int32_t width, int32_t height)
{
	struct playcontext *context = (struct playcontext *)nemoshow_get_userdata(show);

	nemoplay_shader_set_viewport(context->shader,
			nemoshow_canvas_get_texture(context->canvas),
			width, height);

	if (nemoplay_get_frame(context->play) != 0)
		nemoplay_shader_dispatch(context->shader);
}

static void nemoplay_dispatch_video_timer(struct nemotimer *timer, void *data)
{
	struct playcontext *context = (struct playcontext *)data;
	struct nemoplay *play = context->play;
	struct playqueue *queue;
	struct playone *one;
	int state;

	queue = nemoplay_get_video_queue(play);

	state = nemoplay_queue_get_state(queue);
	if (state == NEMOPLAY_QUEUE_NORMAL_STATE) {
		double threshold = 1.0f / nemoplay_get_video_framerate(play);
		double cts = nemoplay_get_cts(play);
		double pts;

		if (cts >= nemoplay_get_duration(play))
			nemoplay_set_cmd(play, NEMOPLAY_SEEK_CMD);

		if (nemoplay_queue_get_count(queue) < 64)
			nemoplay_set_state(play, NEMOPLAY_WAKE_STATE);

		one = nemoplay_queue_dequeue(queue);
		if (one == NULL) {
			nemotimer_set_timeout(timer, threshold * 1000);
		} else if (nemoplay_queue_get_one_serial(one) != nemoplay_queue_get_serial(queue)) {
			nemoplay_queue_destroy_one(one);
			nemotimer_set_timeout(timer, 1);
		} else if (nemoplay_queue_get_one_cmd(one) == NEMOPLAY_QUEUE_NORMAL_COMMAND) {
			nemoplay_set_video_pts(play, nemoplay_queue_get_one_pts(one));

			if (cts > nemoplay_queue_get_one_pts(one) + threshold) {
				nemoplay_queue_destroy_one(one);
				nemotimer_set_timeout(timer, 1);
			} else if (cts < nemoplay_queue_get_one_pts(one) - threshold) {
				nemoplay_queue_enqueue_tail(queue, one);
				nemotimer_set_timeout(timer, threshold * 1000);
			} else {
				if (nemoplay_shader_get_texture_linesize(context->shader) != nemoplay_queue_get_one_width(one))
					nemoplay_shader_set_texture_linesize(context->shader, nemoplay_queue_get_one_width(one));

				nemoplay_shader_update(context->shader,
						nemoplay_queue_get_one_y(one),
						nemoplay_queue_get_one_u(one),
						nemoplay_queue_get_one_v(one));
				nemoplay_shader_dispatch(context->shader);

				nemoshow_canvas_damage_all(context->canvas);
				nemoshow_dispatch_frame(context->show);

				if (nemoplay_queue_peek_pts(queue, &pts) != 0)
					nemotimer_set_timeout(timer, MINMAX(pts > cts ? pts - cts : 1.0f, 1.0f, threshold) * 1000);
				else
					nemotimer_set_timeout(timer, threshold * 1000);

				nemoplay_queue_destroy_one(one);

				nemoplay_next_frame(play);
			}
		}
	} else if (state == NEMOPLAY_QUEUE_STOP_STATE) {
		nemotimer_set_timeout(timer, 1000 / nemoplay_get_video_framerate(play));
	}
}

static void *nemoplay_handle_audioplay(void *arg)
{
	struct playcontext *context = (struct playcontext *)arg;
	struct nemoplay *play = context->play;
	struct playqueue *queue;
	struct playone *one;
	ao_device *device;
	ao_sample_format format;
	int driver;
	int state;

	nemoplay_enter_thread(play);

	ao_initialize();

	format.channels = nemoplay_get_audio_channels(play);
	format.bits = nemoplay_get_audio_samplebits(play);
	format.rate = nemoplay_get_audio_samplerate(play);
	format.byte_format = AO_FMT_NATIVE;
	format.matrix = 0;

	driver = ao_default_driver_id();
	device = ao_open_live(driver, &format, NULL);

	queue = nemoplay_get_audio_queue(play);

	while ((state = nemoplay_queue_get_state(queue)) != NEMOPLAY_QUEUE_DONE_STATE) {
		if (state == NEMOPLAY_QUEUE_NORMAL_STATE) {
			if (nemoplay_queue_get_count(queue) < 64)
				nemoplay_set_state(play, NEMOPLAY_WAKE_STATE);

			one = nemoplay_queue_dequeue(queue);
			if (one == NULL) {
				nemoplay_queue_wait(queue);
			} else if (nemoplay_queue_get_one_serial(one) != nemoplay_queue_get_serial(queue)) {
				nemoplay_queue_destroy_one(one);
			} else if (nemoplay_queue_get_one_cmd(one) == NEMOPLAY_QUEUE_NORMAL_COMMAND) {
				nemoplay_set_audio_pts(play, nemoplay_queue_get_one_pts(one));

				ao_play(device,
						nemoplay_queue_get_one_data(one),
						nemoplay_queue_get_one_size(one));

				nemoplay_queue_destroy_one(one);
			}
		} else if (state == NEMOPLAY_QUEUE_STOP_STATE) {
			nemoplay_queue_wait(queue);
		}
	}

	ao_close(device);
	ao_shutdown();

	nemoplay_leave_thread(play);

	return NULL;
}

static void *nemoplay_handle_decodeframe(void *arg)
{
	struct playcontext *context = (struct playcontext *)arg;
	struct nemoplay *play = context->play;
	int state;

	nemoplay_enter_thread(play);

	while ((state = nemoplay_get_state(play)) != NEMOPLAY_DONE_STATE) {
		if (nemoplay_has_cmd(play, NEMOPLAY_SEEK_CMD) != 0) {
			nemoplay_seek_media(play, 0.0f);
			nemoplay_put_cmd(play, NEMOPLAY_SEEK_CMD);
		}

		if (state == NEMOPLAY_PLAY_STATE) {
			nemoplay_decode_media(play, 256, 128);
		} else if (state == NEMOPLAY_WAIT_STATE || state == NEMOPLAY_STOP_STATE) {
			nemoplay_wait_media(play);
		}
	}

	nemoplay_leave_thread(play);

	return NULL;
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "audio",			required_argument,		NULL,		'a' },
		{ 0 }
	};

	struct playcontext *context;
	struct nemotool *tool;
	struct nemoenvs *envs;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct showone *one;
	struct nemoplay *play;
	struct playshader *shader;
	struct nemotimer *timer;
	pthread_t thread;
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

	context->screenid = NULL;

	context->width = width;
	context->height = height;

	context->play = play = nemoplay_create();
	if (context->play == NULL)
		goto err2;

	nemoplay_prepare_media(play, mediapath);

	if (on_audio == 0)
		nemoplay_revoke_audio(play);

	pthread_create(&thread, NULL, nemoplay_handle_decodeframe, (void *)context);
	pthread_create(&thread, NULL, nemoplay_handle_audioplay, (void *)context);

	context->tool = tool = nemotool_create();
	if (tool == NULL)
		goto err3;
	nemotool_connect_wayland(tool, NULL);

	context->envs = envs = nemoenvs_create(tool);
	if (envs == NULL)
		goto err4;
	nemoenvs_connect(envs, "/nemoshell", "127.0.0.1", 10000);

	context->show = show = nemoshow_create_view(tool, width, height);
	if (show == NULL)
		goto err5;
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
	nemoshow_canvas_set_fill_color(canvas, 1.0f, 1.0f, 1.0f, 1.0f);
	nemoshow_one_attach(scene, canvas);

	context->canvas = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_OPENGL_TYPE);
	nemoshow_canvas_set_dispatch_event(canvas, nemoplay_dispatch_canvas_event);
	nemoshow_canvas_set_dispatch_resize(canvas, nemoplay_dispatch_canvas_resize);
	nemoshow_one_attach(scene, canvas);

	context->shader = shader = nemoplay_shader_create();
	nemoplay_shader_prepare(shader,
			NEMOPLAY_TO_RGBA_VERTEX_SHADER,
			NEMOPLAY_TO_RGBA_FRAGMENT_SHADER);
	nemoplay_shader_set_texture(shader, width, height);

	context->timer = timer = nemotimer_create(tool);
	nemotimer_set_callback(timer, nemoplay_dispatch_video_timer);
	nemotimer_set_userdata(timer, context);
	nemotimer_set_timeout(timer, 10);

	nemoshow_dispatch_frame(show);

	nemotool_run(tool);

	nemotimer_destroy(timer);

	nemoshow_destroy_view(show);

err5:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

err4:
	nemoenvs_destroy(envs);

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
