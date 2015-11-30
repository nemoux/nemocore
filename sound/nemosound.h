#ifndef	__NEMOSOUND_H__
#define	__NEMOSOUND_H__

#include <nemotool.h>

#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>

#include <nemotimer.h>

#include <nemolist.h>
#include <nemolistener.h>

#define	NEMOSOUND_NULL_SINK_NAME		("nullsink")

#define	NEMOSOUND_SINKINPUT_TIMEOUT	(500)

typedef enum {
	NEMOSOUND_NONE_COMMAND = 0,
	NEMOSOUND_SET_SINK_COMMAND = 1,
	NEMOSOUND_PUT_SINK_COMMAND = 2,
	NEMOSOUND_SET_MUTE_COMMAND = 3,
	NEMOSOUND_SET_MUTE_SINK_COMMAND = 4,
	NEMOSOUND_SET_VOLUME_COMMAND = 5,
	NEMOSOUND_SET_VOLUME_SINK_COMMAND = 6,
	NEMOSOUND_GET_INFO_COMMAND = 7,
	NEMOSOUND_GET_INFO_SINK_COMMAND = 8,
	NEMOSOUND_LAST_COMMAND
} NemoSoundCommand;

struct soundcmd {
	int type;
	struct nemosound *sound;
	struct soundone *one;

	uint32_t sink;
	uint32_t mute;
	uint32_t volume;

	struct nemolist link;
};

struct soundone {
	struct nemosound *sound;

	uint32_t pid;
	uint32_t ppid;

	struct nemolist link;

	struct nemolist cmd_list;

	uint32_t sinkinput;
	int has_sinkinput;

	uint32_t sink;
	uint32_t next_sink;
	int has_sink;

	uint32_t next_volume;
};

struct soundsink {
	struct nemosound *sound;

	uint32_t id;

	struct nemolist link;
};

struct nemosound {
	struct nemotool *tool;

	struct nemotimer *timer;

	struct nemolist list;

	struct nemolist sink_list;

	struct nemo_sound_manager *manager;

	pa_glib_mainloop *mainloop;
	pa_mainloop_api *mainapi;

	pa_proplist *proplist;
	pa_context *context;
};

extern struct soundone *nemosound_create_one(struct nemosound *sound, uint32_t pid);
extern void nemosound_destroy_one(struct soundone *one);
extern struct soundone *nemosound_get_one(struct nemosound *sound, uint32_t pid);

extern void nemosound_flush_command(struct nemosound *sound, struct soundone *one);

extern struct soundsink *nemosound_create_sink(struct nemosound *sound, uint32_t id);
extern void nemosound_destroy_sink(struct soundsink *sink);
extern struct soundsink *nemosound_get_sink(struct nemosound *sound, uint32_t id);

extern struct soundcmd *nemosound_create_command(struct nemosound *sound, struct soundone *one, int type);
extern void nemosound_destroy_command(struct soundcmd *cmd);

#endif
