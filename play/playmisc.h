#ifndef __NEMOPLAY_MISC_H__
#define __NEMOPLAY_MISC_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

extern int nemoplay_get_video_info(const char *mediapath, int *width, int *height);
extern int nemoplay_get_audio_info(const char *mediapath, int *channels, int *samplerate, int *samplebits);

extern void nemoplay_dump_media(const char *mediapath);

extern int nemoplay_convert_yuv_to_rgba(uint8_t *y, uint8_t *u, uint8_t *v, uint8_t *c, int width, int height);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
