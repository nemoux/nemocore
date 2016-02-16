#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemoshow.h>
#include <showevent.h>

static int nemoshow_grab_dispatch_tale_event(struct talegrab *base, struct taleevent *event)
{
	struct showgrab *grab = (struct showgrab *)container_of(base, struct showgrab, base);
	int r;

	r = grab->dispatch_event(grab->show, grab->data, grab->tag, event);
	if (r == 0) {
		nemoshow_grab_destroy(grab);

		return 0;
	}

	return r;
}

struct showgrab *nemoshow_grab_create(struct nemoshow *show, void *event, nemoshow_grab_dispatch_event_t dispatch)
{
	struct showgrab *grab;

	grab = (struct showgrab *)malloc(sizeof(struct showgrab));
	if (grab == NULL)
		return NULL;
	memset(grab, 0, sizeof(struct showgrab));

	nemotale_grab_prepare(&grab->base, show->tale, event, nemoshow_grab_dispatch_tale_event);

	nemolist_init(&grab->destroy_listener.link);

	grab->dispatch_event = dispatch;

	return grab;
}

void nemoshow_grab_destroy(struct showgrab *grab)
{
	nemolist_remove(&grab->destroy_listener.link);

	nemotale_grab_finish(&grab->base);

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
