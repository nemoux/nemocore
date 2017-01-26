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

	int maxcount;
};

static void *nemoplay_decoder_handle_thread(void *arg)
{
	struct playdecoder *decoder = (struct playdecoder *)arg;
	struct nemoplay *play = decoder->play;
	int state;

	while (nemoplay_has_flags(play, NEMOPLAY_DONE_FLAG) == 0) {
		if (nemoplay_has_flags(play, NEMOPLAY_SEEK_FLAG) != 0) {
			nemoplay_put_flags(play, NEMOPLAY_SEEK_FLAG);

			nemoplay_queue_flush(play->video_queue);
			nemoplay_queue_flush(play->audio_queue);

			nemoplay_seek_media(play, decoder->pts_to_seek);

			nemoplay_set_clock_cts(play, decoder->pts_to_seek);
		}

		if (nemoplay_has_no_flags(play) != 0) {
			if (nemoplay_decode_media(play, decoder->maxcount) != 0)
				nemoplay_set_flags(play, NEMOPLAY_EOF_FLAG);
		} else {
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
	decoder->maxcount = nemoplay_get_video_framerate(play);

	nemoplay_enter_thread(play);

	pthread_create(&decoder->thread, NULL, nemoplay_decoder_handle_thread, (void *)decoder);

	return decoder;
}

void nemoplay_decoder_destroy(struct playdecoder *decoder)
{
	struct nemoplay *play = decoder->play;

	nemoplay_set_flags(play, NEMOPLAY_DONE_FLAG);
	nemoplay_wake_media(play);
	nemoplay_queue_set_state(play->video_queue, NEMOPLAY_QUEUE_DONE_STATE);
	nemoplay_queue_set_state(play->audio_queue, NEMOPLAY_QUEUE_DONE_STATE);

	nemoplay_wait_thread(play);

	free(decoder);
}

void nemoplay_decoder_play(struct playdecoder *decoder)
{
	nemoplay_put_flags(decoder->play, NEMOPLAY_STOP_FLAG | NEMOPLAY_EOF_FLAG);

	nemoplay_set_clock_state(decoder->play, NEMOPLAY_CLOCK_NORMAL_STATE);
}

void nemoplay_decoder_stop(struct playdecoder *decoder)
{
	nemoplay_set_flags(decoder->play, NEMOPLAY_STOP_FLAG);

	nemoplay_set_clock_state(decoder->play, NEMOPLAY_CLOCK_STOP_STATE);
}

void nemoplay_decoder_seek(struct playdecoder *decoder, double pts)
{
	decoder->pts_to_seek = pts;

	nemoplay_set_flags(decoder->play, NEMOPLAY_SEEK_FLAG);
}

void nemoplay_decoder_set_maxcount(struct playdecoder *decoder, int maxcount)
{
	decoder->maxcount = maxcount;
}

struct playaudio {
	struct nemoplay *play;
	struct playqueue *queue;

	pthread_t thread;

	int mincount;
};

static void *nemoplay_audio_handle_thread(void *arg)
{
	struct playaudio *audio = (struct playaudio *)arg;
	struct nemoplay *play = audio->play;
	struct playqueue *queue = audio->queue;
	struct playone *one;
	ao_device *device;
	ao_sample_format format;
	int driver;
	int state;

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

	while ((state = nemoplay_queue_get_state(queue)) != NEMOPLAY_QUEUE_DONE_STATE) {
		if (state == NEMOPLAY_QUEUE_NORMAL_STATE) {
			if (nemoplay_has_flags(play, NEMOPLAY_EOF_FLAG) == 0 && nemoplay_queue_get_count(queue) < audio->mincount)
				nemoplay_wake_media(play);

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
	audio->queue = nemoplay_get_audio_queue(play);
	audio->mincount = nemoplay_get_video_framerate(play);

	nemoplay_enter_thread(play);

	pthread_create(&audio->thread, NULL, nemoplay_audio_handle_thread, (void *)audio);

	return audio;
}

void nemoplay_audio_destroy(struct playaudio *audio)
{
	struct nemoplay *play = audio->play;

	nemoplay_set_flags(play, NEMOPLAY_DONE_FLAG);
	nemoplay_wake_media(play);
	nemoplay_queue_set_state(play->video_queue, NEMOPLAY_QUEUE_DONE_STATE);
	nemoplay_queue_set_state(play->audio_queue, NEMOPLAY_QUEUE_DONE_STATE);

	nemoplay_wait_thread(play);

	free(audio);
}

void nemoplay_audio_play(struct playaudio *audio)
{
	nemoplay_queue_set_state(audio->queue, NEMOPLAY_QUEUE_NORMAL_STATE);
}

void nemoplay_audio_stop(struct playaudio *audio)
{
	nemoplay_queue_set_state(audio->queue, NEMOPLAY_QUEUE_STOP_STATE);
}

void nemoplay_audio_set_mincount(struct playaudio *audio, int mincount)
{
	audio->mincount = mincount;
}

void __attribute__((constructor(101))) nemoplay_audio_initialize(void)
{
	ao_initialize();
}

void __attribute__((destructor(101))) nemoplay_audio_finalize(void)
{
	ao_shutdown();
}

struct playvideo {
	struct nemoplay *play;
	struct playqueue *queue;
	struct playshader *shader;

	double framerate;
	double droprate;
	double screenrate;

	int mincount;

	struct nemotimer *timer;

	nemoplay_frame_update_t dispatch_update;
	nemoplay_frame_done_t dispatch_done;
	void *data;
};

static void nemoplay_video_handle_timer(struct nemotimer *timer, void *data)
{
	struct playvideo *video = (struct playvideo *)data;
	struct nemoplay *play = video->play;
	struct playqueue *queue = video->queue;
	struct playone *one;
	double droprate = 1.0f / video->framerate * video->droprate;
	double screenrate = 1.0f / video->screenrate;
	int state;

	state = nemoplay_queue_get_state(queue);
	if (state == NEMOPLAY_QUEUE_NORMAL_STATE) {
		double cts = nemoplay_get_clock_cts(play);
		double nts;

		if (nemoplay_has_flags(play, NEMOPLAY_EOF_FLAG) == 0 && nemoplay_queue_get_count(queue) < video->mincount)
			nemoplay_wake_media(play);

retry_next:
		one = nemoplay_queue_dequeue(queue);
		if (one == NULL) {
			if (nemoplay_has_flags(play, NEMOPLAY_EOF_FLAG) == 0) {
				nemotimer_set_timeout(timer, screenrate * 1000);
			} else if (video->dispatch_done != NULL) {
				video->dispatch_done(video->play, video->data);
			}
		} else if (nemoplay_one_get_serial(one) != nemoplay_queue_get_serial(queue)) {
			nemoplay_one_destroy(one);
			goto retry_next;
		} else if (nemoplay_one_get_cmd(one) == NEMOPLAY_QUEUE_NORMAL_COMMAND) {
			if (cts > nemoplay_one_get_pts(one) + droprate) {
				nemoplay_one_destroy(one);
				goto retry_next;
			} else if (cts < nemoplay_one_get_pts(one) - screenrate) {
				nemoplay_queue_enqueue_tail(queue, one);
				nemotimer_set_timeout(timer, MAX((nemoplay_one_get_pts(one) - cts) * 1000, screenrate * 1000));
			} else {
				nemoplay_set_video_pts(play, cts);

				nemoplay_shader_update(video->shader, one);

				if (nemoplay_shader_get_viewport(video->shader) > 0)
					nemoplay_shader_dispatch(video->shader);

				if (video->dispatch_update != NULL)
					video->dispatch_update(video->play, video->data);

				if (nemoplay_queue_peek_pts(queue, &nts) != 0)
					nemotimer_set_timeout(timer, MAX((nts - cts) * 1000, screenrate * 1000));
				else
					nemotimer_set_timeout(timer, screenrate * 1000);

				nemoplay_one_destroy(one);

				nemoplay_next_frame(play);
			}
		}
	} else if (state == NEMOPLAY_QUEUE_STOP_STATE) {
		nemotimer_set_timeout(timer, screenrate * 1000);
	}
}

static void nemoplay_video_handle_timer_nosync(struct nemotimer *timer, void *data)
{
	struct playvideo *video = (struct playvideo *)data;
	struct nemoplay *play = video->play;
	struct playqueue *queue = video->queue;
	struct playone *one;
	double framerate = 1.0f / play->video_framerate;
	int state;

	state = nemoplay_queue_get_state(queue);
	if (state == NEMOPLAY_QUEUE_NORMAL_STATE) {
		double cts = nemoplay_get_clock_cts(play);

		if (nemoplay_has_flags(play, NEMOPLAY_EOF_FLAG) == 0 && nemoplay_queue_get_count(queue) < video->mincount)
			nemoplay_wake_media(play);

retry_next:
		one = nemoplay_queue_dequeue(queue);
		if (one == NULL) {
			if (nemoplay_has_flags(play, NEMOPLAY_EOF_FLAG) == 0) {
				nemotimer_set_timeout(timer, framerate * 1000);
			} else if (video->dispatch_done != NULL) {
				video->dispatch_done(video->play, video->data);
			}
		} else if (nemoplay_one_get_serial(one) != nemoplay_queue_get_serial(queue)) {
			nemoplay_one_destroy(one);
			goto retry_next;
		} else if (nemoplay_one_get_cmd(one) == NEMOPLAY_QUEUE_NORMAL_COMMAND) {
			nemoplay_set_video_pts(play, cts);

			nemoplay_shader_update(video->shader, one);

			if (nemoplay_shader_get_viewport(video->shader) > 0)
				nemoplay_shader_dispatch(video->shader);

			if (video->dispatch_update != NULL)
				video->dispatch_update(video->play, video->data);

			nemotimer_set_timeout(timer, framerate * 1000);

			nemoplay_one_destroy(one);

			nemoplay_next_frame(play);
		}
	} else if (state == NEMOPLAY_QUEUE_STOP_STATE) {
		nemotimer_set_timeout(timer, framerate * 1000);
	}
}

struct playvideo *nemoplay_video_create_by_timer(struct nemoplay *play)
{
	struct playvideo *video;

	video = (struct playvideo *)malloc(sizeof(struct playvideo));
	if (video == NULL)
		return NULL;
	memset(video, 0, sizeof(struct playvideo));

	video->play = play;
	video->queue = nemoplay_get_video_queue(play);
	video->framerate = nemoplay_get_video_framerate(play);
	video->droprate = 2.0f;
	video->screenrate = 60.0f;
	video->mincount = nemoplay_get_video_framerate(play);

	video->shader = nemoplay_shader_create();
	nemoplay_shader_set_format(video->shader,
			nemoplay_get_pixel_format(play));
	nemoplay_shader_resize(video->shader,
			nemoplay_get_video_width(play),
			nemoplay_get_video_height(play));

	video->timer = nemotimer_create(nemotool_get_instance());
	nemotimer_set_callback(video->timer, nemoplay_video_handle_timer);
	nemotimer_set_userdata(video->timer, video);
	nemotimer_set_timeout(video->timer, 1);

	return video;
}

void nemoplay_video_destroy(struct playvideo *video)
{
	struct nemoplay *play = video->play;

	nemoplay_set_flags(play, NEMOPLAY_DONE_FLAG);
	nemoplay_wake_media(play);
	nemoplay_queue_set_state(play->video_queue, NEMOPLAY_QUEUE_DONE_STATE);
	nemoplay_queue_set_state(play->audio_queue, NEMOPLAY_QUEUE_DONE_STATE);

	nemotimer_destroy(video->timer);

	nemoplay_shader_destroy(video->shader);

	free(video);
}

void nemoplay_video_redraw(struct playvideo *video)
{
	if (nemoplay_get_frame(video->play) != 0)
		nemoplay_shader_dispatch(video->shader);
}

void nemoplay_video_play(struct playvideo *video)
{
	nemoplay_queue_set_state(video->queue, NEMOPLAY_QUEUE_NORMAL_STATE);

	nemotimer_set_timeout(video->timer, 1);
}

void nemoplay_video_stop(struct playvideo *video)
{
	nemoplay_queue_set_state(video->queue, NEMOPLAY_QUEUE_STOP_STATE);

	nemotimer_set_timeout(video->timer, 0);
}

struct playshader *nemoplay_video_get_shader(struct playvideo *video)
{
	return video->shader;
}

void nemoplay_video_set_texture(struct playvideo *video, uint32_t texture, int width, int height)
{
	nemoplay_shader_set_viewport(video->shader, texture, width, height);

	if (nemoplay_get_frame(video->play) != 0)
		nemoplay_shader_dispatch(video->shader);
}

void nemoplay_video_set_drop_rate(struct playvideo *video, double rate)
{
	video->droprate = rate;

	if (rate > 0.0f)
		nemotimer_set_callback(video->timer, nemoplay_video_handle_timer);
	else
		nemotimer_set_callback(video->timer, nemoplay_video_handle_timer_nosync);
}

void nemoplay_video_set_screen_rate(struct playvideo *video, double rate)
{
	video->screenrate = rate;
}

void nemoplay_video_set_mincount(struct playvideo *video, int mincount)
{
	video->mincount = mincount;
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

	int maxcount;
};

static void *nemoplay_extractor_handle_thread(void *arg)
{
	struct playextractor *extractor = (struct playextractor *)arg;
	struct nemoplay *play = extractor->play;
	struct playbox *box = extractor->box;
	int maxcount = extractor->maxcount;

	while (nemoplay_has_no_flags(play) != 0 && nemoplay_extract_video(play, box, maxcount) > 0)
		sleep(1);

	nemoplay_leave_thread(play);

	return NULL;
}

struct playextractor *nemoplay_extractor_create(struct nemoplay *play, struct playbox *box, int maxcount)
{
	struct playextractor *extractor;

	extractor = (struct playextractor *)malloc(sizeof(struct playextractor));
	if (extractor == NULL)
		return NULL;
	memset(extractor, 0, sizeof(struct playextractor));

	extractor->play = play;
	extractor->box = box;
	extractor->maxcount = maxcount;

	nemoplay_enter_thread(play);

	pthread_create(&extractor->thread, NULL, nemoplay_extractor_handle_thread, (void *)extractor);

	return extractor;
}

void nemoplay_extractor_destroy(struct playextractor *extractor)
{
	struct nemoplay *play = extractor->play;

	nemoplay_set_flags(play, NEMOPLAY_DONE_FLAG);
	nemoplay_wake_media(play);
	nemoplay_queue_set_state(play->video_queue, NEMOPLAY_QUEUE_DONE_STATE);
	nemoplay_queue_set_state(play->audio_queue, NEMOPLAY_QUEUE_DONE_STATE);

	nemoplay_wait_thread(play);

	free(extractor);
}
