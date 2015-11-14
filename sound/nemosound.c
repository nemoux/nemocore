#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <wayland-client.h>
#include <wayland-nemo-sound-client-protocol.h>

#include <nemotool.h>
#include <nemosound.h>
#include <nemoglib.h>
#include <nemolog.h>
#include <nemomisc.h>

static void nemosound_dispatch_simple_callback(pa_context *context, int success, void *userdata)
{
}

static void nemosound_dispatch_client_info_callback(pa_context *context, const pa_client_info *info, int is_last, void *userdata)
{
	struct nemosound *sound = (struct nemosound *)userdata;

	if (is_last < 0) {
		nemolog_error("SOUND", "failed to get client information: %s\n", pa_strerror(pa_context_errno(context)));
		return;
	}

	if (is_last != 0)
		return;

	if (info == NULL)
		return;

	nemolog_message("SOUND", "CLIENT #%u: name(%s), pid(%s)\n",
			info->index,
			pa_proplist_gets(info->proplist, PA_PROP_APPLICATION_NAME),
			pa_proplist_gets(info->proplist, PA_PROP_APPLICATION_PROCESS_ID));
}

static void nemosound_dispatch_sink_info_callback(pa_context *context, const pa_sink_info *info, int is_last, void *userdata)
{
	struct nemosound *sound = (struct nemosound *)userdata;

	if (is_last < 0) {
		nemolog_error("SOUND", "failed to get sink information: %s\n", pa_strerror(pa_context_errno(context)));
		return;
	}

	if (is_last != 0)
		return;

	nemolog_message("SOUND", "SINK #%u: description(%s)\n",
			info->index,
			pa_proplist_gets(info->proplist, PA_PROP_DEVICE_DESCRIPTION));

	nemo_sound_manager_register_sink(sound->manager, info->index, pa_proplist_gets(info->proplist, PA_PROP_DEVICE_DESCRIPTION));
}

static void nemosound_dispatch_sink_input_info_callback(pa_context *context, const pa_sink_input_info *info, int is_last, void *userdata)
{
	struct nemosound *sound = (struct nemosound *)userdata;
	struct soundone *one;
	const char *spid;
	uint32_t pid;

	if (is_last < 0) {
		nemolog_error("SOUND", "failed to get sink input information: %s\n", pa_strerror(pa_context_errno(context)));
		return;
	}

	if (is_last != 0)
		return;

	nemolog_message("SOUND", "SINK-INPUT #%u: name(%s), pid(%s)\n",
			info->index,
			pa_proplist_gets(info->proplist, PA_PROP_APPLICATION_NAME),
			pa_proplist_gets(info->proplist, PA_PROP_APPLICATION_PROCESS_ID));

	spid = pa_proplist_gets(info->proplist, PA_PROP_APPLICATION_PROCESS_ID);
	pid = strtoul(spid, NULL, 10);

	nemolist_for_each(one, &sound->list, link) {
		if (one->pid == pid) {
			one->sinkinput = info->index;
			one->has_sinkinput = 1;

			nemosound_flush_command(one->sound, one);

			break;
		}
	}
}

static void nemosound_dispatch_move_sink_callback(pa_context *context, int success, void *userdata)
{
	struct soundone *one = (struct soundone *)userdata;

	one->sink = one->next_sink;
	one->has_sink = 1;

	nemolog_message("SOUND", "MOVE-SINK #%u: success(%d)\n", one->sinkinput, success);
}

static void nemosound_dispatch_set_sink_input_mute_callback(pa_context *context, int success, void *userdata)
{
	struct soundone *one = (struct soundone *)userdata;

	nemolog_message("SOUND", "SET-MUTE #%u: success(%d)\n", one->sinkinput, success);
}

static void nemosound_dispatch_set_sink_input_volume_callback(pa_context *context, int success, void *userdata)
{
	struct soundone *one = (struct soundone *)userdata;

	nemolog_message("SOUND", "SET-VOLUME #%u: success(%d)\n", one->sinkinput, success);
}

static void nemosound_dispatch_sink_input_volume_callback(pa_context *context, const pa_sink_input_info *info, int is_last, void *userdata)
{
	struct soundone *one = (struct soundone *)userdata;
	pa_operation *op;
	pa_cvolume cv;

	if (is_last < 0) {
		nemolog_error("SOUND", "failed to get sink input volume: %s\n", pa_strerror(pa_context_errno(context)));
		return;
	}

	if (is_last != 0)
		return;

	pa_cvolume_set(&cv, info->channel_map.channels, (double)PA_VOLUME_NORM * (double)one->next_volume / 100.0f);

	op = pa_context_set_sink_input_volume(context, one->sinkinput, &cv, nemosound_dispatch_set_sink_input_volume_callback, one);
	if (op != NULL)
		pa_operation_unref(op);
}

static void nemosound_dispatch_state_callback(pa_context *context, void *userdata)
{
	struct nemosound *sound = (struct nemosound *)userdata;
	pa_operation *op;

	switch (pa_context_get_state(context)) {
		case PA_CONTEXT_CONNECTING:
		case PA_CONTEXT_AUTHORIZING:
		case PA_CONTEXT_SETTING_NAME:
			break;

		case PA_CONTEXT_READY:
			op = pa_context_get_client_info_list(context, nemosound_dispatch_client_info_callback, sound);
			if (op != NULL)
				pa_operation_unref(op);
			op = pa_context_get_sink_info_list(context, nemosound_dispatch_sink_info_callback, sound);
			if (op != NULL)
				pa_operation_unref(op);
			break;

		case PA_CONTEXT_TERMINATED:
			sound->mainapi->quit(sound->mainapi, 0);
			break;

		case PA_CONTEXT_FAILED:
		default:
			break;
	}
}

static void nemo_sound_manager_set_sink(void *data, struct nemo_sound_manager *manager, uint32_t pid, uint32_t sink)
{
	struct nemosound *sound = (struct nemosound *)data;
	struct soundone *one;
	struct soundcmd *cmd;

	one = nemosound_get_one(sound, pid);
	if (one == NULL)
		one = nemosound_create_one(sound, pid);

	cmd = nemosound_create_command(one, NEMOSOUND_SET_SINK_COMMAND);
	if (cmd == NULL)
		return;
	cmd->sink = sink;

	if (one->has_sinkinput == 0) {
		pa_operation *op;

		op = pa_context_get_sink_input_info_list(sound->context, nemosound_dispatch_sink_input_info_callback, sound);
		if (op != NULL)
			pa_operation_unref(op);

		return;
	}

	nemosound_flush_command(sound, one);
}

static void nemo_sound_manager_set_mute(void *data, struct nemo_sound_manager *manager, uint32_t pid, uint32_t mute)
{
	struct nemosound *sound = (struct nemosound *)data;
	struct soundone *one;
	struct soundcmd *cmd;

	one = nemosound_get_one(sound, pid);
	if (one == NULL)
		one = nemosound_create_one(sound, pid);

	cmd = nemosound_create_command(one, NEMOSOUND_SET_MUTE_COMMAND);
	if (cmd == NULL)
		return;
	cmd->mute = mute;

	if (one->has_sinkinput == 0) {
		pa_operation *op;

		op = pa_context_get_sink_input_info_list(sound->context, nemosound_dispatch_sink_input_info_callback, sound);
		if (op != NULL)
			pa_operation_unref(op);

		return;
	}

	nemosound_flush_command(sound, one);
}

static void nemo_sound_manager_set_mute_sink(void *data, struct nemo_sound_manager *manager, uint32_t sink, uint32_t mute)
{
	struct nemosound *sound = (struct nemosound *)data;
}

static void nemo_sound_manager_set_volume(void *data, struct nemo_sound_manager *manager, uint32_t pid, uint32_t volume)
{
	struct nemosound *sound = (struct nemosound *)data;
	struct soundone *one;
	struct soundcmd *cmd;

	one = nemosound_get_one(sound, pid);
	if (one == NULL)
		one = nemosound_create_one(sound, pid);

	cmd = nemosound_create_command(one, NEMOSOUND_SET_VOLUME_COMMAND);
	if (cmd == NULL)
		return;
	cmd->volume = volume;

	if (one->has_sinkinput == 0) {
		pa_operation *op;

		op = pa_context_get_sink_input_info_list(sound->context, nemosound_dispatch_sink_input_info_callback, sound);
		if (op != NULL)
			pa_operation_unref(op);

		return;
	}

	nemosound_flush_command(sound, one);
}

static void nemo_sound_manager_set_volume_sink(void *data, struct nemo_sound_manager *manager, uint32_t sink, uint32_t volume)
{
	struct nemosound *sound = (struct nemosound *)data;
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
		{ "log",			required_argument,	NULL,		'l' },
		{ 0 }
	};

	GMainLoop *gmainloop;
	struct nemosound *sound;
	struct nemotool *tool;
	int opt;

	nemolog_set_file(2);

	while (opt = getopt_long(argc, argv, "l:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'l':
				nemolog_open_socket(optarg);
				break;

			default:
				break;
		}
	}

	sound = (struct nemosound *)malloc(sizeof(struct nemosound));
	if (sound == NULL)
		return -1;
	memset(sound, 0, sizeof(struct nemosound));

	nemolist_init(&sound->list);
	nemolist_init(&sound->sink_list);

	sound->tool = tool = nemotool_create();
	if (tool == NULL)
		goto out1;
	nemotool_set_dispatch_global(tool, nemosound_dispatch_global);
	nemotool_set_userdata(tool, sound);
	nemotool_connect_wayland(tool, NULL);

	sound->mainloop = pa_glib_mainloop_new(NULL);
	if (sound->mainloop == NULL)
		goto out2;

	sound->mainapi = pa_glib_mainloop_get_api(sound->mainloop);
	if (sound->mainapi == NULL)
		goto out3;

	sound->proplist = pa_proplist_new();
	if (sound->proplist == NULL)
		goto out3;

	sound->context = pa_context_new_with_proplist(sound->mainapi, NULL, sound->proplist);
	if (sound->context == NULL)
		goto out4;

	pa_context_set_state_callback(sound->context, nemosound_dispatch_state_callback, sound);

	if (pa_context_connect(sound->context, NULL, 0, NULL) < 0)
		goto out5;

	gmainloop = g_main_loop_new(NULL, FALSE);
	nemoglib_run_tool(gmainloop, tool);
	g_main_loop_unref(gmainloop);

out5:
	pa_context_unref(sound->context);

out4:
	pa_proplist_free(sound->proplist);

out3:
	pa_glib_mainloop_free(sound->mainloop);

out2:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

out1:
	free(sound);

	return 0;
}

struct soundone *nemosound_create_one(struct nemosound *sound, uint32_t pid)
{
	struct soundone *one;

	one = (struct soundone *)malloc(sizeof(struct soundone));
	if (one == NULL)
		return NULL;
	memset(one, 0, sizeof(struct soundone));

	one->sound = sound;
	one->pid = pid;

	nemolist_init(&one->cmd_list);

	nemolist_insert(&sound->list, &one->link);

	return one;
}

void nemosound_destroy_one(struct soundone *one)
{
	nemolist_remove(&one->link);

	free(one);
}

struct soundone *nemosound_get_one(struct nemosound *sound, uint32_t pid)
{
	struct soundone *one;

	nemolist_for_each(one, &sound->list, link) {
		if (one->pid == pid)
			return one;
	}

	return NULL;
}

void nemosound_flush_command(struct nemosound *sound, struct soundone *one)
{
	struct soundcmd *cmd, *next;

	nemolist_for_each_safe(cmd, next, &one->cmd_list, link) {
		pa_operation *op = NULL;

		if (cmd->type == NEMOSOUND_SET_SINK_COMMAND) {
			op = pa_context_move_sink_input_by_index(sound->context, one->sinkinput, cmd->sink, nemosound_dispatch_move_sink_callback, one);

			one->next_sink = cmd->sink;
		} else if (cmd->type == NEMOSOUND_SET_MUTE_COMMAND) {
			op = pa_context_set_sink_input_mute(sound->context, one->sinkinput, cmd->mute, nemosound_dispatch_set_sink_input_mute_callback, one);
		} else if (cmd->type == NEMOSOUND_SET_VOLUME_COMMAND) {
			op = pa_context_get_sink_input_info(sound->context, one->sinkinput, nemosound_dispatch_sink_input_volume_callback, one);

			one->next_volume = cmd->volume;
		}

		if (op != NULL)
			pa_operation_unref(op);

		nemosound_destroy_command(cmd);
	}
}

struct soundsink *nemosound_create_sink(struct nemosound *sound, uint32_t id)
{
	struct soundsink *sink;

	sink = (struct soundsink *)malloc(sizeof(struct soundsink));
	if (sink == NULL)
		return NULL;
	memset(sink, 0, sizeof(struct soundsink));

	sink->sound = sound;
	sink->id = id;

	nemolist_insert(&sound->sink_list, &sink->link);

	return sink;
}

void nemosound_destroy_sink(struct soundsink *sink)
{
	nemolist_remove(&sink->link);

	free(sink);
}

struct soundsink *nemosound_get_sink(struct nemosound *sound, uint32_t id)
{
	struct soundsink *sink;

	nemolist_for_each(sink, &sound->sink_list, link) {
		if (sink->id == id)
			return sink;
	}

	return NULL;
}

struct soundcmd *nemosound_create_command(struct soundone *one, int type)
{
	struct soundcmd *cmd;

	cmd = (struct soundcmd *)malloc(sizeof(struct soundcmd));
	if (cmd == NULL)
		return NULL;
	memset(cmd, 0, sizeof(struct soundcmd));

	cmd->type = type;
	cmd->one = one;

	nemolist_insert(&one->cmd_list, &cmd->link);

	return cmd;
}

void nemosound_destroy_command(struct soundcmd *cmd)
{
	nemolist_remove(&cmd->link);

	free(cmd);
}