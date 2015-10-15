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
	int sinkinput;
	int sink;
};

static void nemopulse_dispatch_simple_callback(pa_context *context, int success, void *userdata)
{
	nemolog_message("PULSE", "success(%d)\n", success);
}

static void nemopulse_dispatch_client_info_callback(pa_context *context, const pa_client_info *info, int is_last, void *userdata)
{
	char *pl;

	if (is_last < 0) {
		nemolog_error("PULSE", "failed to get client information: %s\n", pa_strerror(pa_context_errno(context)));
		return;
	}

	if (is_last != 0)
		return;

	if (info == NULL)
		return;

	pl = pa_proplist_to_string_sep(info->proplist, "\n\t");

	nemolog_message("PULSE", "CLIENT #%u: pid(%s)\n\t%s\n",
			info->index,
			pa_proplist_gets(info->proplist, PA_PROP_APPLICATION_PROCESS_ID),
			pl);

	pa_xfree(pl);
}

static void nemopulse_dispatch_sink_info_callback(pa_context *context, const pa_sink_info *info, int is_last, void *userdata)
{
	char *pl;

	if (is_last < 0) {
		nemolog_error("PULSE", "failed to get sink information: %s\n", pa_strerror(pa_context_errno(context)));
		return;
	}

	if (is_last != 0)
		return;

	pl = pa_proplist_to_string_sep(info->proplist, "\n\t");

	nemolog_message("PULSE", "SINK #%u:\n\t%s\n",
			info->index,
			pl);

	pa_xfree(pl);
}

static void nemopulse_dispatch_sink_input_info_callback(pa_context *context, const pa_sink_input_info *info, int is_last, void *userdata)
{
	char *pl;

	if (is_last < 0) {
		nemolog_error("PULSE", "failed to get sink input information: %s\n", pa_strerror(pa_context_errno(context)));
		return;
	}

	if (is_last != 0)
		return;

	pl = pa_proplist_to_string_sep(info->proplist, "\n\t");

	nemolog_message("PULSE", "SINK-INPUT #%u:\n\t%s\n",
			info->index,
			pl);

	pa_xfree(pl);
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
			if (pulse->sinkinput >= 0 && pulse->sink >= 0) {
				op = pa_context_move_sink_input_by_index(context, pulse->sinkinput, pulse->sink, nemopulse_dispatch_simple_callback, NULL);
				if (op != NULL)
					pa_operation_unref(op);
			} else {
				op = pa_context_get_client_info_list(context, nemopulse_dispatch_client_info_callback, NULL);
				if (op != NULL)
					pa_operation_unref(op);
				op = pa_context_get_sink_info_list(context, nemopulse_dispatch_sink_info_callback, NULL);
				if (op != NULL)
					pa_operation_unref(op);
				op = pa_context_get_sink_input_info_list(context, nemopulse_dispatch_sink_input_info_callback, NULL);
				if (op != NULL)
					pa_operation_unref(op);
			}
			break;

		case PA_CONTEXT_TERMINATED:
			nemolog_message("PULSE", "terminated...\n");
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
		{ 0 }
	};
	struct nemopulse *pulse;
	pa_mainloop *mainloop;
	pa_mainloop_api *mainapi;
	pa_proplist *proplist;
	pa_context *context;
	int sinkinput = -1;
	int sink = -1;
	int opt;
	int r;

	while (opt = getopt_long(argc, argv, "i:s:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'i':
				sinkinput = strtoul(optarg, NULL, 10);
				break;

			case 's':
				sink = strtoul(optarg, NULL, 10);
				break;

			default:
				break;
		}
	}

	nemolog_set_file(2);

	pulse = (struct nemopulse *)malloc(sizeof(struct nemopulse));
	if (pulse == NULL)
		return -1;
	memset(pulse, 0, sizeof(struct nemopulse));

	pulse->sinkinput = sinkinput;
	pulse->sink = sink;

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

	free(pulse);

	return 0;
}
