#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <cookone.h>
#include <cookstate.h>
#include <nemomisc.h>

int nemocook_one_prepare(struct cookone *one)
{
	nemolist_init(&one->list);

	return 0;
}

void nemocook_one_finish(struct cookone *one)
{
	struct cookstate *state, *next;

	nemolist_for_each_safe(state, next, &one->list, link) {
		nemocook_state_destroy(state);
	}

	nemolist_remove(&one->list);
}

void nemocook_one_attach_state(struct cookone *one, struct cookstate *state)
{
	nemolist_insert_tail(&one->list, &state->link);
}

void nemocook_one_detach_state(struct cookone *one, int tag)
{
	struct cookstate *state, *next;

	nemolist_for_each_safe(state, next, &one->list, link) {
		if (state->tag == tag)
			nemocook_state_destroy(state);
	}
}

void nemocook_one_update_state(struct cookone *one)
{
	struct cookstate *state;

	nemolist_for_each(state, &one->list, link) {
		state->update(state);
	}
}
