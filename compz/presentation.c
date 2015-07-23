#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>
#include <wayland-presentation-timing-server-protocol.h>

#include <compz.h>
#include <canvas.h>
#include <screen.h>
#include <presentation.h>

static void presentation_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void nemopresentation_unbind_feedback(struct wl_resource *resource)
{
	struct nemofeedback *feedback = (struct nemofeedback *)wl_resource_get_user_data(resource);

	wl_list_remove(&feedback->link);

	free(feedback);
}

static void presentation_feedback(struct wl_client *client, struct wl_resource *presentation_resource, struct wl_resource *surface_resource, uint32_t callback)
{
	struct nemocanvas *canvas = (struct nemocanvas *)wl_resource_get_user_data(surface_resource);
	struct nemocompz *compz = canvas->compz;
	struct nemofeedback *feedback;

	feedback = (struct nemofeedback *)malloc(sizeof(struct nemofeedback));
	if (feedback == NULL)
		goto err1;
	memset(feedback, 0, sizeof(struct nemofeedback));

	feedback->resource = wl_resource_create(client, &presentation_feedback_interface, 1, callback);
	if (feedback->resource == NULL)
		goto err2;

	wl_resource_set_implementation(feedback->resource, NULL, feedback, nemopresentation_unbind_feedback);

	wl_list_insert(&canvas->pending.feedback_list, &feedback->link);

	return;

err2:
	free(feedback);

err1:
	wl_client_post_no_memory(client);
}

static const struct presentation_interface presentation_implementation = {
	presentation_destroy,
	presentation_feedback
};

int nemopresentation_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct wl_resource *resource;

	resource = wl_resource_create(client, &presentation_interface, version, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return -1;
	}

	wl_resource_set_implementation(resource, &presentation_implementation, NULL, NULL);

	return 0;
}

void nemopresentation_discard_feedback_list(struct wl_list *list)
{
	struct nemofeedback *feedback, *tmp;

	wl_list_for_each_safe(feedback, tmp, list, link) {
		presentation_feedback_send_discarded(feedback->resource);

		wl_resource_destroy(feedback->resource);
	}
}

void nemopresentation_present_feedback_list(struct wl_list *list, struct nemoscreen *screen, uint32_t refresh, uint32_t secs, uint32_t nsecs, uint64_t seq, uint32_t psf_flags)
{
	struct nemofeedback *feedback, *tmp;
	struct wl_client *client;
	struct wl_resource *resource;

	wl_list_for_each_safe(feedback, tmp, list, link) {
		client = wl_resource_get_client(feedback->resource);

		wl_resource_for_each(resource, &screen->resource_list) {
			if (wl_resource_get_client(resource) != client)
				continue;

			presentation_feedback_send_sync_output(feedback->resource, resource);
		}

		presentation_feedback_send_presented(feedback->resource,
				0, secs,
				nsecs,
				refresh,
				seq >> 32, seq & 0xfffffffff,
				psf_flags | feedback->psf_flags);

		wl_client_flush(wl_resource_get_client(feedback->resource));

		wl_resource_destroy(feedback->resource);
	}
}
