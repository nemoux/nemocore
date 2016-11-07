#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <pthread.h>
#include <ao/ao.h>

#include <playback.h>
#include <nemoplay.h>
#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>
#include <nemomisc.h>

struct playback_decoder {
	struct nemoplay *play;

	pthread_t thread;

	double pts_to_seek;
};

static void *nemoplay_back_handle_decoder(void *arg)
{
	struct playback_decoder *decoder = (struct playback_decoder *)arg;
	struct nemoplay *play = decoder->play;
	int state;

	nemoplay_enter_thread(play);

	while ((state = nemoplay_get_state(play)) != NEMOPLAY_DONE_STATE) {
		if (nemoplay_has_cmd(play, NEMOPLAY_SEEK_CMD) != 0) {
			nemoplay_seek_media(play, decoder->pts_to_seek);
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

struct playback_decoder *nemoplay_back_create_decoder(struct nemoplay *play)
{
	struct playback_decoder *decoder;

	decoder = (struct playback_decoder *)malloc(sizeof(struct playback_decoder));
	if (decoder == NULL)
		return NULL;
	memset(decoder, 0, sizeof(struct playback_decoder));

	decoder->play = play;

	pthread_create(&decoder->thread, NULL, nemoplay_back_handle_decoder, (void *)decoder);

	return decoder;
}

void nemoplay_back_destroy_decoder(struct playback_decoder *decoder)
{
	struct nemoplay *play = decoder->play;

	nemoplay_set_state(play, NEMOPLAY_DONE_STATE);
	nemoplay_wait_thread(play);

	free(decoder);
}

void nemoplay_back_seek_decoder(struct playback_decoder *decoder, double pts)
{
	decoder->pts_to_seek = pts;

	nemoplay_set_cmd(decoder->play, NEMOPLAY_SEEK_CMD);
}

struct playback_audio {
	struct nemoplay *play;

	pthread_t thread;
};

static void *nemoplay_back_handle_audio(void *arg)
{
	struct playback_audio *audio = (struct playback_audio *)arg;
	struct nemoplay *play = audio->play;
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
	if (device == NULL) {
		nemoplay_revoke_audio(play);
		goto out;
	}

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

out:
	ao_shutdown();

	nemoplay_leave_thread(play);

	return NULL;
}

struct playback_audio *nemoplay_back_create_audio_by_ao(struct nemoplay *play)
{
	struct playback_audio *audio;

	audio = (struct playback_audio *)malloc(sizeof(struct playback_audio));
	if (audio == NULL)
		return NULL;
	memset(audio, 0, sizeof(struct playback_audio));

	audio->play = play;

	pthread_create(&audio->thread, NULL, nemoplay_back_handle_audio, (void *)audio);

	return audio;
}

void nemoplay_back_destroy_audio(struct playback_audio *audio)
{
	struct nemoplay *play = audio->play;

	nemoplay_set_state(play, NEMOPLAY_DONE_STATE);
	nemoplay_wait_thread(play);

	free(audio);
}

struct playback_video {
	struct nemoplay *play;

	struct nemotool *tool;
	struct nemotimer *timer;

	struct playshader *shader;
	struct showone *canvas;

	nemoplay_back_video_update_t dispatch_update;
	nemoplay_back_video_done_t dispatch_done;
	void *data;
};

static void nemoplay_back_handle_video(struct nemotimer *timer, void *data)
{
	struct playback_video *video = (struct playback_video *)data;
	struct nemoplay *play = video->play;
	struct playqueue *queue;
	struct playone *one;
	int state;

	queue = nemoplay_get_video_queue(play);

	state = nemoplay_queue_get_state(queue);
	if (state == NEMOPLAY_QUEUE_NORMAL_STATE) {
		double threshold = 1.0f / nemoplay_get_video_framerate(play);
		double cts = nemoplay_get_cts(play);
		double pts;

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
				if (nemoplay_shader_get_texture_linesize(video->shader) != nemoplay_queue_get_one_width(one))
					nemoplay_shader_set_texture_linesize(video->shader, nemoplay_queue_get_one_width(one));

				nemoplay_shader_update(video->shader,
						nemoplay_queue_get_one_y(one),
						nemoplay_queue_get_one_u(one),
						nemoplay_queue_get_one_v(one));
				nemoplay_shader_dispatch(video->shader);

				if (video->dispatch_update != NULL)
					video->dispatch_update(video->play, video->data);

				if (nemoplay_queue_peek_pts(queue, &pts) != 0)
					nemotimer_set_timeout(timer, MINMAX(pts > cts ? pts - cts : 1.0f, 1.0f, threshold) * 1000);
				else
					nemotimer_set_timeout(timer, threshold * 1000);

				nemoplay_queue_destroy_one(one);

				nemoplay_next_frame(play);
			}
		}

		if (cts >= nemoplay_get_duration(play)) {
			if (video->dispatch_done != NULL)
				video->dispatch_done(video->play, video->data);
		}
	} else if (state == NEMOPLAY_QUEUE_STOP_STATE) {
		nemotimer_set_timeout(timer, 1000 / nemoplay_get_video_framerate(play));
	}
}

struct playback_video *nemoplay_back_create_video_by_timer(struct nemoplay *play, struct nemotool *tool)
{
	struct playback_video *video;

	video = (struct playback_video *)malloc(sizeof(struct playback_video));
	if (video == NULL)
		return NULL;
	memset(video, 0, sizeof(struct playback_video));

	video->play = play;
	video->tool = tool;

	video->shader = nemoplay_shader_create();
	nemoplay_shader_prepare(video->shader,
			NEMOPLAY_TO_RGBA_VERTEX_SHADER,
			NEMOPLAY_TO_RGBA_FRAGMENT_SHADER);
	nemoplay_shader_set_texture(video->shader,
			nemoplay_get_video_width(play),
			nemoplay_get_video_height(play));

	video->timer = nemotimer_create(tool);
	nemotimer_set_callback(video->timer, nemoplay_back_handle_video);
	nemotimer_set_userdata(video->timer, video);
	nemotimer_set_timeout(video->timer, 10);

	return video;
}

void nemoplay_back_destroy_video(struct playback_video *video)
{
	struct nemoplay *play = video->play;

	nemoplay_set_state(play, NEMOPLAY_DONE_STATE);

	nemotimer_destroy(video->timer);

	nemoplay_shader_destroy(video->shader);

	free(video);
}

void nemoplay_back_resize_video(struct playback_video *video, int width, int height)
{
	nemoplay_shader_set_viewport(video->shader,
			nemoshow_canvas_get_texture(video->canvas),
			width, height);

	if (nemoplay_get_frame(video->play) != 0)
		nemoplay_shader_dispatch(video->shader);
}

void nemoplay_back_redraw_video(struct playback_video *video)
{
	if (nemoplay_get_frame(video->play) != 0)
		nemoplay_shader_dispatch(video->shader);
}

void nemoplay_back_set_video_canvas(struct playback_video *video, struct showone *canvas, int width, int height)
{
	video->canvas = canvas;

	nemoplay_shader_set_viewport(video->shader,
			nemoshow_canvas_get_texture(video->canvas),
			width, height);
}

void nemoplay_back_set_video_update(struct playback_video *video, nemoplay_back_video_update_t dispatch)
{
	video->dispatch_update = dispatch;
}

void nemoplay_back_set_video_done(struct playback_video *video, nemoplay_back_video_done_t dispatch)
{
	video->dispatch_done = dispatch;
}

void nemoplay_back_set_video_data(struct playback_video *video, void *data)
{
	video->data = data;
}
