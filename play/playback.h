#ifndef __NEMOPLAY_BACK_H__
#define __NEMOPLAY_BACK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemotool.h>
#include <nemoplay.h>
#include <nemoshow.h>

struct playback_decoder;
struct playback_audio;
struct playback_video;

typedef void (*nemoplay_back_video_update_t)(struct nemoplay *play, void *data);

extern struct playback_decoder *nemoplay_back_create_decoder(struct nemoplay *play);
extern void nemoplay_back_destroy_decoder(struct playback_decoder *decoder);

extern struct playback_audio *nemoplay_back_create_audio_by_ao(struct nemoplay *play);
extern void nemoplay_back_destroy_audio(struct playback_audio *audio);

extern struct playback_video *nemoplay_back_create_video_by_timer(struct nemoplay *play, struct nemotool *tool);
extern void nemoplay_back_destroy_video(struct playback_video *video);
extern void nemoplay_back_resize_video(struct playback_video *video, int width, int height);
extern void nemoplay_back_redraw_video(struct playback_video *video);
extern void nemoplay_back_set_video_canvas(struct playback_video *video, struct showone *canvas);
extern void nemoplay_back_set_video_update(struct playback_video *video, nemoplay_back_video_update_t dispatch);
extern void nemoplay_back_set_video_data(struct playback_video *video, void *data);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
