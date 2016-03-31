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

void nemoplay_dump_media(const char *mediapath)
{
	AVFormatContext *container;
	int i;

	container = avformat_alloc_context();
	if (container == NULL)
		return;

	if (avformat_open_input(&container, mediapath, NULL, NULL) < 0)
		goto out1;

	if (avformat_find_stream_info(container, NULL) < 0)
		goto out1;

	for (i = 0; i < container->nb_streams; i++) {
		av_dump_format(container, i, mediapath, 0);
	}

out1:
	avformat_close_input(&container);
}

int nemoplay_convert_yuv_to_rgba(uint8_t *y, uint8_t *u, uint8_t *v, uint8_t *c, int width, int height)
{
	int i, j;

	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			float _y = y[i * width + j] / 255.0f;
			float _u = u[(i / 2) * (width / 2) + (j / 2)] / 255.0f - 0.5f;
			float _v = v[(i / 2) * (width / 2) + (j / 2)] / 255.0f - 0.5f;
			float r = _y + 1.402f * _v;
			float g = _y - 0.344f * _u - 0.714f * _v;
			float b = _y + 1.772f * _u;

			r = MIN(MAX(r, 0.0f), 1.0f);
			g = MIN(MAX(g, 0.0f), 1.0f);
			b = MIN(MAX(b, 0.0f), 1.0f);

			c[(i * width + j) * 4 + 0] = b * 255;
			c[(i * width + j) * 4 + 1] = g * 255;
			c[(i * width + j) * 4 + 2] = r * 255;
			c[(i * width + j) * 4 + 3] = 255;
		}
	}

	return 0;
}
