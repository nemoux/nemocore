#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <dlfcn.h>
#include <wayland-client.h>

#include <nemoplugin.h>
#include <nemotool.h>
#include <nemochannel.h>
#include <nemoqueue.h>
#include <nemolog.h>
#include <nemomisc.h>

int nemotool_load_plugin(struct nemotool *tool, const char *path, const char *args, struct nemochannel *chan, struct nemoqueue *queue)
{
	int (*init)(struct nemotool *tool, const char *args, struct nemochannel *chan, struct nemoqueue *queue);
	void *handle;
	char *err;

	nemolog_message("PLUGIN", "load '%s' plugin\n", path);

	handle = dlopen(path, RTLD_LAZY);
	if (handle == NULL) {
		nemolog_error("PLUGIN", "failed to open '%s': %s\n", path, dlerror());

		return -1;
	}

	init = dlsym(handle, "nemoplugin_init");
	if ((err = dlerror()) != NULL) {
		nemolog_error("PLUGIN", "failed to open '%s:nemoplugin_init': %s\n", path, err);

		return -1;
	}

	return init(tool, args, chan, queue);
}
