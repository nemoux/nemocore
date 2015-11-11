#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <nemotool.h>
#include <nemosound.h>
#include <nemoglib.h>
#include <nemomisc.h>

static void nemo_sound_manager_set_sink(void *data, struct nemo_sound_manager *manager, uint32_t pid, uint32_t sink)
{
}

static void nemo_sound_manager_set_mute(void *data, struct nemo_sound_manager *manager, uint32_t pid, uint32_t mute)
{
}

static void nemo_sound_manager_set_mute_sink(void *data, struct nemo_sound_manager *manager, uint32_t sink, uint32_t mute)
{
}

static void nemo_sound_manager_set_volume(void *data, struct nemo_sound_manager *manager, uint32_t pid, uint32_t volume)
{
}

static void nemo_sound_manager_set_volume_sink(void *data, struct nemo_sound_manager *manager, uint32_t sink, uint32_t volume)
{
}

static struct nemo_sound_manager_listener nemo_sound_manager_listener = {
	nemo_sound_manager_set_sink,
	nemo_sound_manager_set_mute,
	nemo_sound_manager_set_mute_sink,
	nemo_sound_manager_set_volume,
	nemo_sound_manager_set_volume_sink
};

static void nemosound_dispatch_global(struct nemotool *tool, uint32_t id, const char *interface, uint32_t version)
{
	struct nemosound *sound = (struct nemosound *)nemotool_get_userdata(tool);

	if (strcmp(interface, "nemo_sound_manager") == 0) {
		sound->manager = wl_registry_bind(tool->registry, id, &nemo_sound_manager_interface, 1);
		nemo_sound_manager_add_listener(sound->manager, &nemo_sound_manager_listener, sound);
	}
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ 0 }
	};

	GMainLoop *gmainloop;
	struct nemosound *sound;
	struct nemotool *tool;
	int opt;

	while (opt = getopt_long(argc, argv, "", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			default:
				break;
		}
	}

	sound = (struct nemosound *)malloc(sizeof(struct nemosound));
	if (sound == NULL)
		return -1;
	memset(sound, 0, sizeof(struct nemosound));

	sound->tool = tool = nemotool_create();
	if (tool == NULL)
		goto out1;
	nemotool_set_dispatch_global(tool, nemosound_dispatch_global);
	nemotool_set_userdata(tool, sound);
	nemotool_connect_wayland(tool, NULL);

	gmainloop = g_main_loop_new(NULL, FALSE);
	nemoglib_run_tool(gmainloop, tool);
	g_main_loop_unref(gmainloop);

	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

out1:
	free(sound);

	return 0;
}
