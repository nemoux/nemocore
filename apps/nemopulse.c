#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <pulse/pulseaudio.h>

#include <nemolog.h>
#include <nemomisc.h>

static void nemopulse_dispatch_client_info_callback(pa_context *context, const pa_client_info *info, int is_last, void *userdata)
{
	char *pl;

	if (is_last < 0) {
		nemolog_error("PULSE", "failed to get client information: %s\n", pa_strerror(pa_context_errno(context)));
		exit(-1);
		return;
	}

	if (info == NULL)
		return;

	pl = pa_proplist_to_string_sep(info->proplist, "\n\t");

	nemolog_message("PULSE", "CLIENT #%u: pid(%s)\n\t%s\n",
			info->index,
			pa_proplist_gets(info->proplist, PA_PROP_APPLICATION_PROCESS_ID),
			pl);

	pa_xfree(pl);
}

static void nemopulse_dispatch_state_callback(pa_context *context, void *userdata)
{
	pa_operation *op = NULL;

	switch (pa_context_get_state(context)) {
		case PA_CONTEXT_CONNECTING:
		case PA_CONTEXT_AUTHORIZING:
		case PA_CONTEXT_SETTING_NAME:
			break;

		case PA_CONTEXT_READY:
			op = pa_context_get_client_info_list(context, nemopulse_dispatch_client_info_callback, NULL);
			if (op != NULL)
				pa_operation_unref(op);
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
	pa_mainloop *mainloop;
	pa_mainloop_api *mainapi;
	pa_proplist *proplist;
	pa_context *context;
	int r;

	nemolog_set_file(2);

	proplist = pa_proplist_new();

	mainloop = pa_mainloop_new();
	if (mainloop == NULL)
		goto out1;

	mainapi = pa_mainloop_get_api(mainloop);

	context = pa_context_new_with_proplist(mainapi, NULL, proplist);
	if (context == NULL)
		goto out2;

	pa_context_set_state_callback(context, nemopulse_dispatch_state_callback, NULL);

	if (pa_context_connect(context, NULL, 0, NULL) < 0)
		goto out2;

	if (pa_mainloop_run(mainloop, &r) < 0)
		goto out2;

out2:
	pa_mainloop_free(mainloop);

out1:
	pa_proplist_free(proplist);

	return 0;
}
