#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomote.h>
#include <builders/velocity.h>

int nemomote_velocity_update(struct nemomote *mote, struct nemozone *zone)
{
	double x, y;
	int i;

	for (i = mote->lcount; i < mote->lcount + mote->rcount; i++) {
		nemozone_locates(zone, &x, &y);

		NEMOMOTE_VELOCITY_X(mote, i) = x;
		NEMOMOTE_VELOCITY_Y(mote, i) = y;
	}

	return 0;
}

int nemomote_velocity_set(struct nemomote *mote, int base, int count, struct nemozone *zone)
{
	double x, y;
	int i;

	for (i = base; i < base + count; i++) {
		nemozone_locates(zone, &x, &y);

		NEMOMOTE_VELOCITY_X(mote, i) = x;
		NEMOMOTE_VELOCITY_Y(mote, i) = y;
	}

	return 0;
}
