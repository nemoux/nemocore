#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showevent.h>

int nemoshow_event_dispatch_grab(struct nemoshow *show, void *event, nemotale_grab_dispatch_event_t dispatch, void *data, uint32_t tag, struct nemosignal *signal)
{
	struct talegrab *grab;

	grab = nemotale_grab_create(show->tale, event, dispatch);
	nemotale_grab_set_userdata(grab, data);
	nemotale_grab_set_tag(grab, tag);
	nemotale_grab_check_signal(grab, signal);

	nemotale_dispatch_grab(show->tale, event);

	return 0;
}
