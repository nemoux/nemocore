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
#include <playbox.h>
#include <playmisc.h>

typedef enum {
	NEMOPLAY_STOP_FLAG = (1 << 0),
	NEMOPLAY_EOF_FLAG = (1 << 1),
	NEMOPLAY_DONE_FLAG = (1 << 2),
	NEMOPLAY_SEEK_FLAG = (1 << 8)
} NemoPlayFlag;

typedef enum {
	NEMOPLAY_YUV420_PIXEL_FORMAT = 0x0,
	NEMOPLAY_BGRA_PIXEL_FORMAT = 0x100,
	NEMOPLAY_RGBA_PIXEL_FORMAT = 0x101,
	NEMOPLAY_LAST_PIXEL_FORMAT
} NemoPlayPixelFormat;

#define NEMOPLAY_PIXEL_IS_YUV420_FORMAT(fmt)			(((fmt) & 0xff00) == 0x0)
#define NEMOPLAY_PIXEL_IS_RGBA_FORMAT(fmt)				(((fmt) & 0xff00) == 0x100)

struct nemoplay {
	char *path;

	uint32_t flags;

	uint64_t frame;

	pthread_mutex_t lock;
	pthread_cond_t signal;

	int threadcount;

	struct playqueue *video_queue;
	struct playqueue *audio_queue;

	AVFormatContext *container;

	double duration;

	AVCodecContext *video_context;
	AVCodecContext *audio_context;
	AVDictionary *video_opts;
	AVDictionary *audio_opts;
	int video_stream;
	int audio_stream;

	double video_timebase;
	double audio_timebase;

	SwrContext *swr;

	struct playclock *clock;

	int pixel_format;

	int video_width;
	int video_height;
	double video_framerate;
	int video_framecount;

	int audio_channels;
	int audio_samplerate;
	int audio_samplebits;

	void *userdata;
};

struct playone {
	struct nemolist link;

	int cmd;

	double pts;

	void *data[4];
	int linesize[4];
	int height;

	uint32_t serial;
};

extern struct nemoplay *nemoplay_create(void);
extern void nemoplay_destroy(struct nemoplay *play);

extern void nemoplay_set_video_stropt(struct nemoplay *play, const char *key, const char *value);
extern void nemoplay_set_video_intopt(struct nemoplay *play, const char *key, int64_t value);
extern void nemoplay_set_audio_stropt(struct nemoplay *play, const char *key, const char *value);
extern void nemoplay_set_audio_intopt(struct nemoplay *play, const char *key, int64_t value);

extern void nemoplay_set_flags(struct nemoplay *play, uint32_t flags);
extern void nemoplay_put_flags(struct nemoplay *play, uint32_t flags);
extern void nemoplay_put_flags_all(struct nemoplay *play);
extern int nemoplay_has_flags(struct nemoplay *play, uint32_t flags);
extern int nemoplay_has_flags_all(struct nemoplay *play, uint32_t flags);
extern int nemoplay_has_no_flags(struct nemoplay *play);

extern void nemoplay_reset_media(struct nemoplay *play);

extern int nemoplay_load_media(struct nemoplay *play, const char *mediapath);
extern void nemoplay_unload_media(struct nemoplay *play);

extern int nemoplay_seek_media(struct nemoplay *play, double pts);

extern void nemoplay_wait_media(struct nemoplay *play);
extern void nemoplay_wake_media(struct nemoplay *play);

extern int nemoplay_decode_media(struct nemoplay *play, int maxcount);
extern int nemoplay_extract_video(struct nemoplay *play, struct playbox *box, int maxcount);

extern void nemoplay_revoke_video(struct nemoplay *play);
extern void nemoplay_revoke_audio(struct nemoplay *play);

extern void nemoplay_enter_thread(struct nemoplay *play);
extern void nemoplay_leave_thread(struct nemoplay *play);
extern void nemoplay_wait_thread(struct nemoplay *play);

extern void nemoplay_set_video_pts(struct nemoplay *play, double pts);
extern void nemoplay_set_audio_pts(struct nemoplay *play, double pts);
extern void nemoplay_set_clock_cts(struct nemoplay *play, double cts);
extern double nemoplay_get_clock_cts(struct nemoplay *play);

extern void nemoplay_set_clock_state(struct nemoplay *play, int state);
extern void nemoplay_set_clock_speed(struct nemoplay *play, double speed);

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

static inline int nemoplay_get_pixel_format(struct nemoplay *play)
{
	return play->pixel_format;
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

static inline int nemoplay_get_video_framecount(struct nemoplay *play)
{
	return play->video_framecount;
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

static inline double nemoplay_get_duration(struct nemoplay *play)
{
	return play->duration;
}

static inline const char *nemoplay_get_path(struct nemoplay *play)
{
	return play->path;
}

static inline void nemoplay_set_userdata(struct nemoplay *play, void *userdata)
{
	play->userdata = userdata;
}

static inline void *nemoplay_get_userdata(struct nemoplay *play)
{
	return play->userdata;
}

extern struct playone *nemoplay_one_create(void);
extern void nemoplay_one_destroy(struct playone *one);

static inline int nemoplay_one_get_cmd(struct playone *one)
{
	return one->cmd;
}

static inline double nemoplay_one_get_pts(struct playone *one)
{
	return one->pts;
}

static inline void *nemoplay_one_get_data(struct playone *one, int index)
{
	return one->data[index];
}

static inline uint32_t nemoplay_one_get_linesize(struct playone *one, int index)
{
	return one->linesize[index];
}

static inline int nemoplay_one_get_height(struct playone *one)
{
	return one->height;
}

static inline uint32_t nemoplay_one_get_serial(struct playone *one)
{
	return one->serial;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
