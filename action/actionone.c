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

	return one;
}

void nemoaction_one_destroy(struct actionone *one)
{
	nemolist_remove(&one->link);

	free(one);
}

void nemoaction_one_set_tap_callback(struct nemoaction *action, void *target, nemoaction_tap_dispatch_event_t dispatch)
{
	struct actionone *one;

	one = nemoaction_get_one_by_target(action, target);
	if (one == NULL) {
		one = nemoaction_one_create(action);
		one->target = target;
	}

	one->dispatch_tap_event = dispatch;
}
