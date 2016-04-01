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
	play->subtitle_queue = nemoplay_queue_create();

	play->video_clock = nemoplay_clock_create();
	play->audio_clock = nemoplay_clock_create();

	play->max_queuesize = NEMOPLAY_MAX_QUEUESIZE;

	return play;

err2:
	pthread_mutex_destroy(&play->lock);

err1:
	free(play);

	return NULL;
}

void nemoplay_destroy(struct nemoplay *play)
{
	nemoplay_clock_destroy(play->video_clock);
	nemoplay_clock_destroy(play->audio_clock);

	nemoplay_queue_destroy(play->video_queue);
	nemoplay_queue_destroy(play->audio_queue);
	nemoplay_queue_destroy(play->subtitle_queue);

	if (play->container != NULL)
		avformat_close_input(&play->container);

	pthread_cond_destroy(&play->signal);
	pthread_mutex_destroy(&play->lock);

	free(play);
}

int nemoplay_prepare_media(struct nemoplay *play, const char *mediapath)
{
	AVFormatContext *container;
	AVCodecContext *context;
	AVCodec *codec;
	AVCodecContext *video_context = NULL;
	AVCodecContext *audio_context = NULL;
	AVCodecContext *subtitle_context = NULL;
	int video_stream = -1;
	int audio_stream = -1;
	int subtitle_stream = -1;
	int i;

	container = avformat_alloc_context();
	if (container == NULL)
		return -1;

	if (avformat_open_input(&container, mediapath, NULL, NULL) < 0)
		goto err1;

	if (avformat_find_stream_info(container, NULL) < 0)
		goto err1;

	for (i = 0; i < container->nb_streams; i++) {
		context = container->streams[i]->codec;

		if (context->codec_type == AVMEDIA_TYPE_VIDEO) {
			video_stream = i;
			video_context = context;

			codec = avcodec_find_decoder(context->codec_id);
			if (codec != NULL)
				avcodec_open2(context, codec, NULL);
		} else if (context->codec_type == AVMEDIA_TYPE_AUDIO) {
			audio_stream = i;
			audio_context = context;

			codec = avcodec_find_decoder(context->codec_id);
			if (codec != NULL)
				avcodec_open2(context, codec, NULL);
		} else if (context->codec_type == AVMEDIA_TYPE_SUBTITLE) {
			subtitle_stream = i;
			subtitle_context = context;
		}
	}

	video_stream = av_find_best_stream(container, AVMEDIA_TYPE_VIDEO, video_stream, -1, NULL, 0);
	audio_stream = av_find_best_stream(container, AVMEDIA_TYPE_AUDIO, audio_stream, video_stream, NULL, 0);
	subtitle_stream = av_find_best_stream(container, AVMEDIA_TYPE_SUBTITLE, subtitle_stream, audio_stream >= 0 ? audio_stream : video_stream, NULL, 0);

	if (video_context != NULL) {
		play->video_width = video_context->width;
		play->video_height = video_context->height;
	}

	if (audio_context != NULL) {
		play->audio_channels = audio_context->channels;
		play->audio_samplerate = audio_context->sample_rate;
		play->audio_samplebits = 16;
	}

	play->container = container;

	play->video_context = video_context;
	play->audio_context = audio_context;
	play->subtitle_context = subtitle_context;
	play->video_stream = video_stream;
	play->audio_stream = audio_stream;
	play->subtitle_stream = subtitle_stream;

	play->video_framerate = av_q2d(av_guess_frame_rate(container, container->streams[video_stream], NULL));

	return 0;

err1:
	avformat_close_input(&container);

	return -1;
}

void nemoplay_finish_media(struct nemoplay *play)
{
}

int nemoplay_decode_media(struct nemoplay *play)
{
	AVFormatContext *container = play->container;
	AVCodecContext *video_context = play->video_context;
	AVCodecContext *audio_context = play->audio_context;
	AVCodecContext *subtitle_context = play->subtitle_context;
	SwrContext *swr;
	AVFrame *frame;
	AVPacket packet;
	double video_timebase;
	double audio_timebase;
	int video_stream = play->video_stream;
	int audio_stream = play->audio_stream;
	int subtitle_stream = play->subtitle_stream;
	int finished = 0;

	play->state = NEMOPLAY_PLAYING_STATE;

	swr = swr_alloc();
	av_opt_set_int(swr, "in_channel_layout", audio_context->channel_layout, 0);
	av_opt_set_int(swr, "out_channel_layout", audio_context->channel_layout, 0);
	av_opt_set_int(swr, "in_sample_rate", audio_context->sample_rate, 0);
	av_opt_set_int(swr, "out_sample_rate", audio_context->sample_rate, 0);
	av_opt_set_sample_fmt(swr, "in_sample_fmt", audio_context->sample_fmt, 0);
	av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
	swr_init(swr);

	audio_timebase = av_q2d(container->streams[audio_stream]->time_base);
	video_timebase = av_q2d(container->streams[video_stream]->time_base);

	frame = av_frame_alloc();

	while (av_read_frame(container, &packet) >= 0) {
		if (play->state == NEMOPLAY_PLAYING_STATE) {
			if (packet.stream_index == video_stream) {
				avcodec_decode_video2(video_context, frame, &finished, &packet);

				if (finished != 0) {
					struct playone *one;
					uint8_t *y, *u, *v;

					y = (uint8_t *)malloc(frame->linesize[0] * frame->height);
					u = (uint8_t *)malloc(frame->linesize[1] * frame->height / 2);
					v = (uint8_t *)malloc(frame->linesize[2] * frame->height / 2);

					memcpy(y, frame->data[0], frame->linesize[0] * frame->height);
					memcpy(u, frame->data[1], frame->linesize[1] * frame->height / 2);
					memcpy(v, frame->data[2], frame->linesize[2] * frame->height / 2);

					one = nemoplay_queue_create_one();
					one->cmd = NEMOPLAY_QUEUE_NORMAL_COMMAND;
					one->pts = video_timebase * av_frame_get_best_effort_timestamp(frame);

					one->y = y;
					one->u = u;
					one->v = v;
					one->width = play->video_width;
					one->height = play->video_height;

					nemoplay_queue_enqueue(play->video_queue, one);
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

					samplesize = swr_convert(swr, &buffer, buffersize, (const uint8_t **)frame->extended_data, frame->nb_samples);

					one = nemoplay_queue_create_one();
					one->cmd = NEMOPLAY_QUEUE_NORMAL_COMMAND;
					one->pts = audio_timebase * av_frame_get_best_effort_timestamp(frame);

					one->data = buffer;
					one->size = samplesize * audio_context->channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);

					nemoplay_queue_enqueue(play->audio_queue, one);
				}

				av_frame_unref(frame);
			} else if (packet.stream_index == subtitle_stream) {
			}

			av_free_packet(&packet);
		} else if (play->state == NEMOPLAY_DONE_STATE) {
			struct playone *one;

			one = nemoplay_queue_create_one();
			one->cmd = NEMOPLAY_QUEUE_DONE_COMMAND;
			nemoplay_queue_enqueue(play->video_queue, one);

			one = nemoplay_queue_create_one();
			one->cmd = NEMOPLAY_QUEUE_DONE_COMMAND;
			nemoplay_queue_enqueue(play->audio_queue, one);

			av_free_packet(&packet);

			break;
		}

		if (nemoplay_queue_get_count(play->video_queue) > play->max_queuesize &&
				nemoplay_queue_get_count(play->audio_queue) > play->max_queuesize) {
			pthread_mutex_lock(&play->lock);
			pthread_cond_wait(&play->signal, &play->lock);
			pthread_mutex_unlock(&play->lock);
		}
	}

	av_frame_free(&frame);

	swr_free(&swr);

	return 0;
}

void nemoplay_wakeup_media(struct nemoplay *play)
{
	pthread_cond_signal(&play->signal);
}
