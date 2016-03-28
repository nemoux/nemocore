#ifndef	__NEMOPLAY_H__
#define	__NEMOPLAY_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <playqueue.h>

struct nemoplay {
	struct playqueue *video_queue;
	struct playqueue *audio_queue;
	struct playqueue *subtitle_queue;

	int audio_channels;
	int audio_samplerate;
	int audio_samplebits;

	void *userdata;
};

extern struct nemoplay *nemoplay_create(void);
extern void nemoplay_destroy(struct nemoplay *play);

extern int nemoplay_decode_media(struct nemoplay *play, const char *mediapath);

static inline struct playqueue *nemoplay_get_video_queue(struct nemoplay *play)
{
	return play->video_queue;
}

static inline struct playqueue *nemoplay_get_audio_queue(struct nemoplay *play)
{
	return play->audio_queue;
}

static inline struct playqueue *nemoplay_get_subtitle_queue(struct nemoplay *play)
{
	return play->subtitle_queue;
}

static inline int nemoplay_get_audio_channels(struct nemoplay *play)
{
	return play->audio_channels;
}

static inline int nemoplay_get_audio_samplerate(struct nemoplay *play)
{
	return play->audio_samplerate;
}

static inline int nemoplay_get_audio_samplebits(struct nemoplay *play)
{
	return play->audio_samplebits;
}

static inline void nemoplay_set_userdata(struct nemoplay *play, void *userdata)
{
	play->userdata = userdata;
}

static inline void *nemoplay_get_userdata(struct nemoplay *play)
{
	return play->userdata;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
