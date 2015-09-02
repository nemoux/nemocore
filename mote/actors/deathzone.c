#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomote.h>
#include <actors/deathzone.h>

int nemomote_deathzone_update(struct nemomote *mote, double secs, struct nemozone *zone)
{
	double x, y, z;
	int i;

	for (i = 0; i < mote->lcount; i++) {
		if (nemozone_contains(zone, NEMOMOTE_POSITION_X(mote, i), NEMOMOTE_POSITION_Y(mote, i), NEMOMOTE_POSITION_Z(mote, i))) {
			mote->attrs[i] |= NEMOMOTE_DEAD_BIT;
		}
	}

	return 0;
}
