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
#include <nemomisc.h>

int nemocompz_load_plugin(struct nemocompz *compz, const char *path)
{
	int (*init)(struct nemocompz *compz);
	void *handle;
	char *err;

	handle = dlopen(path, RTLD_LAZY);
	if (handle == NULL)
		return -1;

	init = dlsym(handle, "nemoplugin_init");
	if ((err = dlerror()) != NULL)
		return -1;

	init(compz);

	return 0;
}

void nemocompz_load_plugins(struct nemocompz *compz)
{
	const char *path;
	int index = 0;

	for (index = 0;
			(index = nemoitem_get(compz->configs, "//nemoshell/plugin", index)) >= 0;
			index++) {
		path = nemoitem_get_attr(compz->configs, index, "path");

		nemocompz_load_plugin(compz, path);
	}
}
