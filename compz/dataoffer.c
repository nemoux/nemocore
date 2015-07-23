#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <dataoffer.h>
#include <datadevice.h>
#include <nemomisc.h>

static void data_offer_accept(struct wl_client *client, struct wl_resource *resource, uint32_t serial, const char *mime_type)
{
	struct nemodataoffer *offer = (struct nemodataoffer *)wl_resource_get_user_data(resource);

	if (offer->source != NULL)
		offer->source->accept(offer->source, serial, mime_type);
}

static void data_offer_receive(struct wl_client *client, struct wl_resource *resource, const char *mime_type, int32_t fd)
{
	struct nemodataoffer *offer = (struct nemodataoffer *)wl_resource_get_user_data(resource);

	if (offer->source != NULL)
		offer->source->send(offer->source, mime_type, fd);
	else
		close(fd);
}

static void data_offer_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static const struct wl_data_offer_interface data_offer_implementation = {
	data_offer_accept,
	data_offer_receive,
	data_offer_destroy
};

static void dataoffer_unbind_resource(struct wl_resource *resource)
{
	struct nemodataoffer *offer = (struct nemodataoffer *)wl_resource_get_user_data(resource);

	if (offer->source != NULL)
		wl_list_remove(&offer->source_destroy_listener.link);

	free(offer);
}

static void dataoffer_handle_source_destroy(struct wl_listener *listener, void *data)
{
	struct nemodataoffer *offer = (struct nemodataoffer *)container_of(listener, struct nemodataoffer, source_destroy_listener);

	offer->source = NULL;
}

struct wl_resource *dataoffer_create(struct nemodatasource *source, struct wl_resource *target)
{
	struct nemodataoffer *offer;
	char **p;

	offer = (struct nemodataoffer *)malloc(sizeof(struct nemodataoffer));
	if (offer == NULL)
		return NULL;
	memset(offer, 0, sizeof(struct nemodataoffer));

	offer->resource = wl_resource_create(wl_resource_get_client(target), &wl_data_offer_interface, 1, 0);
	if (offer->resource == NULL)
		goto err1;

	wl_resource_set_implementation(offer->resource, &data_offer_implementation, offer, dataoffer_unbind_resource);

	offer->source = source;
	offer->source_destroy_listener.notify = dataoffer_handle_source_destroy;
	wl_signal_add(&source->destroy_signal, &offer->source_destroy_listener);

	wl_data_device_send_data_offer(target, offer->resource);

	wl_array_for_each(p, &source->mime_types)
		wl_data_offer_send_offer(offer->resource, *p);

	return offer->resource;

err1:
	free(offer);

	return NULL;
}
