#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <shell.h>

#include <nemograb.h>
#include <talehelper.h>

struct nemograb *nemograb_create(struct nemoshell *shell, struct nemotale *tale, struct taleevent *event, nemotale_dispatch_grab_t dispatch)
{
	struct nemograb *grab;

	grab = (struct nemograb *)malloc(sizeof(struct nemograb));
	if (grab == NULL)
		return NULL;
	memset(grab, 0, sizeof(struct nemograb));

	grab->shell = shell;
	grab->x = event->x;
	grab->y = event->y;

	nemotale_prepare_grab(&grab->base, tale, event->device, dispatch);

	nemolist_init(&grab->destroy_listener.link);

	return grab;
}

void nemograb_destroy(struct nemograb *grab)
{
	nemolist_remove(&grab->destroy_listener.link);

	nemotale_finish_grab(&grab->base);

	free(grab);
}

static void nemograb_handle_destroy_signal(struct nemolistener *listener, void *data)
{
	struct nemograb *grab = (struct nemograb *)container_of(listener, struct nemograb, destroy_listener);

	nemograb_destroy(grab);
}

void nemograb_check_signal(struct nemograb *grab, struct nemosignal *signal)
{
	grab->destroy_listener.notify = nemograb_handle_destroy_signal;
	nemosignal_add(signal, &grab->destroy_listener);
}
