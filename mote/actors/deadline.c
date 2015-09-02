#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomote.h>
#include <actors/deadline.h>

int nemomote_deadline_update(struct nemomote *mote, double secs, struct nemozone *zone)
{
	int i;

	for (i = 0; i < mote->lcount; i++) {
		NEMOMOTE_LIFETIME(mote, i) -= secs;
		if (NEMOMOTE_LIFETIME(mote, i) <= 0.0f) {
			mote->attrs[i] |= NEMOMOTE_DEAD_BIT;
		}
	}

	return 0;
}
