#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <compz.h>
#include <backend.h>
#include <drmbackend.h>
#include <fbbackend.h>
#include <evdevbackend.h>
#include <nemomisc.h>

int nemocompz_load_backend(struct nemocompz *compz, const char *name, const char *args)
{
	if (strcmp(name, "drm") == 0) {
		drmbackend_create(compz, args);
	} else if (strcmp(name, "fb") == 0) {
		fbbackend_create(compz, args);
	} else if (strcmp(name, "evdev") == 0) {
		evdevbackend_create(compz, args);
	}

	return 0;
}
