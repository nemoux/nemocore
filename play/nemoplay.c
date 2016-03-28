#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemoplay.h>

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

	play->container = avformat_alloc_context();
	if (play->container == NULL)
		goto err1;

	play->video_stream = -1;
	play->audio_stream = -1;
	play->subtitle_stream = -1;

	return play;

err1:
	free(play);

	return NULL;
}

void nemoplay_destroy(struct nemoplay *play)
{
	if (play->container != NULL)
		avformat_close_input(&play->container);

	free(play);
}

int nemoplay_open(struct nemoplay *play, const char *filepath)
{
	AVCodecContext *context;
	int i;

	if (avformat_open_input(&play->container, filepath, NULL, NULL) < 0)
		return -1;

	if (avformat_find_stream_info(play->container, NULL) < 0)
		return -1;

	for (i = 0; i < play->container->nb_streams; i++) {
		context = play->container->streams[i]->codec;

		if (context->codec_type == AVMEDIA_TYPE_VIDEO) {
			play->video_stream = i;
		} else if (context->codec_type == AVMEDIA_TYPE_AUDIO) {
			play->audio_stream = i;
		} else if (context->codec_type == AVMEDIA_TYPE_SUBTITLE) {
			play->subtitle_stream = i;
		}
	}

	play->video_stream = av_find_best_stream(play->container, AVMEDIA_TYPE_VIDEO, play->video_stream, -1, NULL, 0);
	play->audio_stream = av_find_best_stream(play->container, AVMEDIA_TYPE_AUDIO, play->audio_stream, play->video_stream, NULL, 0);
	play->subtitle_stream = av_find_best_stream(play->container, AVMEDIA_TYPE_SUBTITLE, play->subtitle_stream, play->audio_stream >= 0 ? play->audio_stream : play->video_stream, NULL, 0);

	return 0;
}
