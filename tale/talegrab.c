#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <talegrab.h>
#include <nemomisc.h>

static void talegrab_handle_tale_destroy(struct nemolistener *listener, void *data)
{
	struct talegrab *grab = (struct talegrab *)container_of(listener, struct talegrab, tale_destroy_listener);

	nemotale_destroy_grab(grab);
}

static void talegrab_handle_node_destroy(struct nemolistener *listener, void *data)
{
	struct talegrab *grab = (struct talegrab *)container_of(listener, struct talegrab, node_destroy_listener);

	nemotale_destroy_grab(grab);
}

int nemotale_prepare_grab(struct talegrab *grab, struct nemotale *tale, struct talenode *node, uint64_t device, nemotale_dispatch_grab_t dispatch)
{
	grab->tale = tale;
	grab->node = node;
	grab->device = device;
	grab->dispatch = dispatch;

	nemolist_insert(&tale->grab_list, &grab->link);

	grab->tale_destroy_listener.notify = talegrab_handle_tale_destroy;
	nemosignal_add(&tale->destroy_signal, &grab->tale_destroy_listener);

	grab->node_destroy_listener.notify = talegrab_handle_node_destroy;
	nemosignal_add(&node->destroy_signal, &grab->node_destroy_listener);

	return 0;
}

void nemotale_finish_grab(struct talegrab *grab)
{
	nemolist_remove(&grab->link);

	nemolist_remove(&grab->tale_destroy_listener.link);
	nemolist_remove(&grab->node_destroy_listener.link);
}

struct talegrab *nemotale_create_grab(struct nemotale *tale, struct talenode *node, uint64_t device, nemotale_dispatch_grab_t dispatch)
{
	struct talegrab *grab;

	grab = (struct talegrab *)malloc(sizeof(struct talegrab));
	if (grab == NULL)
		return NULL;

	nemotale_prepare_grab(grab, tale, node, device, dispatch);

	return grab;
}

void nemotale_destroy_grab(struct talegrab *grab)
{
	nemotale_finish_grab(grab);

	free(grab);
}

void nemotale_destroy_grab_with_device(struct nemotale *tale, uint64_t device)
{
	struct talegrab *grab;

	nemolist_for_each(grab, &tale->grab_list, link) {
		if (grab->device == device) {
			nemotale_finish_grab(grab);

			free(grab);

			return;
		}
	}
}
