#ifndef	__NEMOUX_PULSEAUDIO_HELPER_H__
#define	__NEMOUX_PULSEAUDIO_HELPER_H__

#include <glib.h>
#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>

struct nemopulse {
	pa_glib_mainloop *mainloop;
	pa_mainloop_api *mainapi;

	pa_proplist *proplist;
	pa_context *context;

	uint32_t pid;
	uint32_t sink;
	uint32_t sinkinput;
	int has_sinkinput;

	int volume_control;
	double current_volume;
};

extern struct nemopulse *nemopulse_create(GMainContext *context);
extern void nemopulse_destroy(struct nemopulse *pulse);

extern void nemopulse_fetch_sink_input(struct nemopulse *pulse, uint32_t pid);

extern void nemopulse_set_volume(struct nemopulse *pulse, int volume);
extern void nemopulse_set_mute(struct nemopulse *pulse, int mute);
extern void nemopulse_set_sink(struct nemopulse *pulse, int sink);

static inline int nemopulse_has_sink_input(struct nemopulse *pulse)
{
	return pulse->has_sinkinput;
}

#endif
