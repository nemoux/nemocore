#ifndef	__NEMO_SOUND_H__
#define	__NEMO_SOUND_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct soundsink {
	uint32_t id;

	char *desc;

	struct wl_list link;
};

struct nemosound {
	struct nemocompz *compz;

	struct wl_signal destroy_signal;

	struct wl_list resource_list;

	struct wl_resource *manager;

	struct wl_list sink_list;
};

extern struct nemosound *nemosound_create(struct nemocompz *compz);
extern void nemosound_destroy(struct nemosound *sound);

extern void nemosound_set_sink(struct nemosound *sound, uint32_t pid, uint32_t sink);
extern void nemosound_put_sink(struct nemosound *sound, uint32_t pid);
extern void nemosound_set_mute(struct nemosound *sound, uint32_t pid, uint32_t mute);
extern void nemosound_set_mute_sink(struct nemosound *sound, uint32_t sink, uint32_t mute);
extern void nemosound_set_volume(struct nemosound *sound, uint32_t pid, uint32_t volume);
extern void nemosound_set_volume_sink(struct nemosound *sound, uint32_t sink, uint32_t volume);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
