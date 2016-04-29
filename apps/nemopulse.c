#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <pulse/pulseaudio.h>

#include <nemolog.h>
#include <nemomisc.h>

struct nemopulse {
	char *cmd;

	int sinkinput;
	int sink;

	char *name;

	int mute;
	uint32_t volume;

	int actions;
};

static void nemopulse_dispatch_drain_complete(pa_context *context, void *userdata)
{
	pa_context_disconnect(context);
}

static void nemopulse_complete_action(pa_context *context, void *userdata)
{
	struct nemopulse *pulse = (struct nemopulse *)userdata;

	pulse->actions--;

	if (pulse->actions <= 0) {
		pa_operation *op;

		op = pa_context_drain(context, nemopulse_dispatch_drain_complete, userdata);
		if (op == NULL)
			pa_context_disconnect(context);
		else
			pa_operation_unref(op);
	}
}

static void nemopulse_dispatch_simple_callback(pa_context *context, int success, void *userdata)
{
	nemolog_message("PULSE", "success(%d)\n", success);

	nemopulse_complete_action(context, userdata);
}

static void nemopulse_dispatch_client_info_callback(pa_context *context, const pa_client_info *info, int is_last, void *userdata)
{
	if (is_last < 0) {
		nemolog_error("PULSE", "failed to get client information: %s\n", pa_strerror(pa_context_errno(context)));
		return;
	}

	if (is_last != 0) {
		nemopulse_complete_action(context, userdata);
		return;
	}

	if (info == NULL)
		return;

	nemolog_message("PULSE", "CLIENT #%u: name(%s) pid(%s)\n",
			info->index,
			pa_proplist_gets(info->proplist, PA_PROP_APPLICATION_NAME),
			pa_proplist_gets(info->proplist, PA_PROP_APPLICATION_PROCESS_ID));
}

static void nemopulse_dispatch_sink_info_callback(pa_context *context, const pa_sink_info *info, int is_last, void *userdata)
{
	if (is_last < 0) {
		nemolog_error("PULSE", "failed to get sink information: %s\n", pa_strerror(pa_context_errno(context)));
		return;
	}

	if (is_last != 0) {
		nemopulse_complete_action(context, userdata);
		return;
	}

	nemolog_message("PULSE", "SINK #%u: name(%s) description(%s) volume(%d) mute(%d)\n",
			info->index,
			info->name,
			pa_proplist_gets(info->proplist, PA_PROP_DEVICE_DESCRIPTION),
			info->volume.values[0],
			info->mute);
}

static void nemopulse_dispatch_sink_input_info_callback(pa_context *context, const pa_sink_input_info *info, int is_last, void *userdata)
{
	if (is_last < 0) {
		nemolog_error("PULSE", "failed to get sink input information: %s\n", pa_strerror(pa_context_errno(context)));
		return;
	}

	if (is_last != 0) {
		nemopulse_complete_action(context, userdata);
		return;
	}

	nemolog_message("PULSE", "SINK-INPUT #%u: name(%s) pid(%s) volume(%d) mute(%d)\n",
			info->index,
			pa_proplist_gets(info->proplist, PA_PROP_APPLICATION_NAME),
			pa_proplist_gets(info->proplist, PA_PROP_APPLICATION_PROCESS_ID),
			info->volume.values[0],
			info->mute);
}

static void nemopulse_dispatch_sink_volume_callback(pa_context *context, const pa_sink_info *info, int is_last, void *userdata)
{
	struct nemopulse *pulse = (struct nemopulse *)userdata;
	pa_operation *op;
	pa_cvolume cv;

	if (is_last < 0) {
		nemolog_error("PULSE", "failed to get sink volume: %s\n", pa_strerror(pa_context_errno(context)));
		return;
	}

	if (is_last != 0) {
		nemopulse_complete_action(context, userdata);
		return;
	}

	pa_cvolume_set(&cv, info->channel_map.channels, (double)PA_VOLUME_NORM / 100.0f * pulse->volume);

	op = pa_context_set_sink_volume_by_index(context, pulse->sink, &cv, nemopulse_dispatch_simple_callback, pulse);
	if (op != NULL)
		pa_operation_unref(op);
}

static void nemopulse_dispatch_sink_input_volume_callback(pa_context *context, const pa_sink_input_info *info, int is_last, void *userdata)
{
	struct nemopulse *pulse = (struct nemopulse *)userdata;
	pa_operation *op;
	pa_cvolume cv;

	if (is_last < 0) {
		nemolog_error("PULSE", "failed to get sink input volume: %s\n", pa_strerror(pa_context_errno(context)));
		return;
	}

	if (is_last != 0) {
		nemopulse_complete_action(context, userdata);
		return;
	}

	pa_cvolume_set(&cv, info->channel_map.channels, (double)PA_VOLUME_NORM / 100.0f * pulse->volume);

	op = pa_context_set_sink_input_volume(context, pulse->sinkinput, &cv, nemopulse_dispatch_simple_callback, pulse);
	if (op != NULL)
		pa_operation_unref(op);
}

static void nemopulse_dispatch_state_callback(pa_context *context, void *userdata)
{
	struct nemopulse *pulse = (struct nemopulse *)userdata;
	pa_operation *op = NULL;

	switch (pa_context_get_state(context)) {
		case PA_CONTEXT_CONNECTING:
		case PA_CONTEXT_AUTHORIZING:
		case PA_CONTEXT_SETTING_NAME:
			break;

		case PA_CONTEXT_READY:
			if (pulse->cmd == NULL || strcmp(pulse->cmd, "list") == 0) {
				op = pa_context_get_client_info_list(context, nemopulse_dispatch_client_info_callback, pulse);
				if (op != NULL) {
					pa_operation_unref(op);

					pulse->actions++;
				}
				op = pa_context_get_sink_info_list(context, nemopulse_dispatch_sink_info_callback, pulse);
				if (op != NULL) {
					pa_operation_unref(op);

					pulse->actions++;
				}
				op = pa_context_get_sink_input_info_list(context, nemopulse_dispatch_sink_input_info_callback, pulse);
				if (op != NULL) {
					pa_operation_unref(op);

					pulse->actions++;
				}
			} else if (strcmp(pulse->cmd, "default") == 0) {
				if (pulse->name != NULL) {
					op = pa_context_set_default_sink(context, pulse->name, nemopulse_dispatch_simple_callback, pulse);
					if (op != NULL) {
						pa_operation_unref(op);

						pulse->actions++;
					}
				}
			} else if (strcmp(pulse->cmd, "move") == 0) {
				if (pulse->sinkinput >= 0 && pulse->sink >= 0) {
					op = pa_context_move_sink_input_by_index(context, pulse->sinkinput, pulse->sink, nemopulse_dispatch_simple_callback, pulse);
					if (op != NULL) {
						pa_operation_unref(op);

						pulse->actions++;
					}
				}
			} else if (strcmp(pulse->cmd, "mute") == 0) {
				if (pulse->sink >= 0) {
					op = pa_context_set_sink_mute_by_index(context, pulse->sink, pulse->mute, nemopulse_dispatch_simple_callback, pulse);
					if (op != NULL) {
						pa_operation_unref(op);

						pulse->actions++;
					}
				}
				if (pulse->sinkinput >= 0) {
					op = pa_context_set_sink_input_mute(context, pulse->sinkinput, pulse->mute, nemopulse_dispatch_simple_callback, pulse);
					if (op != NULL) {
						pa_operation_unref(op);

						pulse->actions++;
					}
				}
			} else if (strcmp(pulse->cmd, "volume") == 0) {
				if (pulse->sink >= 0) {
					op = pa_context_get_sink_info_by_index(context, pulse->sink, nemopulse_dispatch_sink_volume_callback, pulse);
					if (op != NULL) {
						pa_operation_unref(op);

						pulse->actions++;
					}
				}
				if (pulse->sinkinput >= 0) {
					op = pa_context_get_sink_input_info(context, pulse->sinkinput, nemopulse_dispatch_sink_input_volume_callback, pulse);
					if (op != NULL) {
						pa_operation_unref(op);

						pulse->actions++;
					}
				}
			}
			break;

		case PA_CONTEXT_TERMINATED:
			exit(0);
			break;

		case PA_CONTEXT_FAILED:
		default:
			nemolog_error("PULSE", "failed...\n");
			exit(-1);
			break;
	}
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "sinkinput",		required_argument,		NULL,		'i' },
		{ "sink",					required_argument,		NULL,		's' },
		{ "name",					required_argument,		NULL,		'n' },
		{ "mute",					required_argument,		NULL,		'm' },
		{ "volume",				required_argument,		NULL,		'v' },
		{ 0 }
	};
	struct nemopulse *pulse;
	pa_mainloop *mainloop;
	pa_mainloop_api *mainapi;
	pa_proplist *proplist;
	pa_context *context;
	char *cmd = NULL;
	char *name = NULL;
	int sinkinput = -1;
	int sink = -1;
	int mute = 0;
	int volume = 50;
	int opt;
	int r;

	while (opt = getopt_long(argc, argv, "i:s:n:m:v:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'i':
				sinkinput = strtoul(optarg, NULL, 10);
				break;

			case 's':
				sink = strtoul(optarg, NULL, 10);
				break;

			case 'n':
				name = strdup(optarg);
				break;

			case 'm':
				mute = strcmp(optarg, "on") == 0;
				break;

			case 'v':
				volume = strtoul(optarg, NULL, 10);
				break;

			default:
				break;
		}
	}

	if (optind < argc)
		cmd = strdup(argv[optind]);

	pulse = (struct nemopulse *)malloc(sizeof(struct nemopulse));
	if (pulse == NULL)
		return -1;
	memset(pulse, 0, sizeof(struct nemopulse));

	pulse->cmd = cmd;
	pulse->sinkinput = sinkinput;
	pulse->sink = sink;
	pulse->name = name;
	pulse->mute = mute;
	pulse->volume = MIN(volume, 100);

	proplist = pa_proplist_new();

	mainloop = pa_mainloop_new();
	if (mainloop == NULL)
		goto out1;

	mainapi = pa_mainloop_get_api(mainloop);

	context = pa_context_new_with_proplist(mainapi, NULL, proplist);
	if (context == NULL)
		goto out2;

	pa_context_set_state_callback(context, nemopulse_dispatch_state_callback, pulse);

	if (pa_context_connect(context, NULL, 0, NULL) < 0)
		goto out2;

	if (pa_mainloop_run(mainloop, &r) < 0)
		goto out2;

out2:
	pa_mainloop_free(mainloop);

out1:
	pa_proplist_free(proplist);

	if (pulse->cmd != NULL)
		free(pulse->cmd);
	if (pulse->name != NULL)
		free(pulse->name);

	free(pulse);

	return 0;
}
