#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <ffmpegconfig.h>

#include <playmisc.h>
#include <nemomisc.h>

int nemoplay_get_video_info(const char *mediapath, int *width, int *height)
{
	AVFormatContext *container;
	AVCodecContext *context;
	int r = 0;
	int i;

	container = avformat_alloc_context();
	if (container == NULL)
		goto out1;

	if (avformat_open_input(&container, mediapath, NULL, NULL) < 0)
		goto out2;

	if (avformat_find_stream_info(container, NULL) < 0)
		goto out2;

	for (i = 0; i < container->nb_streams; i++) {
		context = container->streams[i]->codec;

		if (context->codec_type == AVMEDIA_TYPE_VIDEO) {
			*width = context->width;
			*height = context->height;

			r = 1;

			break;
		}
	}

out2:
	avformat_close_input(&container);

out1:
	return r;
}

int nemoplay_get_audio_info(const char *mediapath, int *channels, int *samplerate, int *samplebits)
{
	AVFormatContext *container;
	AVCodecContext *context;
	int r = 0;
	int i;

	container = avformat_alloc_context();
	if (container == NULL)
		goto out1;

	if (avformat_open_input(&container, mediapath, NULL, NULL) < 0)
		goto out2;

	if (avformat_find_stream_info(container, NULL) < 0)
		goto out2;

	for (i = 0; i < container->nb_streams; i++) {
		context = container->streams[i]->codec;

		if (context->codec_type == AVMEDIA_TYPE_AUDIO) {
			*channels = context->channels;
			*samplerate = context->sample_rate;

			if (context->sample_fmt == AV_SAMPLE_FMT_U8 || context->sample_fmt == AV_SAMPLE_FMT_U8P)
				*samplebits = 8;
			else if (context->sample_fmt == AV_SAMPLE_FMT_S16 || context->sample_fmt == AV_SAMPLE_FMT_S16P)
				*samplebits = 16;
			else if (context->sample_fmt == AV_SAMPLE_FMT_S32 || context->sample_fmt == AV_SAMPLE_FMT_S32P)
				*samplebits = 16;
			else if (context->sample_fmt == AV_SAMPLE_FMT_FLT || context->sample_fmt == AV_SAMPLE_FMT_FLTP)
				*samplebits = 16;
			else if (context->sample_fmt == AV_SAMPLE_FMT_DBL || context->sample_fmt == AV_SAMPLE_FMT_DBLP)
				*samplebits = 16;
			else
				*samplebits = 0;

			r = 1;

			break;
		}
	}

out2:
	avformat_close_input(&container);

out1:
	return r;
}
