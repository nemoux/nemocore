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

	if (sound->manager != NULL)
		nemo_sound_manager_send_set_sink(sound->manager, pid, sink);
}

static void nemo_sound_put_sink(struct wl_client *client, struct wl_resource *sound_resource, uint32_t pid)
{
	struct nemosound *sound = (struct nemosound *)wl_resource_get_user_data(sound_resource);

	if (sound->manager != NULL)
		nemo_sound_manager_send_put_sink(sound->manager, pid);
}

static void nemo_sound_set_mute(struct wl_client *client, struct wl_resource *sound_resource, uint32_t pid, uint32_t mute)
{
	struct nemosound *sound = (struct nemosound *)wl_resource_get_user_data(sound_resource);

	if (sound->manager != NULL)
		nemo_sound_manager_send_set_mute(sound->manager, pid, mute);
}

static void nemo_sound_set_mute_sink(struct wl_client *client, struct wl_resource *sound_resource, uint32_t sink, uint32_t mute)
{
	struct nemosound *sound = (struct nemosound *)wl_resource_get_user_data(sound_resource);

	if (sound->manager != NULL)
		nemo_sound_manager_send_set_mute_sink(sound->manager, sink, mute);
}

static void nemo_sound_set_volume(struct wl_client *client, struct wl_resource *sound_resource, uint32_t pid, uint32_t volume)
{
	struct nemosound *sound = (struct nemosound *)wl_resource_get_user_data(sound_resource);

	if (sound->manager != NULL)
		nemo_sound_manager_send_set_volume(sound->manager, pid, volume);
}

static void nemo_sound_set_volume_sink(struct wl_client *client, struct wl_resource *sound_resource, uint32_t sink, uint32_t volume)
{
	struct nemosound *sound = (struct nemosound *)wl_resource_get_user_data(sound_resource);

	if (sound->manager != NULL)
		nemo_sound_manager_send_set_volume_sink(sound->manager, sink, volume);
}

static const struct nemo_sound_interface nemo_sound_implementation = {
	nemo_sound_set_sink,
	nemo_sound_put_sink,
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

static void nemo_sound_manager_register_sink(struct wl_client *client, struct wl_resource *resource, uint32_t sinkid, const char *name, const char *desc)
{
	struct nemosound *sound = (struct nemosound *)wl_resource_get_user_data(resource);
	struct soundsink *sink;

	sink = (struct soundsink *)malloc(sizeof(struct soundsink));
	if (sink == NULL) {
		wl_resource_post_no_memory(resource);
		return;
	}
	memset(sink, 0, sizeof(struct soundsink));

	sink->id = sinkid;
	sink->name = strdup(name);
	sink->desc = strdup(desc);

	wl_list_insert(sound->sink_list.prev, &sink->link);
}

static void nemo_sound_manager_unregister_sink(struct wl_client *client, struct wl_resource *resource, uint32_t sinkid)
{
}

static void nemo_sound_manager_synchronize_sink(struct wl_client *client, struct wl_resource *resource, uint32_t sinkid, uint32_t volume, uint32_t mute)
{
	struct nemosound *sound = (struct nemosound *)wl_resource_get_user_data(resource);
	struct soundinfo info;

	info.id = sinkid;
	info.is_sink = 1;
	info.volume = volume;
	info.mute = mute;

	wl_signal_emit(&sound->info_signal, &info);
}

static void nemo_sound_manager_synchronize_sinkinput(struct wl_client *client, struct wl_resource *resource, uint32_t pid, uint32_t volume, uint32_t mute)
{
	struct nemosound *sound = (struct nemosound *)wl_resource_get_user_data(resource);
	struct soundinfo info;

	info.id = pid;
	info.is_sink = 0;
	info.volume = volume;
	info.mute = mute;

	wl_signal_emit(&sound->info_signal, &info);
}

static const struct nemo_sound_manager_interface nemo_sound_manager_implementation = {
	nemo_sound_manager_register_sink,
	nemo_sound_manager_unregister_sink,
	nemo_sound_manager_synchronize_sink,
	nemo_sound_manager_synchronize_sinkinput,
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

	wl_signal_init(&sound->info_signal);

	wl_list_init(&sound->sink_list);

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

struct soundsink *nemosound_get_main_sink(struct nemosound *sound)
{
	if (wl_list_empty(&sound->sink_list))
		return NULL;

	return (struct soundsink *)container_of(sound->sink_list.next, struct soundsink, link);
}

struct soundsink *nemosound_get_sink_by_name(struct nemosound *sound, const char *name)
{
	struct soundsink *sink;

	wl_list_for_each(sink, &sound->sink_list, link) {
		if (strcmp(sink->name, name) == 0)
			return sink;
	}

	return NULL;
}

void nemosound_set_sink(struct nemosound *sound, uint32_t pid, uint32_t sink)
{
	if (sound->manager != NULL)
		nemo_sound_manager_send_set_sink(sound->manager, pid, sink);
}

void nemosound_put_sink(struct nemosound *sound, uint32_t pid)
{
	if (sound->manager != NULL)
		nemo_sound_manager_send_put_sink(sound->manager, pid);
}

void nemosound_set_mute(struct nemosound *sound, uint32_t pid, uint32_t mute)
{
	if (sound->manager != NULL)
		nemo_sound_manager_send_set_mute(sound->manager, pid, mute);
}

void nemosound_set_mute_sink(struct nemosound *sound, uint32_t sink, uint32_t mute)
{
	if (sound->manager != NULL)
		nemo_sound_manager_send_set_mute_sink(sound->manager, sink, mute);
}

void nemosound_set_volume(struct nemosound *sound, uint32_t pid, uint32_t volume)
{
	if (sound->manager != NULL)
		nemo_sound_manager_send_set_volume(sound->manager, pid, volume);
}

void nemosound_set_volume_sink(struct nemosound *sound, uint32_t sink, uint32_t volume)
{
	if (sound->manager != NULL)
		nemo_sound_manager_send_set_volume_sink(sound->manager, sink, volume);
}

void nemosound_get_info(struct nemosound *sound, uint32_t pid)
{
	if (sound->manager != NULL)
		nemo_sound_manager_send_get_info(sound->manager, pid);
}

void nemosound_get_info_sink(struct nemosound *sound, uint32_t sink)
{
	if (sound->manager != NULL)
		nemo_sound_manager_send_get_info_sink(sound->manager, sink);
}
