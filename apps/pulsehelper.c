#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <pulsehelper.h>
#include <nemolog.h>
#include <nemomisc.h>

static void nemopulse_dispatch_simple_callback(pa_context *context, int success, void *userdata)
{
}

static void nemopulse_dispatch_client_info_callback(pa_context *context, const pa_client_info *info, int is_last, void *userdata)
{
	struct nemopulse *pulse = (struct nemopulse *)userdata;

	if (is_last < 0) {
		nemolog_error("PULSE", "failed to get client information: %s\n", pa_strerror(pa_context_errno(context)));
		return;
	}

	if (is_last != 0)
		return;

	if (info == NULL)
		return;

	nemolog_message("PULSE", "CLIENT #%u: name(%s), pid(%s)\n",
			info->index,
			pa_proplist_gets(info->proplist, PA_PROP_APPLICATION_NAME),
			pa_proplist_gets(info->proplist, PA_PROP_APPLICATION_PROCESS_ID));
}

static void nemopulse_dispatch_sink_info_callback(pa_context *context, const pa_sink_info *info, int is_last, void *userdata)
{
	struct nemopulse *pulse = (struct nemopulse *)userdata;

	if (is_last < 0) {
		nemolog_error("PULSE", "failed to get sink information: %s\n", pa_strerror(pa_context_errno(context)));
		return;
	}

	if (is_last != 0)
		return;

	nemolog_message("PULSE", "SINK #%u: description(%s)\n",
			info->index,
			pa_proplist_gets(info->proplist, PA_PROP_DEVICE_DESCRIPTION));
}

static void nemopulse_dispatch_sink_input_info_callback(pa_context *context, const pa_sink_input_info *info, int is_last, void *userdata)
{
	struct nemopulse *pulse = (struct nemopulse *)userdata;
	const char *pid;

	if (is_last < 0) {
		nemolog_error("PULSE", "failed to get sink input information: %s\n", pa_strerror(pa_context_errno(context)));
		return;
	}

	if (is_last != 0)
		return;

	nemolog_message("PULSE", "SINK-INPUT #%u: name(%s), pid(%s)\n",
			info->index,
			pa_proplist_gets(info->proplist, PA_PROP_APPLICATION_NAME),
			pa_proplist_gets(info->proplist, PA_PROP_APPLICATION_PROCESS_ID));

	pid = pa_proplist_gets(info->proplist, PA_PROP_APPLICATION_PROCESS_ID);

	if (pulse->pid == strtoul(pid, NULL, 10)) {
		pulse->sinkinput = info->index;
		pulse->has_sinkinput = 1;

		nemopulse_set_sink(pulse, pulse->sink);

		nemolog_message("PULSE", "has sinkinput #%u\n", pulse->sinkinput);
	}
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

	if (is_last != 0)
		return;

	pulse->current_volume = MAX(MIN(info->volume.values[0] + (double)PA_VOLUME_NORM / 100.0f * pulse->volume_control, PA_VOLUME_NORM), 0.0f);

	pa_cvolume_set(&cv, info->channel_map.channels, pulse->current_volume);
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
			op = pa_context_get_client_info_list(context, nemopulse_dispatch_client_info_callback, pulse);
			if (op != NULL)
				pa_operation_unref(op);
			op = pa_context_get_sink_info_list(context, nemopulse_dispatch_sink_info_callback, pulse);
			if (op != NULL)
				pa_operation_unref(op);
			break;

		case PA_CONTEXT_TERMINATED:
			pulse->mainapi->quit(pulse->mainapi, 0);
			break;

		case PA_CONTEXT_FAILED:
		default:
			break;
	}
}

struct nemopulse *nemopulse_create(GMainContext *context)
{
	struct nemopulse *pulse;

	pulse = (struct nemopulse *)malloc(sizeof(struct nemopulse));
	if (pulse == NULL)
		return NULL;
	memset(pulse, 0, sizeof(struct nemopulse));

	pulse->current_volume = (double)PA_VOLUME_NORM * 0.5f;

	pulse->mainloop = pa_glib_mainloop_new(context);
	if (pulse->mainloop == NULL)
		goto err1;

	pulse->mainapi = pa_glib_mainloop_get_api(pulse->mainloop);
	if (pulse->mainapi == NULL)
		goto err2;

	pulse->proplist = pa_proplist_new();
	if (pulse->proplist == NULL)
		goto err2;

	pulse->context = pa_context_new_with_proplist(pulse->mainapi, NULL, pulse->proplist);
	if (pulse->context == NULL)
		goto err3;

	pa_context_set_state_callback(pulse->context, nemopulse_dispatch_state_callback, pulse);

	if (pa_context_connect(pulse->context, NULL, 0, NULL) < 0)
		goto err4;

	return pulse;

err4:
	pa_context_unref(pulse->context);

err3:
	pa_proplist_free(pulse->proplist);

err2:
	pa_glib_mainloop_free(pulse->mainloop);

err1:
	free(pulse);

	return NULL;
}

void nemopulse_destroy(struct nemopulse *pulse)
{
	pa_context_unref(pulse->context);

	pa_proplist_free(pulse->proplist);

	pa_glib_mainloop_free(pulse->mainloop);

	free(pulse);
}

void nemopulse_fetch_sink_input(struct nemopulse *pulse, uint32_t pid)
{
	pa_operation *op = NULL;

	pulse->pid = pid;

	op = pa_context_get_sink_input_info_list(pulse->context, nemopulse_dispatch_sink_input_info_callback, pulse);
	if (op != NULL)
		pa_operation_unref(op);
}

void nemopulse_set_volume(struct nemopulse *pulse, int volume)
{
	pa_operation *op = NULL;

	if (pulse->has_sinkinput == 0)
		return;

	pulse->volume_control = volume;

	op = pa_context_get_sink_input_info(pulse->context, pulse->sinkinput, nemopulse_dispatch_sink_input_volume_callback, pulse);
	if (op != NULL)
		pa_operation_unref(op);
}

void nemopulse_set_mute(struct nemopulse *pulse, int mute)
{
	pa_operation *op = NULL;

	if (pulse->has_sinkinput == 0)
		return;

	op = pa_context_set_sink_input_mute(pulse->context, pulse->sinkinput, mute, nemopulse_dispatch_simple_callback, pulse);
	if (op != NULL)
		pa_operation_unref(op);
}

void nemopulse_set_sink(struct nemopulse *pulse, int sink)
{
	pa_operation *op = NULL;
	pa_cvolume cv;

	pulse->sink = sink;

	if (pulse->has_sinkinput == 0)
		return;

	op = pa_context_move_sink_input_by_index(pulse->context, pulse->sinkinput, sink, nemopulse_dispatch_simple_callback, pulse);
	if (op != NULL)
		pa_operation_unref(op);

	pa_cvolume_set(&cv, 1, pulse->current_volume);
	op = pa_context_set_sink_input_volume(pulse->context, pulse->sinkinput, &cv, nemopulse_dispatch_simple_callback, pulse);
	if (op != NULL)
		pa_operation_unref(op);
}
