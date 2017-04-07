#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <actionone.h>
#include <nemoaction.h>
#include <nemomisc.h>

struct actionone *nemoaction_one_create(struct nemoaction *action)
{
	struct actionone *one;

	one = (struct actionone *)malloc(sizeof(struct actionone));
	if (one == NULL)
		return NULL;
	memset(one, 0, sizeof(struct actionone));

	nemolist_insert_tail(&action->one_list, &one->link);
	nemolist_init(&one->listener.link);

	return one;
}

void nemoaction_one_destroy(struct actionone *one)
{
	nemolist_remove(&one->link);
	nemolist_remove(&one->listener.link);

	free(one);
}

static void nemoaction_one_handle_destroy(struct nemolistener *listener, void *data)
{
	struct actionone *one = (struct actionone *)container_of(listener, struct actionone, listener);

	nemoaction_one_destroy(one);
}

void nemoaction_one_set_tap_callback(struct nemoaction *action, void *target, struct nemosignal *signal, nemoaction_tap_dispatch_event_t dispatch)
{
	struct actionone *one;

	one = nemoaction_get_one_by_target(action, target);
	if (one == NULL) {
		one = nemoaction_one_create(action);
		one->target = target;
		one->signal = signal;

		if (signal != NULL) {
			one->listener.notify = nemoaction_one_handle_destroy;
			nemosignal_add(signal, &one->listener);
		}
	}

	one->dispatch_tap_event = dispatch;
}
