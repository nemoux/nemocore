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

typedef enum {
	NEMOPLAY_NONE_STATE = 0,
	NEMOPLAY_PLAY_STATE = 1,
	NEMOPLAY_STOP_STATE = 2,
	NEMOPLAY_WAKE_STATE = 3,
	NEMOPLAY_WAIT_STATE = 4,
	NEMOPLAY_DONE_STATE = 5,
	NEMOPLAY_LAST_STATE
} NemoPlayState;

typedef enum {
	NEMOPLAY_SEEK_CMD = (1 << 0)
} NemoPlayCmd;

struct nemoplay {
	int state;
	uint32_t cmd;

	uint64_t frame;

	pthread_mutex_t lock;
	pthread_cond_t signal;

	int threadcount;

	struct playqueue *video_queue;
	struct playqueue *audio_queue;
	struct playqueue *subtitle_queue;

	AVFormatContext *container;

	int32_t duration;

	AVCodecContext *video_context;
	AVCodecContext *audio_context;
	AVCodecContext *subtitle_context;
	int video_stream;
	int audio_stream;
	int subtitle_stream;

	double video_timebase;
	double audio_timebase;

	SwrContext *swr;

	struct playclock *clock;

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
extern int nemoplay_decode_media(struct nemoplay *play, int reqcount, int maxcount);
extern int nemoplay_seek_media(struct nemoplay *play, double pts);
extern void nemoplay_wait_media(struct nemoplay *play);

extern void nemoplay_revoke_video(struct nemoplay *play);
extern void nemoplay_revoke_audio(struct nemoplay *play);

extern void nemoplay_set_state(struct nemoplay *play, int state);

extern void nemoplay_enter_thread(struct nemoplay *play);
extern void nemoplay_leave_thread(struct nemoplay *play);
extern void nemoplay_wait_thread(struct nemoplay *play);

extern void nemoplay_set_video_pts(struct nemoplay *play, double pts);
extern void nemoplay_set_audio_pts(struct nemoplay *play, double pts);
extern double nemoplay_get_cts(struct nemoplay *play);

extern void nemoplay_set_speed(struct nemoplay *play, double speed);

extern void nemoplay_set_cmd(struct nemoplay *play, uint32_t cmd);
extern void nemoplay_put_cmd(struct nemoplay *play, uint32_t cmd);
extern int nemoplay_has_cmd(struct nemoplay *play, uint32_t cmd);

static inline int nemoplay_get_state(struct nemoplay *play)
{
	return play->state;
}

static inline uint64_t nemoplay_get_frame(struct nemoplay *play)
{
	return play->frame;
}

static inline uint64_t nemoplay_next_frame(struct nemoplay *play)
{
	return ++play->frame;
}

static inline void nemoplay_clear_frame(struct nemoplay *play)
{
	play->frame = 0;
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

static inline int nemoplay_get_video_width(struct nemoplay *play)
{
	return play->video_width;
}

static inline int nemoplay_get_video_height(struct nemoplay *play)
{
	return play->video_height;
}

static inline double nemoplay_get_video_aspectratio(struct nemoplay *play)
{
	return (double)play->video_width / (double)play->video_height;
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

static inline int64_t nemoplay_get_duration(struct nemoplay *play)
{
	return play->duration;
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
