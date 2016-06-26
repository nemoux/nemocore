#ifndef	__NEMOTOOL_SOUND_H__
#define	__NEMOTOOL_SOUND_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemotool.h>

extern void nemosound_set_sink(struct nemotool *tool, uint32_t pid, uint32_t sink);
extern void nemosound_set_mute(struct nemotool *tool, uint32_t pid, uint32_t mute);
extern void nemosound_set_volume(struct nemotool *tool, uint32_t pid, uint32_t volume);

extern void nemosound_set_mute_sink(struct nemotool *tool, uint32_t sink, uint32_t mute);
extern void nemosound_set_volume_sink(struct nemotool *tool, uint32_t sink, uint32_t volume);

extern void nemosound_set_current_sink(struct nemotool *tool, uint32_t sink);
extern void nemosound_set_current_mute(struct nemotool *tool, uint32_t mute);
extern void nemosound_set_current_volume(struct nemotool *tool, uint32_t volume);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
