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
#include <nemomisc.h>

struct playdecoder {
	struct nemoplay *play;

	pthread_t thread;

	double pts_to_seek;
};

static void *nemoplay_decoder_handle_thread(void *arg)
{
	struct playdecoder *decoder = (struct playdecoder *)arg;
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

struct playdecoder *nemoplay_decoder_create(struct nemoplay *play)
{
	struct playdecoder *decoder;

	decoder = (struct playdecoder *)malloc(sizeof(struct playdecoder));
	if (decoder == NULL)
		return NULL;
	memset(decoder, 0, sizeof(struct playdecoder));

	decoder->play = play;

	pthread_create(&decoder->thread, NULL, nemoplay_decoder_handle_thread, (void *)decoder);

	return decoder;
}

void nemoplay_decoder_destroy(struct playdecoder *decoder)
{
	struct nemoplay *play = decoder->play;

	nemoplay_set_state(play, NEMOPLAY_DONE_STATE);
	nemoplay_wait_thread(play);

	free(decoder);
}

void nemoplay_decoder_seek(struct playdecoder *decoder, double pts)
{
	decoder->pts_to_seek = pts;

	nemoplay_set_cmd(decoder->play, NEMOPLAY_SEEK_CMD);
}

struct playaudio {
	struct nemoplay *play;

	pthread_t thread;
};

static void *nemoplay_audio_handle_thread(void *arg)
{
	struct playaudio *audio = (struct playaudio *)arg;
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
			} else if (nemoplay_one_get_serial(one) != nemoplay_queue_get_serial(queue)) {
				nemoplay_one_destroy(one);
			} else if (nemoplay_one_get_cmd(one) == NEMOPLAY_QUEUE_NORMAL_COMMAND) {
				nemoplay_set_audio_pts(play, nemoplay_one_get_pts(one));

				ao_play(device,
						nemoplay_one_get_data(one, 0),
						nemoplay_one_get_linesize(one, 0));

				nemoplay_one_destroy(one);
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

struct playaudio *nemoplay_audio_create_by_ao(struct nemoplay *play)
{
	struct playaudio *audio;

	audio = (struct playaudio *)malloc(sizeof(struct playaudio));
	if (audio == NULL)
		return NULL;
	memset(audio, 0, sizeof(struct playaudio));

	audio->play = play;

	pthread_create(&audio->thread, NULL, nemoplay_audio_handle_thread, (void *)audio);

	return audio;
}

void nemoplay_audio_destroy(struct playaudio *audio)
{
	struct nemoplay *play = audio->play;

	nemoplay_set_state(play, NEMOPLAY_DONE_STATE);
	nemoplay_wait_thread(play);

	free(audio);
}

struct playvideo {
	struct nemoplay *play;

	struct nemotool *tool;
	struct nemotimer *timer;

	struct playshader *shader;

	nemoplay_frame_update_t dispatch_update;
	nemoplay_frame_done_t dispatch_done;
	void *data;
};

static void nemoplay_video_handle_timer(struct nemotimer *timer, void *data)
{
	struct playvideo *video = (struct playvideo *)data;
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
		} else if (nemoplay_one_get_serial(one) != nemoplay_queue_get_serial(queue)) {
			nemoplay_one_destroy(one);
			nemotimer_set_timeout(timer, 1);
		} else if (nemoplay_one_get_cmd(one) == NEMOPLAY_QUEUE_NORMAL_COMMAND) {
			nemoplay_set_video_pts(play, nemoplay_one_get_pts(one));

			if (cts > nemoplay_one_get_pts(one) + threshold) {
				nemoplay_one_destroy(one);
				nemotimer_set_timeout(timer, 1);
			} else if (cts < nemoplay_one_get_pts(one) - threshold) {
				nemoplay_queue_enqueue_tail(queue, one);
				nemotimer_set_timeout(timer, threshold * 1000);
			} else {
				nemoplay_shader_update(video->shader, one);
				nemoplay_shader_dispatch(video->shader);

				if (video->dispatch_update != NULL)
					video->dispatch_update(video->play, video->data);

				if (nemoplay_queue_peek_pts(queue, &pts) != 0)
					nemotimer_set_timeout(timer, MINMAX(pts > cts ? pts - cts : 1.0f, 1.0f, threshold) * 1000);
				else
					nemotimer_set_timeout(timer, threshold * 1000);

				nemoplay_one_destroy(one);

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

struct playvideo *nemoplay_video_create_by_timer(struct nemoplay *play, struct nemotool *tool)
{
	struct playvideo *video;

	video = (struct playvideo *)malloc(sizeof(struct playvideo));
	if (video == NULL)
		return NULL;
	memset(video, 0, sizeof(struct playvideo));

	video->play = play;
	video->tool = tool;

	video->shader = nemoplay_shader_create();
	nemoplay_shader_set_format(video->shader,
			nemoplay_get_pixel_format(play));
	nemoplay_shader_set_texture(video->shader,
			nemoplay_get_video_width(play),
			nemoplay_get_video_height(play));

	video->timer = nemotimer_create(tool);
	nemotimer_set_callback(video->timer, nemoplay_video_handle_timer);
	nemotimer_set_userdata(video->timer, video);
	nemotimer_set_timeout(video->timer, 10);

	return video;
}

void nemoplay_video_destroy(struct playvideo *video)
{
	struct nemoplay *play = video->play;

	nemoplay_set_state(play, NEMOPLAY_DONE_STATE);

	nemotimer_destroy(video->timer);

	nemoplay_shader_destroy(video->shader);

	free(video);
}

void nemoplay_video_redraw(struct playvideo *video)
{
	if (nemoplay_get_frame(video->play) != 0)
		nemoplay_shader_dispatch(video->shader);
}

void nemoplay_video_set_texture(struct playvideo *video, uint32_t texture, int width, int height)
{
	nemoplay_shader_set_viewport(video->shader, texture, width, height);

	if (nemoplay_get_frame(video->play) != 0)
		nemoplay_shader_dispatch(video->shader);
}

void nemoplay_video_set_update(struct playvideo *video, nemoplay_frame_update_t dispatch)
{
	video->dispatch_update = dispatch;
}

void nemoplay_video_set_done(struct playvideo *video, nemoplay_frame_done_t dispatch)
{
	video->dispatch_done = dispatch;
}

void nemoplay_video_set_data(struct playvideo *video, void *data)
{
	video->data = data;
}

struct playextractor {
	struct nemoplay *play;

	struct playbox *box;

	pthread_t thread;
};

static void *nemoplay_extractor_handle_thread(void *arg)
{
	struct playextractor *extractor = (struct playextractor *)arg;
	struct nemoplay *play = extractor->play;
	struct playbox *box = extractor->box;

	nemoplay_enter_thread(play);

	nemoplay_extract_video(play, box);

	nemoplay_leave_thread(play);

	nemoplay_set_state(play, NEMOPLAY_DONE_STATE);

	return NULL;
}

struct playextractor *nemoplay_extractor_create(struct nemoplay *play, struct playbox *box)
{
	struct playextractor *extractor;

	extractor = (struct playextractor *)malloc(sizeof(struct playextractor));
	if (extractor == NULL)
		return NULL;
	memset(extractor, 0, sizeof(struct playextractor));

	extractor->play = play;
	extractor->box = box;

	pthread_create(&extractor->thread, NULL, nemoplay_extractor_handle_thread, (void *)extractor);

	return extractor;
}

void nemoplay_extractor_destroy(struct playextractor *extractor)
{
	struct nemoplay *play = extractor->play;

	nemoplay_set_state(play, NEMOPLAY_DONE_STATE);
	nemoplay_wait_thread(play);

	free(extractor);
}
