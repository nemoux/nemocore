#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <actiontap.h>
#include <nemoaction.h>
#include <nemomisc.h>

struct actiontap *nemoaction_tap_create(struct nemoaction *action)
{
	struct actiontap *tap;

	tap = (struct actiontap *)malloc(sizeof(struct actiontap));
	if (tap == NULL)
		return NULL;
	memset(tap, 0, sizeof(struct actiontap));

	tap->action = action;
	tap->dispatch_event = action->dispatch_tap_event;

	nemolist_insert_tail(&action->tap_list, &tap->link);
	nemolist_init(&tap->listener.link);

	return tap;
}

void nemoaction_tap_destroy(struct actiontap *tap)
{
	nemolist_remove(&tap->link);
	nemolist_remove(&tap->listener.link);

	if (tap->traces != NULL)
		free(tap->traces);

	free(tap);
}

void nemoaction_tap_attach(struct nemoaction *action, struct actiontap *tap)
{
	nemolist_insert_tail(&action->tap_list, &tap->link);
}

void nemoaction_tap_detach(struct actiontap *tap)
{
	nemolist_remove(&tap->link);
	nemolist_init(&tap->link);
}

int nemoaction_tap_set_trace_max(struct actiontap *tap, int maximum)
{
	if (tap->traces != NULL)
		free(tap->traces);

	tap->traces = (float *)malloc(sizeof(float[2]) * maximum);
	if (tap->traces == NULL)
		return -1;

	tap->mtraces = maximum;
	tap->ntraces = 0;

	return 0;
}

static int nemoaction_tap_dispatch_no_event(struct nemoaction *action, struct actiontap *tap, uint32_t event)
{
	return 0;
}

static void nemoaction_tap_handle_destroy(struct nemolistener *listener, void *data)
{
	struct actiontap *tap = (struct actiontap *)container_of(listener, struct actiontap, listener);

	nemolist_remove(&tap->listener.link);
	nemolist_init(&tap->listener.link);

	tap->target = NULL;
	tap->dispatch_event = nemoaction_tap_dispatch_no_event;
}

int nemoaction_tap_set_focus(struct nemoaction *action, struct actiontap *tap, void *target)
{
	struct actionone *one;

	nemolist_remove(&tap->listener.link);
	nemolist_init(&tap->listener.link);

	one = nemoaction_get_one_by_target(action, target);
	if (one != NULL) {
		tap->target = one->target;
		tap->dispatch_event = one->dispatch_tap_event;

		if (one->signal != NULL) {
			tap->listener.notify = nemoaction_tap_handle_destroy;
			nemosignal_add(one->signal, &tap->listener);
		}

		return 1;
	}

	tap->target = NULL;
	tap->dispatch_event = nemoaction_tap_dispatch_no_event;

	return 0;
}
