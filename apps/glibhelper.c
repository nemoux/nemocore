#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <glib.h>

#include <nemotool.h>
#include <glibhelper.h>

struct nemosource {
	GSource source;
	GPollFD pfd;

	GMainLoop *mainloop;

	struct nemotool *tool;
};

static gboolean nemoglib_prepare_source(GSource *base, int *timeout)
{
	struct nemosource *source = (struct nemosource *)base;

	*timeout = -1;

	nemotool_flush(source->tool);

	return FALSE;
}

static gboolean nemoglib_check_source(GSource *base)
{
	struct nemosource *source = (struct nemosource *)base;

	return source->pfd.revents;
}

static gboolean nemoglib_dispatch_source(GSource *base, GSourceFunc callback, void *data)
{
	struct nemosource *source = (struct nemosource *)base;

	nemotool_dispatch(source->tool);

	if (nemotool_is_running(source->tool) == 0)
		g_main_loop_quit(source->mainloop);

	return TRUE;
}

static GSourceFuncs nemosource_handlers = {
	nemoglib_prepare_source,
	nemoglib_check_source,
	nemoglib_dispatch_source,
	NULL
};

int nemoglib_run_tool(GMainLoop *gmainloop, struct nemotool *tool)
{
	struct nemosource *source;
	GSource *gsource;

	source = (struct nemosource *)g_source_new(&nemosource_handlers, sizeof(struct nemosource));
	if (source == NULL)
		return -1;
	source->mainloop = gmainloop;
	source->tool = tool;
	source->pfd.fd = nemotool_get_fd(tool);
	source->pfd.events = G_IO_IN | G_IO_ERR;

	gsource = &source->source;

	g_source_add_poll(gsource, &source->pfd);

	g_source_attach(gsource, NULL);

	g_main_loop_run(gmainloop);

	g_source_destroy(gsource);

	g_source_unref(gsource);

	return 0;
}
