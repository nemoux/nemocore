#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>
#include <wayland-nemo-sound-server-protocol.h>

#include <sound.h>
#include <compz.h>
#include <nemomisc.h>

static void nemosound_unbind(struct wl_resource *resource)
{
	wl_list_remove(wl_resource_get_link(resource));
}

static void nemosound_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct nemosound *sound = (struct nemosound *)data;
	struct wl_resource *resource;

	resource = wl_resource_create(client, &nemo_sound_interface, MIN(version, 1), id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_list_insert(&sound->resource_list, wl_resource_get_link(resource));
	wl_resource_set_implementation(resource, NULL, sound, nemosound_unbind);
}

struct nemosound *nemosound_create(struct nemocompz *compz)
{
	struct nemosound *sound;

	sound = (struct nemosound *)malloc(sizeof(struct nemosound));
	if (sound == NULL)
		return NULL;
	memset(sound, 0, sizeof(struct nemosound));

	sound->compz = compz;

	wl_signal_init(&sound->destroy_signal);

	wl_list_init(&sound->resource_list);

	if (!wl_global_create(compz->display, &nemo_sound_interface, 1, sound, nemosound_bind))
		goto err1;

	return sound;

err1:
	free(sound);

	return NULL;
}

void nemosound_destroy(struct nemosound *sound)
{
	wl_signal_emit(&sound->destroy_signal, sound);

	free(sound);
}
