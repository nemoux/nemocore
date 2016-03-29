#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <ffmpegconfig.h>

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

	play->video_queue = nemoplay_queue_create();
	play->audio_queue = nemoplay_queue_create();
	play->subtitle_queue = nemoplay_queue_create();

	return play;

err1:
	free(play);

	return NULL;
}

void nemoplay_destroy(struct nemoplay *play)
{
	nemoplay_queue_destroy(play->video_queue);
	nemoplay_queue_destroy(play->audio_queue);
	nemoplay_queue_destroy(play->subtitle_queue);

	free(play);
}

int nemoplay_decode_media(struct nemoplay *play, const char *mediapath)
{
	AVFormatContext *container;
	AVCodecContext *context;
	AVCodec *codec;
	AVFrame *frame;
	AVPacket packet;
	AVCodecContext *video_context = NULL;
	AVCodecContext *audio_context = NULL;
	AVCodecContext *subtitle_context = NULL;
	int video_stream = -1;
	int audio_stream = -1;
	int subtitle_stream = -1;
	int finished = 0;
	int i;

	container = avformat_alloc_context();
	if (container == NULL)
		return -1;

	if (avformat_open_input(&container, mediapath, NULL, NULL) < 0)
		goto out1;

	if (avformat_find_stream_info(container, NULL) < 0)
		goto out1;

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

	play->video_width = video_context->width;
	play->video_height = video_context->height;

	play->audio_channels = audio_context->channels;
	play->audio_samplerate = audio_context->sample_rate;

	if (audio_context->sample_fmt == AV_SAMPLE_FMT_U8 || audio_context->sample_fmt == AV_SAMPLE_FMT_U8P)
		play->audio_samplebits = 8;
	else if (audio_context->sample_fmt == AV_SAMPLE_FMT_S16 || audio_context->sample_fmt == AV_SAMPLE_FMT_S16P)
		play->audio_samplebits = 16;
	else if (audio_context->sample_fmt == AV_SAMPLE_FMT_S32 || audio_context->sample_fmt == AV_SAMPLE_FMT_S32P)
		play->audio_samplebits = 16;
	else if (audio_context->sample_fmt == AV_SAMPLE_FMT_FLT || audio_context->sample_fmt == AV_SAMPLE_FMT_FLTP)
		play->audio_samplebits = 16;
	else if (audio_context->sample_fmt == AV_SAMPLE_FMT_DBL || audio_context->sample_fmt == AV_SAMPLE_FMT_DBLP)
		play->audio_samplebits = 16;
	else
		play->audio_samplebits = 0;

	frame = av_frame_alloc();

	while (av_read_frame(container, &packet) >= 0) {
		if (packet.stream_index == video_stream) {
			avcodec_decode_video2(video_context, frame, &finished, &packet);

			if (finished != 0) {
				struct playone *one;
				uint8_t *buffer;
				int nbytes;

				nbytes = (play->video_width * play->video_height) + ((play->video_width * play->video_height) / 4 * 2);
				buffer = (uint8_t *)malloc(nbytes);

				memcpy(buffer, frame->data, nbytes);

				one = nemoplay_queue_create_one();
				one->cmd = NEMOPLAY_QUEUE_NORMAL_COMMAND;
				one->data = buffer;
				one->size = nbytes;

				one->width = play->video_width;
				one->height = play->video_height;
				one->stride = frame->linesize[0];

				nemoplay_queue_enqueue(play->video_queue, one);
			}
		} else if (packet.stream_index == audio_stream) {
			int planesize;
			int len = avcodec_decode_audio4(audio_context, frame, &finished, &packet);
			int size = av_samples_get_buffer_size(&planesize, audio_context->channels, frame->nb_samples, audio_context->sample_fmt, 1);

			if (finished != 0) {
				struct playone *one;
				uint16_t *buffer;
				uint32_t size = 0;
				int index = 0;
				int nb, ch;

				buffer = (uint16_t *)malloc(planesize * audio_context->channels);

				switch (audio_context->sample_fmt) {
					case AV_SAMPLE_FMT_S16P:
						for (nb = 0; nb < planesize / sizeof(uint16_t); nb++) {
							for (ch = 0; ch < audio_context->channels; ch++) {
								buffer[index++] = ((uint16_t *)frame->extended_data[ch])[nb];
							}
						}

						size = planesize * audio_context->channels;
						break;

					case AV_SAMPLE_FMT_FLTP:
						for (nb = 0; nb < planesize / sizeof(float); nb++) {
							for (ch = 0; ch < audio_context->channels; ch++) {
								buffer[index++] = ((float *)frame->extended_data[ch])[nb] * UINT_MAX;
							}
						}

						size = planesize * audio_context->channels;
						break;

					case AV_SAMPLE_FMT_S16:
						for (nb = 0; nb < planesize / sizeof(uint16_t); nb++) {
							buffer[nb] = (uint16_t)(((uint16_t *)frame->extended_data[0])[nb]);
						}

						size = frame->linesize[0];
						break;

					case AV_SAMPLE_FMT_FLT:
						for (nb = 0; nb < planesize / sizeof(float); nb++) {
							buffer[nb] = (uint16_t)(((float *)frame->extended_data[0])[nb] * UINT_MAX);
						}

						size = planesize / sizeof(float) * sizeof(uint16_t);
						break;

					case AV_SAMPLE_FMT_U8P:
						for (nb = 0; nb < planesize / sizeof(uint8_t); nb++) {
							for (ch = 0; ch < audio_context->channels; ch++) {
								buffer[index++] = (((uint8_t *)frame->extended_data[ch])[nb] - 127) * UINT_MAX / 127;
							}
						}

						size = planesize / sizeof(uint8_t) * sizeof(uint16_t);
						break;

					case AV_SAMPLE_FMT_U8:
						for (nb = 0; nb < planesize / sizeof(uint8_t); nb++) {
							for (ch = 0; ch < audio_context->channels; ch++) {
								buffer[index++] = (((uint8_t *)frame->extended_data[0])[nb] - 127) * UINT_MAX / 127;
							}
						}

						size = planesize / sizeof(uint8_t) * sizeof(uint16_t);
						break;

					default:
						fprintf(stderr, "PCM type not supported\n");
						break;
				}

				one = nemoplay_queue_create_one();
				one->cmd = NEMOPLAY_QUEUE_NORMAL_COMMAND;
				one->data = buffer;
				one->size = size;

				nemoplay_queue_enqueue(play->audio_queue, one);
			}
		} else if (packet.stream_index == subtitle_stream) {
		}

		av_free_packet(&packet);
	}

	av_frame_free(&frame);

out1:
	avformat_close_input(&container);

	return 0;
}
