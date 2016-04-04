#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomote.h>
#include <actors/livezone.h>

int nemomote_livezone_update(struct nemomote *mote, uint32_t type, double secs, struct nemozone *zone)
{
	int i;

	for (i = 0; i < mote->lcount; i++) {
		if (mote->types[i] != type)
			continue;

		if (!nemozone_contain(zone, NEMOMOTE_POSITION_X(mote, i), NEMOMOTE_POSITION_Y(mote, i))) {
			mote->attrs[i] |= NEMOMOTE_DEAD_BIT;
		}
	}

	return 0;
}
