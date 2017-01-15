#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-client.h>
#include <wayland-nemo-sound-client-protocol.h>

#include <nemotool.h>
#include <nemosound.h>

void nemosound_set_sink(struct nemotool *tool, uint32_t pid, uint32_t sink)
{
	nemo_sound_set_sink(tool->sound, pid, sink);
}

void nemosound_set_mute(struct nemotool *tool, uint32_t pid, uint32_t mute)
{
	nemo_sound_set_mute(tool->sound, pid, mute);
}

void nemosound_set_volume(struct nemotool *tool, uint32_t pid, uint32_t volume)
{
	nemo_sound_set_volume(tool->sound, pid, volume);
}

void nemosound_set_mute_sink(struct nemotool *tool, uint32_t sink, uint32_t mute)
{
	nemo_sound_set_mute_sink(tool->sound, sink, mute);
}

void nemosound_set_volume_sink(struct nemotool *tool, uint32_t sink, uint32_t volume)
{
	nemo_sound_set_volume_sink(tool->sound, sink, volume);
}

void nemosound_set_current_sink(struct nemotool *tool, uint32_t sink)
{
	nemo_sound_set_sink(tool->sound, getpid(), sink);
}

void nemosound_set_current_mute(struct nemotool *tool, uint32_t mute)
{
	nemo_sound_set_mute(tool->sound, getpid(), mute);
}

void nemosound_set_current_volume(struct nemotool *tool, uint32_t volume)
{
	nemo_sound_set_volume(tool->sound, getpid(), volume);
}
