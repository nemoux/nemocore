#ifndef	__NEMOPLAY_H__
#define	__NEMOPLAY_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <ffmpegconfig.h>

#include <playqueue.h>

struct nemoplay {
	AVFormatContext *container;

	int video_stream;
	int audio_stream;
	int subtitle_stream;

	void *userdata;
};

extern struct nemoplay *nemoplay_create(void);
extern void nemoplay_destroy(struct nemoplay *play);

extern int nemoplay_open(struct nemoplay *play, const char *filepath);

static inline void nemoplay_set_userdata(struct nemoplay *play, void *userdata)
{
	play->userdata = userdata;
}

static inline void *nemoplay_get_userdata(struct nemoplay *play)
{
	return play->userdata;
}

static inline int nemoplay_get_video_stream(struct nemoplay *play)
{
	return play->video_stream;
}

static inline int nemoplay_get_audio_stream(struct nemoplay *play)
{
	return play->audio_stream;
}

static inline int nemoplay_get_subtitle_stream(struct nemoplay *play)
{
	return play->subtitle_stream;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
