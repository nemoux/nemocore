#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <talegrab.h>
#include <nemomisc.h>

static void nemotale_grab_handle_tale_destroy_signal(struct nemolistener *listener, void *data)
{
	struct talegrab *grab = (struct talegrab *)container_of(listener, struct talegrab, tale_destroy_listener);

	nemotale_grab_destroy(grab);
}

int nemotale_grab_prepare(struct talegrab *grab, struct nemotale *tale, struct taleevent *event, nemotale_grab_dispatch_event_t dispatch)
{
	grab->tale = tale;
	grab->device = event->device;
	grab->dispatch_event = dispatch;

	nemolist_insert(&tale->grab_list, &grab->link);

	grab->tale_destroy_listener.notify = nemotale_grab_handle_tale_destroy_signal;
	nemosignal_add(&tale->destroy_signal, &grab->tale_destroy_listener);

	nemolist_init(&grab->destroy_listener.link);

	return 0;
}

void nemotale_grab_finish(struct talegrab *grab)
{
	nemolist_remove(&grab->link);

	nemolist_remove(&grab->tale_destroy_listener.link);
	nemolist_remove(&grab->destroy_listener.link);
}

struct talegrab *nemotale_grab_create(struct nemotale *tale, struct taleevent *event, nemotale_grab_dispatch_event_t dispatch)
{
	struct talegrab *grab;

	grab = (struct talegrab *)malloc(sizeof(struct talegrab));
	if (grab == NULL)
		return NULL;

	nemotale_grab_prepare(grab, tale, event, dispatch);

	return grab;
}

void nemotale_grab_destroy(struct talegrab *grab)
{
	nemotale_grab_finish(grab);

	free(grab);
}

static void nemotale_grab_handle_destroy_signal(struct nemolistener *listener, void *data)
{
	struct talegrab *grab = (struct talegrab *)container_of(listener, struct talegrab, destroy_listener);

	nemotale_grab_destroy(grab);
}

void nemotale_grab_check_signal(struct talegrab *grab, struct nemosignal *signal)
{
	grab->destroy_listener.notify = nemotale_grab_handle_destroy_signal;
	nemosignal_add(signal, &grab->destroy_listener);
}
