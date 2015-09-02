#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomote.h>
#include <builders/position.h>

int nemomote_positionbuilder_update(struct nemomote *mote, double secs, struct nemozone *zone)
{
	double x, y, z;
	int i;

	for (i = mote->lcount; i < mote->lcount + mote->rcount; i++) {
		nemozone_locates(zone, &x, &y, &z);

		NEMOMOTE_POSITION_X(mote, i) = x;
		NEMOMOTE_POSITION_Y(mote, i) = y;
		NEMOMOTE_POSITION_Z(mote, i) = z;
	}

	return 0;
}
