#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <dlfcn.h>
#include <wayland-server.h>

#include <plugin.h>
#include <compz.h>
#include <nemolog.h>
#include <nemomisc.h>

int nemocompz_load_plugin(struct nemocompz *compz, const char *path, const char *args)
{
	int (*init)(struct nemocompz *compz, const char *args);
	void *handle;
	char *err;

	nemolog_message("PLUGIN", "load '%s' plugin...\n", path);

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

	init(compz, args);

	return 0;
}

void nemocompz_load_plugins(struct nemocompz *compz)
{
	const char *path;
	const char *args;
	int index = 0;

	for (index = 0;
			(index = nemoitem_get(compz->configs, "//nemoshell/plugin", index)) >= 0;
			index++) {
		path = nemoitem_get_attr(compz->configs, index, "path");
		args = nemoitem_get_attr(compz->configs, index, "args");

		nemocompz_load_plugin(compz, path, args);
	}
}
