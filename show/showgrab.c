#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemoshow.h>
#include <showevent.h>
#include <showgrab.h>

struct showgrab *nemoshow_grab_create(struct nemoshow *show, struct showevent *event, nemoshow_grab_dispatch_event_t dispatch)
{
	struct showgrab *grab;

	grab = (struct showgrab *)malloc(sizeof(struct showgrab));
	if (grab == NULL)
		return NULL;
	memset(grab, 0, sizeof(struct showgrab));

	grab->show = show;
	grab->device = event->device;
	grab->dispatch_event = dispatch;

	nemolist_init(&grab->destroy_listener.link);

	nemolist_insert(&show->grab_list, &grab->link);

	return grab;
}

void nemoshow_grab_destroy(struct showgrab *grab)
{
	nemolist_remove(&grab->link);

	nemolist_remove(&grab->destroy_listener.link);

	free(grab);
}

static void nemoshow_grab_handle_destroy_signal(struct nemolistener *listener, void *data)
{
	struct showgrab *grab = (struct showgrab *)container_of(listener, struct showgrab, destroy_listener);

	nemoshow_grab_destroy(grab);
}

void nemoshow_grab_check_signal(struct showgrab *grab, struct nemosignal *signal)
{
	grab->destroy_listener.notify = nemoshow_grab_handle_destroy_signal;
	nemosignal_add(signal, &grab->destroy_listener);
}

int nemoshow_dispatch_grab(struct nemoshow *show, struct showevent *event)
{
	struct showgrab *grab;

	nemolist_for_each(grab, &show->grab_list, link) {
		if (grab->device == event->device)
			return grab->dispatch_event(show, grab, event);
	}

	return 0;
}

void nemoshow_dispatch_grab_all(struct nemoshow *show, struct showevent *event)
{
	struct showgrab *grab, *next;
	int i;

	for (i = 0; i < event->tapcount; i++) {
		nemolist_for_each_safe(grab, next, &show->grab_list, link) {
			if (grab->device == event->taps[i]->device)
				grab->dispatch_event(show, grab, event);
		}
	}
}
