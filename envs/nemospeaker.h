#ifndef __NEMOSHELL_SPEAKER_H__
#define __NEMOSHELL_SPEAKER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <showhelper.h>

#define NEMOSPEAKER_SIZE			(300)

struct nemospeaker {
	struct nemoshell *shell;

	struct nemosignal destroy_signal;

	struct nemoshow *show;

	struct showone *scene;

	struct showone *back;
	struct showone *canvas;

	struct showone *group;

	struct showone *layout0;
	struct showone *layout1;
	struct showone *layout2;

	struct showone *volume;
	double volumeratio;

	double x, y;
	double r;

	struct soundsink *sink;
	uint32_t pid;

	struct wl_listener sound_info_listener;
};

extern struct nemospeaker *nemospeaker_create(struct nemoshell *shell, uint32_t size, double x, double y, double r);
extern void nemospeaker_destroy(struct nemospeaker *speaker);

extern void nemospeaker_set_sink(struct nemospeaker *speaker, struct soundsink *sink);
extern void nemospeaker_set_volume(struct nemospeaker *speaker, uint32_t volume, uint32_t duration, uint32_t delay);

static inline uint32_t nemospeaker_get_sink_id(struct nemospeaker *speaker)
{
	return speaker->sink == NULL ? 0 : speaker->sink->id;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
