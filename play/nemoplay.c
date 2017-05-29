#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemoplay.h>
#include <nemomisc.h>

void __attribute__((constructor(101))) nemoplay_initialize(void)
{
	av_register_all();
}

void __attribute__((destructor(101))) nemoplay_finalize(void)
{
}

struct nemoplay *nemoplay_create(void)
{
	struct nemoplay *play;

	play = (struct nemoplay *)malloc(sizeof(struct nemoplay));
	if (play == NULL)
		return NULL;
	memset(play, 0, sizeof(struct nemoplay));

	if (pthread_mutex_init(&play->lock, NULL) != 0)
		goto err1;

	if (pthread_cond_init(&play->signal, NULL) != 0)
		goto err2;

	play->video_queue = nemoplay_queue_create();
	play->audio_queue = nemoplay_queue_create();

	play->clock = nemoplay_clock_create();

	return play;

err2:
	pthread_mutex_destroy(&play->lock);

err1:
	free(play);

	return NULL;
}

void nemoplay_destroy(struct nemoplay *play)
{
	nemoplay_clock_destroy(play->clock);

	nemoplay_queue_destroy(play->video_queue);
	nemoplay_queue_destroy(play->audio_queue);

	if (play->swr != NULL)
		swr_free(&play->swr);

	if (play->video_context != NULL)
		avcodec_close(play->video_context);
	if (play->audio_context != NULL)
		avcodec_close(play->audio_context);

	if (play->container != NULL) {
		avformat_close_input(&play->container);
		avformat_free_context(play->container);
	}

	if (play->video_opts != NULL)
		av_dict_free(&play->video_opts);
	if (play->audio_opts != NULL)
		av_dict_free(&play->audio_opts);

	pthread_cond_destroy(&play->signal);
	pthread_mutex_destroy(&play->lock);

	if (play->path != NULL)
		free(play->path);

	free(play);
}

void nemoplay_set_video_stropt(struct nemoplay *play, const char *key, const char *value)
{
	av_dict_set(&play->video_opts, key, value, 0);
}

void nemoplay_set_video_intopt(struct nemoplay *play, const char *key, int64_t value)
{
	av_dict_set_int(&play->video_opts, key, value, 0);
}

void nemoplay_set_audio_stropt(struct nemoplay *play, const char *key, const char *value)
{
	av_dict_set(&play->audio_opts, key, value, 0);
}

void nemoplay_set_audio_intopt(struct nemoplay *play, const char *key, int64_t value)
{
	av_dict_set_int(&play->audio_opts, key, value, 0);
}

void nemoplay_set_flags(struct nemoplay *play, uint32_t flags)
{
	pthread_mutex_lock(&play->lock);

	play->flags |= flags;

	pthread_mutex_unlock(&play->lock);
}

void nemoplay_put_flags(struct nemoplay *play, uint32_t flags)
{
	pthread_mutex_lock(&play->lock);

	play->flags &= ~flags;

	pthread_mutex_unlock(&play->lock);
}

void nemoplay_put_flags_all(struct nemoplay *play)
{
	pthread_mutex_lock(&play->lock);

	play->flags = 0x0;

	pthread_mutex_unlock(&play->lock);
}

int nemoplay_has_flags(struct nemoplay *play, uint32_t flags)
{
	return play->flags & flags;
}

int nemoplay_has_flags_all(struct nemoplay *play, uint32_t flags)
{
	return (play->flags & flags) == flags;
}

int nemoplay_has_no_flags(struct nemoplay *play)
{
	return play->flags == 0x0;
}

void nemoplay_reset_media(struct nemoplay *play)
{
	nemoplay_put_flags_all(play);
}

int nemoplay_load_media(struct nemoplay *play, const char *mediapath)
{
	AVFormatContext *container;
	AVCodecContext *context;
	AVCodec *codec;
	AVCodecContext *video_context = NULL;
	AVCodecContext *audio_context = NULL;
	int video_stream = -1;
	int audio_stream = -1;
	int i;

	container = avformat_alloc_context();
	if (container == NULL)
		return -1;

	if (avformat_open_input(&container, mediapath, NULL, NULL) < 0)
		goto err1;

	if (avformat_find_stream_info(container, NULL) < 0)
		goto err2;

	for (i = 0; i < container->nb_streams; i++) {
		context = container->streams[i]->codec;

		if (context->codec_type == AVMEDIA_TYPE_VIDEO) {
			video_stream = i;
			video_context = context;

			codec = avcodec_find_decoder(context->codec_id);
			if (codec != NULL)
				avcodec_open2(context, codec, &play->video_opts);
		} else if (context->codec_type == AVMEDIA_TYPE_AUDIO) {
			audio_stream = i;
			audio_context = context;

			codec = avcodec_find_decoder(context->codec_id);
			if (codec != NULL)
				avcodec_open2(context, codec, &play->audio_opts);
		}
	}

	video_stream = av_find_best_stream(container, AVMEDIA_TYPE_VIDEO, video_stream, -1, NULL, 0);
	audio_stream = av_find_best_stream(container, AVMEDIA_TYPE_AUDIO, audio_stream, video_stream, NULL, 0);

	if (video_context != NULL) {
		play->video_width = video_context->width;
		play->video_height = video_context->height;
		play->video_timebase = av_q2d(container->streams[video_stream]->time_base);

		if (video_context->pix_fmt == AV_PIX_FMT_BGRA)
			play->pixel_format = NEMOPLAY_BGRA_PIXEL_FORMAT;
		else if (video_context->pix_fmt == AV_PIX_FMT_RGBA)
			play->pixel_format = NEMOPLAY_RGBA_PIXEL_FORMAT;
		else
			play->pixel_format = NEMOPLAY_YUV420_PIXEL_FORMAT;
	}

	if (audio_context != NULL) {
		SwrContext *swr;

		play->audio_channels = audio_context->channels;
		play->audio_samplerate = audio_context->sample_rate;
		play->audio_samplebits = 16;
		play->audio_timebase = av_q2d(container->streams[audio_stream]->time_base);

		swr = swr_alloc();
		if (audio_context->channel_layout != 0) {
			av_opt_set_int(swr, "in_channel_layout", audio_context->channel_layout, 0);
			av_opt_set_int(swr, "out_channel_layout", audio_context->channel_layout, 0);
		} else if (audio_context->channels == 2) {
			av_opt_set_int(swr, "in_channel_layout", AV_CH_FRONT_LEFT | AV_CH_FRONT_RIGHT, 0);
			av_opt_set_int(swr, "out_channel_layout", AV_CH_FRONT_LEFT | AV_CH_FRONT_RIGHT, 0);
		} else if (audio_context->channels == 1) {
			av_opt_set_int(swr, "in_channel_layout", AV_CH_FRONT_CENTER, 0);
			av_opt_set_int(swr, "out_channel_layout", AV_CH_FRONT_CENTER, 0);
		}
		av_opt_set_int(swr, "in_sample_rate", audio_context->sample_rate, 0);
		av_opt_set_int(swr, "out_sample_rate", audio_context->sample_rate, 0);
		av_opt_set_sample_fmt(swr, "in_sample_fmt", audio_context->sample_fmt, 0);
		av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
		swr_init(swr);

		play->swr = swr;
	}

	play->container = container;

	play->duration = (float)container->duration / (float)AV_TIME_BASE;

	play->video_context = video_context;
	play->audio_context = audio_context;
	play->video_stream = video_stream;
	play->audio_stream = audio_stream;

	play->video_framerate = av_q2d(av_guess_frame_rate(container, container->streams[video_stream], NULL));
	play->video_framecount = ceil(play->video_framerate * play->duration);

	play->flags = 0x0;
	play->frame = 0;
	play->path = strdup(mediapath);

	return 0;

err2:
	avformat_close_input(&container);

err1:
	avformat_free_context(container);

	return -1;
}

void nemoplay_unload_media(struct nemoplay *play)
{
	nemoplay_queue_flush(play->video_queue);
	nemoplay_queue_flush(play->audio_queue);

	if (play->swr != NULL) {
		swr_free(&play->swr);
		play->swr = NULL;
	}

	if (play->video_context != NULL) {
		avcodec_close(play->video_context);
		play->video_context = NULL;
	}
	if (play->audio_context != NULL) {
		avcodec_close(play->audio_context);
		play->audio_context = NULL;
	}

	if (play->container != NULL) {
		avformat_close_input(&play->container);
		avformat_free_context(play->container);
		play->container = NULL;
	}

	if (play->path != NULL) {
		free(play->path);
		play->path = NULL;
	}
}

int nemoplay_seek_media(struct nemoplay *play, double pts)
{
	if (avformat_seek_file(play->container, -1, INT64_MIN, (int64_t)(pts * AV_TIME_BASE), INT64_MAX, 0) >= 0) {
		if (play->video_context != NULL)
			avcodec_flush_buffers(play->video_context);
		if (play->audio_context != NULL)
			avcodec_flush_buffers(play->audio_context);

		return 1;
	}

	return 0;
}

void nemoplay_wait_media(struct nemoplay *play)
{
	pthread_mutex_lock(&play->lock);

	if (nemoplay_has_flags(play, NEMOPLAY_DONE_FLAG) == 0)
		pthread_cond_wait(&play->signal, &play->lock);

	pthread_mutex_unlock(&play->lock);
}

void nemoplay_wake_media(struct nemoplay *play)
{
	pthread_mutex_lock(&play->lock);

	pthread_cond_signal(&play->signal);

	pthread_mutex_unlock(&play->lock);
}

int nemoplay_decode_media(struct nemoplay *play, int maxcount)
{
	AVFormatContext *container = play->container;
	AVCodecContext *video_context = play->video_context;
	AVCodecContext *audio_context = play->audio_context;
	AVFrame *frame;
	AVPacket packet;
	int video_stream = play->video_stream;
	int audio_stream = play->audio_stream;
	int finished = 0;
	int done = 0;
	int i;

	frame = av_frame_alloc();

	for (i = 0; i < maxcount && nemoplay_has_no_flags(play) != 0; i++) {
		if (av_read_frame(container, &packet) < 0) {
			done = 1;
			break;
		}

		if (packet.stream_index == video_stream) {
			avcodec_decode_video2(video_context, frame, &finished, &packet);

			if (finished != 0) {
				if (NEMOPLAY_PIXEL_IS_YUV420_FORMAT(play->pixel_format)) {
					struct playone *one;
					void *y, *u, *v;

					y = malloc(frame->linesize[0] * frame->height);
					u = malloc(frame->linesize[1] * frame->height / 2);
					v = malloc(frame->linesize[2] * frame->height / 2);

					memcpy(y, frame->data[0], frame->linesize[0] * frame->height);
					memcpy(u, frame->data[1], frame->linesize[1] * frame->height / 2);
					memcpy(v, frame->data[2], frame->linesize[2] * frame->height / 2);

					one = nemoplay_one_create();
					one->cmd = NEMOPLAY_QUEUE_NORMAL_COMMAND;
					one->pts = play->video_timebase * av_frame_get_best_effort_timestamp(frame);
					one->serial = play->video_queue->serial;

					one->data[0] = y;
					one->data[1] = u;
					one->data[2] = v;
					one->linesize[0] = frame->linesize[0];
					one->linesize[1] = frame->linesize[1];
					one->linesize[2] = frame->linesize[2];
					one->height = frame->height;

					nemoplay_queue_enqueue(play->video_queue, one);
				} else if (NEMOPLAY_PIXEL_IS_RGBA_FORMAT(play->pixel_format)) {
					struct playone *one;
					void *buffer;

					buffer = malloc(frame->linesize[0] * frame->height);

					memcpy(buffer, frame->data[0], frame->linesize[0] * frame->height);

					one = nemoplay_one_create();
					one->cmd = NEMOPLAY_QUEUE_NORMAL_COMMAND;
					one->pts = play->video_timebase * av_frame_get_best_effort_timestamp(frame);
					one->serial = play->video_queue->serial;

					one->data[0] = buffer;
					one->linesize[0] = frame->linesize[0];
					one->height = frame->height;

					nemoplay_queue_enqueue(play->video_queue, one);
				}
			}

			av_frame_unref(frame);
		} else if (packet.stream_index == audio_stream) {
			uint8_t *buffer;
			int buffersize;
			int samplesize;

			avcodec_decode_audio4(audio_context, frame, &finished, &packet);

			buffersize = av_samples_get_buffer_size(NULL, audio_context->channels, frame->nb_samples, audio_context->sample_fmt, 1);

			if (finished != 0) {
				struct playone *one;

				buffer = (uint8_t *)malloc(buffersize);

				samplesize = swr_convert(play->swr, &buffer, buffersize, (const uint8_t **)frame->extended_data, frame->nb_samples);

				one = nemoplay_one_create();
				one->cmd = NEMOPLAY_QUEUE_NORMAL_COMMAND;
				one->pts = play->audio_timebase * av_frame_get_best_effort_timestamp(frame);
				one->serial = play->audio_queue->serial;

				one->data[0] = buffer;
				one->linesize[0] = samplesize * audio_context->channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);

				nemoplay_queue_enqueue(play->audio_queue, one);
			}

			av_frame_unref(frame);
		}

		if ((video_context == NULL || nemoplay_queue_get_count(play->video_queue) > maxcount) &&
				(audio_context == NULL || nemoplay_queue_get_count(play->audio_queue) > maxcount))
			nemoplay_wait_media(play);

		av_packet_unref(&packet);
	}

	av_frame_free(&frame);

	return done;
}

int nemoplay_extract_video(struct nemoplay *play, struct playbox *box, int maxcount)
{
	AVFormatContext *container = play->container;
	AVCodecContext *video_context = play->video_context;
	AVFrame *frame;
	AVPacket packet;
	int video_stream = play->video_stream;
	int finished = 0;
	int i;

	frame = av_frame_alloc();

	for (i = 0; i < maxcount && nemoplay_has_no_flags(play) != 0; i++) {
		if (av_read_frame(container, &packet) < 0)
			break;

		if (packet.stream_index == video_stream) {
			avcodec_decode_video2(video_context, frame, &finished, &packet);

			if (finished != 0) {
				if (NEMOPLAY_PIXEL_IS_YUV420_FORMAT(play->pixel_format)) {
					struct playone *one;
					void *y, *u, *v;

					y = malloc(frame->linesize[0] * frame->height);
					u = malloc(frame->linesize[1] * frame->height / 2);
					v = malloc(frame->linesize[2] * frame->height / 2);

					memcpy(y, frame->data[0], frame->linesize[0] * frame->height);
					memcpy(u, frame->data[1], frame->linesize[1] * frame->height / 2);
					memcpy(v, frame->data[2], frame->linesize[2] * frame->height / 2);

					one = nemoplay_one_create();
					one->cmd = NEMOPLAY_QUEUE_NORMAL_COMMAND;
					one->pts = play->video_timebase * av_frame_get_best_effort_timestamp(frame);
					one->serial = play->video_queue->serial;

					one->data[0] = y;
					one->data[1] = u;
					one->data[2] = v;
					one->linesize[0] = frame->linesize[0];
					one->linesize[1] = frame->linesize[1];
					one->linesize[2] = frame->linesize[2];
					one->height = frame->height;

					nemoplay_box_insert_one(box, one);
				} else if (NEMOPLAY_PIXEL_IS_RGBA_FORMAT(play->pixel_format)) {
					struct playone *one;
					void *buffer;

					buffer = malloc(frame->linesize[0] * frame->height);

					memcpy(buffer, frame->data[0], frame->linesize[0] * frame->height);

					one = nemoplay_one_create();
					one->cmd = NEMOPLAY_QUEUE_NORMAL_COMMAND;
					one->pts = play->video_timebase * av_frame_get_best_effort_timestamp(frame);
					one->serial = play->video_queue->serial;

					one->data[0] = buffer;
					one->linesize[0] = frame->linesize[0];
					one->height = frame->height;

					nemoplay_box_insert_one(box, one);
				}
			}

			av_frame_unref(frame);
		}

		av_packet_unref(&packet);
	}

	av_frame_free(&frame);

	return i;
}

void nemoplay_revoke_video(struct nemoplay *play)
{
	if (play->video_context != NULL) {
		avcodec_close(play->video_context);

		play->video_context = NULL;
		play->video_stream = -1;
	}
}

void nemoplay_revoke_audio(struct nemoplay *play)
{
	if (play->audio_context != NULL) {
		avcodec_close(play->audio_context);

		play->audio_context = NULL;
		play->audio_stream = -1;
	}
}

void nemoplay_enter_thread(struct nemoplay *play)
{
	pthread_mutex_lock(&play->lock);

	play->threadcount++;

	pthread_cond_signal(&play->signal);

	pthread_mutex_unlock(&play->lock);
}

void nemoplay_leave_thread(struct nemoplay *play)
{
	pthread_mutex_lock(&play->lock);

	play->threadcount--;

	pthread_cond_signal(&play->signal);

	pthread_mutex_unlock(&play->lock);
}

void nemoplay_wait_thread(struct nemoplay *play)
{
	pthread_mutex_lock(&play->lock);

	while (play->threadcount > 0)
		pthread_cond_wait(&play->signal, &play->lock);

	pthread_mutex_unlock(&play->lock);
}

void nemoplay_set_video_pts(struct nemoplay *play, double pts)
{
	if (play->audio_context == NULL)
		nemoplay_clock_set(play->clock, pts);
}

void nemoplay_set_audio_pts(struct nemoplay *play, double pts)
{
	nemoplay_clock_set(play->clock, pts);
}

void nemoplay_set_clock_cts(struct nemoplay *play, double cts)
{
	nemoplay_clock_set(play->clock, cts);
}

double nemoplay_get_clock_cts(struct nemoplay *play)
{
	return nemoplay_clock_get(play->clock);
}

void nemoplay_set_clock_state(struct nemoplay *play, int state)
{
	nemoplay_clock_set_state(play->clock, state);
}

void nemoplay_set_clock_speed(struct nemoplay *play, double speed)
{
	nemoplay_clock_set_speed(play->clock, speed);
}

struct playone *nemoplay_one_create(void)
{
	struct playone *one;

	one = (struct playone *)malloc(sizeof(struct playone));
	if (one == NULL)
		return NULL;
	memset(one, 0, sizeof(struct playone));

	nemolist_init(&one->link);

	return one;
}

void nemoplay_one_destroy(struct playone *one)
{
	nemolist_remove(&one->link);

	if (one->data[0] != NULL)
		free(one->data[0]);
	if (one->data[1] != NULL)
		free(one->data[1]);
	if (one->data[2] != NULL)
		free(one->data[2]);
	if (one->data[3] != NULL)
		free(one->data[3]);

	free(one);
}
