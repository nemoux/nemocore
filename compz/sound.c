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

static void nemo_sound_set_sink(struct wl_client *client, struct wl_resource *sound_resource, uint32_t pid, uint32_t sink)
{
	struct nemosound *sound = (struct nemosound *)wl_resource_get_user_data(sound_resource);
}

static void nemo_sound_set_mute(struct wl_client *client, struct wl_resource *sound_resource, uint32_t pid, uint32_t mute)
{
	struct nemosound *sound = (struct nemosound *)wl_resource_get_user_data(sound_resource);
}

static void nemo_sound_set_mute_sink(struct wl_client *client, struct wl_resource *sound_resource, uint32_t sink, uint32_t mute)
{
	struct nemosound *sound = (struct nemosound *)wl_resource_get_user_data(sound_resource);
}

static void nemo_sound_set_volume(struct wl_client *client, struct wl_resource *sound_resource, uint32_t pid, uint32_t volume)
{
	struct nemosound *sound = (struct nemosound *)wl_resource_get_user_data(sound_resource);
}

static void nemo_sound_set_volume_sink(struct wl_client *client, struct wl_resource *sound_resource, uint32_t sink, uint32_t volume)
{
	struct nemosound *sound = (struct nemosound *)wl_resource_get_user_data(sound_resource);
}

static const struct nemo_sound_interface nemo_sound_implementation = {
	nemo_sound_set_sink,
	nemo_sound_set_mute,
	nemo_sound_set_mute_sink,
	nemo_sound_set_volume,
	nemo_sound_set_volume_sink
};

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
	wl_resource_set_implementation(resource, &nemo_sound_implementation, sound, nemosound_unbind);
}

static void nemo_sound_manager_sink(struct wl_client *client, struct wl_resource *sound_resource, uint32_t sink, const char *desc)
{
}

static const struct nemo_sound_manager_interface nemo_sound_manager_implementation = {
	nemo_sound_manager_sink
};

static void nemosound_unbind_manager(struct wl_resource *resource)
{
	struct nemosound *sound = (struct nemosound *)wl_resource_get_user_data(resource);

	sound->manager = NULL;
}

static void nemosound_bind_manager(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct nemosound *sound = (struct nemosound *)data;
	struct wl_resource *resource;

	resource = wl_resource_create(client, &nemo_sound_manager_interface, MIN(version, 1), id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	sound->manager = resource;

	wl_resource_set_implementation(resource, &nemo_sound_manager_implementation, sound, nemosound_unbind_manager);
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
	if (!wl_global_create(compz->display, &nemo_sound_manager_interface, 1, sound, nemosound_bind_manager))
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
