#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <minigrab.h>
#include <talehelper.h>

struct minigrab *minishell_grab_create(struct minishell *mini, struct nemotale *tale, struct taleevent *event, nemotale_dispatch_grab_t dispatch, void *data)
{
	struct minigrab *grab;

	grab = (struct minigrab *)malloc(sizeof(struct minigrab));
	if (grab == NULL)
		return NULL;
	memset(grab, 0, sizeof(struct minigrab));

	grab->mini = mini;
	grab->x = event->x;
	grab->y = event->y;
	grab->data = data;

	nemotale_prepare_grab(&grab->base, tale, event->device, dispatch);

	return grab;
}

void minishell_grab_destroy(struct minigrab *grab)
{
	nemotale_finish_grab(&grab->base);

	free(grab);
}
