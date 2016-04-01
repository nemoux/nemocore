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
#include <nemolog.h>
#include <nemomisc.h>

struct playcontext {
	struct nemotool *tool;

	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;
	struct showone *canvas;

	struct nemoplay *play;
	struct playshader *shader;
	int has_frame;

	struct nemotimer *timer;
};

static void nemoplay_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, void *event)
{
	struct playcontext *context = (struct playcontext *)nemoshow_get_userdata(show);

	if (nemoshow_event_is_down(show, event) || nemoshow_event_is_up(show, event)) {
		nemoshow_event_update_taps(show, canvas, event);

		if (nemoshow_event_is_single_tap(show, event)) {
			nemoshow_view_move(show, nemoshow_event_get_serial_on(event, 0));
		} else if (nemoshow_event_is_many_taps(show, event)) {
			nemoshow_view_pick_distant(show, event, NEMOSHOW_VIEW_PICK_ALL_TYPE);
		}
	}
}

static void nemoplay_dispatch_canvas_resize(struct nemoshow *show, struct showone *one, int32_t width, int32_t height)
{
	struct playcontext *context = (struct playcontext *)nemoshow_get_userdata(show);

	nemoplay_shader_set_viewport(context->shader,
			nemoshow_canvas_get_texture(context->canvas),
			width, height);

	if (context->has_frame != 0)
		nemoplay_shader_dispatch(context->shader);
}

static void nemoplay_dispatch_video_timer(struct nemotimer *timer, void *data)
{
	struct playcontext *context = (struct playcontext *)data;
	struct playqueue *queue;
	struct playclock *clock;
	struct playone *one;
	struct playone *pone;
	double threshold;
	double time;
	uint32_t timeout;

	queue = nemoplay_get_video_queue(context->play);
	clock = nemoplay_get_audio_clock(context->play);

	threshold = 1.0f / nemoplay_get_video_framerate(context->play);
	timeout = threshold * 1000;
	time = nemoplay_clock_get(clock);

	one = nemoplay_queue_dequeue(queue);
	if (one == NULL) {
		nemoplay_wakeup_media(context->play);
		nemotimer_set_timeout(timer, timeout);
		return;
	}

	if (one->cmd == NEMOPLAY_QUEUE_DONE_COMMAND) {
		nemoplay_queue_destroy_one(one);
		nemotimer_set_timeout(timer, 0);
		return;
	}

	if (time > one->pts + threshold) {
		nemoplay_queue_destroy_one(one);
		nemotimer_set_timeout(timer, 1);
		return;
	} else if (time < one->pts - threshold) {
		nemoplay_queue_enqueue_tail(queue, one);
		nemotimer_set_timeout(timer, (one->pts - time) * 1000);
		return;
	}

	nemoplay_shader_update(context->shader, one->y, one->u, one->v);
	nemoplay_shader_dispatch(context->shader);

	nemoshow_canvas_damage_all(context->canvas);
	nemoshow_dispatch_frame(context->show);

	context->has_frame = 1;

	pone = nemoplay_queue_peek(queue);
	if (pone != NULL)
		timeout = pone->pts > time ? MAX((pone->pts - time) * 1000, 1) : 1;

	nemotimer_set_timeout(timer, timeout);

	nemoplay_queue_destroy_one(one);
}

static void *nemoplay_handle_decodeframe(void *arg)
{
	struct playcontext *context = (struct playcontext *)arg;

	nemoplay_decode_media(context->play);

	return NULL;
}

static void *nemoplay_handle_audioplay(void *arg)
{
	struct playcontext *context = (struct playcontext *)arg;
	struct playqueue *queue;
	struct playclock *clock;
	struct playone *one;
	ao_device *device;
	ao_sample_format format;
	int driver;

	ao_initialize();

	format.channels = nemoplay_get_audio_channels(context->play);
	format.bits = nemoplay_get_audio_samplebits(context->play);
	format.rate = nemoplay_get_audio_samplerate(context->play);
	format.byte_format = AO_FMT_NATIVE;
	format.matrix = 0;

	driver = ao_default_driver_id();
	device = ao_open_live(driver, &format, NULL);

	queue = nemoplay_get_audio_queue(context->play);
	clock = nemoplay_get_audio_clock(context->play);

	do {
		one = nemoplay_queue_dequeue(queue);
		if (one == NULL) {
			nemoplay_wakeup_media(context->play);
			nemoplay_queue_wait(queue);
		} else {
			if (one->cmd == NEMOPLAY_QUEUE_NORMAL_COMMAND) {
				ao_play(device, (char *)one->data, one->size);

				nemoplay_clock_set(clock, one->pts);

				nemoplay_queue_destroy_one(one);
			} else if (one->cmd == NEMOPLAY_QUEUE_DONE_COMMAND) {
				nemoplay_queue_destroy_one(one);

				break;
			}
		}
	} while (1);

	ao_close(device);
	ao_shutdown();

	return NULL;
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
	struct nemoplay *play;
	struct playshader *shader;
	struct nemotimer *timer;
	pthread_t thread0;
	pthread_t thread1;
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

	context->play = play = nemoplay_create();
	if (context->play == NULL)
		goto err2;

	nemoplay_prepare_media(play, mediapath);

	pthread_create(&thread0, NULL, nemoplay_handle_decodeframe, (void *)context);
	pthread_create(&thread1, NULL, nemoplay_handle_audioplay, (void *)context);

	context->tool = tool = nemotool_create();
	if (tool == NULL)
		goto err3;
	nemotool_connect_wayland(tool, NULL);

	context->show = show = nemoshow_create_view(tool, width, height);
	if (show == NULL)
		goto err4;
	nemoshow_set_userdata(show, context);

	context->scene = scene = nemoshow_scene_create();
	nemoshow_scene_set_width(scene, width);
	nemoshow_scene_set_height(scene, height);
	nemoshow_set_scene(show, scene);

	context->back = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_BACK_TYPE);
	nemoshow_canvas_set_fill_color(canvas, 0.0f, 0.0f, 0.0f, 0.0f);
	nemoshow_canvas_set_alpha(canvas, 0.0f);
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

err4:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

err3:
	nemoplay_set_state(play, NEMOPLAY_DONE_STATE);
	pthread_join(thread0, NULL);
	pthread_join(thread1, NULL);
	nemoplay_destroy(play);

err2:
	free(context);

err1:
	free(mediapath);

	return 0;
}
