#ifndef	__NEMOPLAY_H__
#define	__NEMOPLAY_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <pthread.h>

#include <ffmpegconfig.h>

#include <playqueue.h>
#include <playclock.h>
#include <playshader.h>
#include <playmisc.h>

#define NEMOPLAY_MAX_QUEUESIZE		(128)

typedef enum {
	NEMOPLAY_NONE_STATE = 0,
	NEMOPLAY_PLAY_STATE = 1,
	NEMOPLAY_FULL_STATE = 2,
	NEMOPLAY_STOP_STATE = 3,
	NEMOPLAY_IDLE_STATE = 4,
	NEMOPLAY_DONE_STATE = 5,
	NEMOPLAY_LAST_STATE
} NemoPlayState;

struct nemoplay {
	int state;

	pthread_mutex_t lock;
	pthread_cond_t signal;

	int max_queuesize;

	struct playqueue *video_queue;
	struct playqueue *audio_queue;
	struct playqueue *subtitle_queue;

	AVFormatContext *container;

	AVCodecContext *video_context;
	AVCodecContext *audio_context;
	AVCodecContext *subtitle_context;
	int video_stream;
	int audio_stream;
	int subtitle_stream;

	struct playclock *video_clock;
	struct playclock *audio_clock;

	int video_width;
	int video_height;
	double video_framerate;

	int audio_channels;
	int audio_samplerate;
	int audio_samplebits;

	void *userdata;
};

extern struct nemoplay *nemoplay_create(void);
extern void nemoplay_destroy(struct nemoplay *play);

extern int nemoplay_prepare_media(struct nemoplay *play, const char *mediapath);
extern void nemoplay_finish_media(struct nemoplay *play);
extern int nemoplay_decode_media(struct nemoplay *play);

extern void nemoplay_set_state(struct nemoplay *play, int state);

static inline int nemoplay_get_state(struct nemoplay *play)
{
	return play->state;
}

static inline void nemoplay_set_max_queuesize(struct nemoplay *play, int queuesize)
{
	play->max_queuesize = queuesize;
}

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

static inline struct playclock *nemoplay_get_video_clock(struct nemoplay *play)
{
	return play->video_clock;
}

static inline struct playclock *nemoplay_get_audio_clock(struct nemoplay *play)
{
	return play->audio_clock;
}

static inline int nemoplay_get_video_width(struct nemoplay *play)
{
	return play->video_width;
}

static inline int nemoplay_get_video_height(struct nemoplay *play)
{
	return play->video_height;
}

static inline double nemoplay_get_video_framerate(struct nemoplay *play)
{
	return play->video_framerate;
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

static inline int nemoplay_has_video(struct nemoplay *play)
{
	return play->video_context != NULL;
}

static inline int nemoplay_has_audio(struct nemoplay *play)
{
	return play->audio_context != NULL;
}

static inline int nemoplay_has_subtitle(struct nemoplay *play)
{
	return play->subtitle_context != NULL;
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
