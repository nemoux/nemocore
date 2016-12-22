#ifndef __NEMOPLAY_BACK_H__
#define __NEMOPLAY_BACK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemotool.h>
#include <nemoplay.h>

struct playdecoder;
struct playaudio;
struct playvideo;
struct playextractor;

typedef void (*nemoplay_frame_update_t)(struct nemoplay *play, void *data);
typedef void (*nemoplay_frame_done_t)(struct nemoplay *play, void *data);

extern struct playdecoder *nemoplay_decoder_create(struct nemoplay *play);
extern void nemoplay_decoder_destroy(struct playdecoder *decoder);
extern void nemoplay_decoder_play(struct playdecoder *decoder);
extern void nemoplay_decoder_stop(struct playdecoder *decoder);
extern void nemoplay_decoder_seek(struct playdecoder *decoder, double pts);

extern struct playaudio *nemoplay_audio_create_by_ao(struct nemoplay *play);
extern void nemoplay_audio_destroy(struct playaudio *audio);

extern struct playvideo *nemoplay_video_create_by_timer(struct nemoplay *play, struct nemotool *tool);
extern void nemoplay_video_destroy(struct playvideo *video);
extern void nemoplay_video_redraw(struct playvideo *video);
extern struct playshader *nemoplay_video_get_shader(struct playvideo *video);
extern void nemoplay_video_set_texture(struct playvideo *video, uint32_t texture, int width, int height);
extern void nemoplay_video_set_threshold(struct playvideo *video, double threshold);
extern void nemoplay_video_set_update(struct playvideo *video, nemoplay_frame_update_t dispatch);
extern void nemoplay_video_set_done(struct playvideo *video, nemoplay_frame_done_t dispatch);
extern void nemoplay_video_set_data(struct playvideo *video, void *data);

extern struct playextractor *nemoplay_extractor_create(struct nemoplay *play, struct playbox *box);
extern void nemoplay_extractor_destroy(struct playextractor *extractor);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
