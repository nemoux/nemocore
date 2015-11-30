#ifndef	__NEMO_SOUND_H__
#define	__NEMO_SOUND_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct nemosound;

struct soundsink {
	uint32_t id;

	char *name;
	char *desc;

	struct wl_list link;
};

struct soundinfo {
	uint32_t id;

	int is_sink;

	uint32_t volume;
	uint32_t mute;
};

struct nemosound {
	struct nemocompz *compz;

	struct wl_signal destroy_signal;

	struct wl_signal info_signal;

	struct wl_list resource_list;

	struct wl_resource *manager;

	struct wl_list sink_list;

	void *userdata;
};

extern struct nemosound *nemosound_create(struct nemocompz *compz);
extern void nemosound_destroy(struct nemosound *sound);

extern struct soundsink *nemosound_get_main_sink(struct nemosound *sound);
extern struct soundsink *nemosound_get_sink_by_name(struct nemosound *sound, const char *name);

extern void nemosound_set_sink(struct nemosound *sound, uint32_t pid, uint32_t sink);
extern void nemosound_put_sink(struct nemosound *sound, uint32_t pid);
extern void nemosound_set_mute(struct nemosound *sound, uint32_t pid, uint32_t mute);
extern void nemosound_set_mute_sink(struct nemosound *sound, uint32_t sink, uint32_t mute);
extern void nemosound_set_volume(struct nemosound *sound, uint32_t pid, uint32_t volume);
extern void nemosound_set_volume_sink(struct nemosound *sound, uint32_t sink, uint32_t volume);
extern void nemosound_get_info(struct nemosound *sound, uint32_t pid);
extern void nemosound_get_info_sink(struct nemosound *sound, uint32_t sink);

static inline void nemosound_set_userdata(struct nemosound *sound, void *data)
{
	sound->userdata = data;
}

static inline void *nemosound_get_userdata(struct nemosound *sound)
{
	return sound->userdata;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
