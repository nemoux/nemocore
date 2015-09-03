#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <math.h>

#include <nemomote.h>
#include <actors/mousegravity.h>

int nemomote_mousegravity_update(struct nemomote *mote, uint32_t type, double secs, double x, double y, double intensity)
{
	double dx, dy;
	double dsq, d;
	int i;

	for (i = 0; i < mote->lcount; i++) {
		if (mote->types[i] != type)
			continue;

		dx = NEMOMOTE_POSITION_X(mote, i) - x;
		dy = NEMOMOTE_POSITION_Y(mote, i) - y;
		dsq = dx * dx + dy * dy;
		d = sqrtf(dsq);

		NEMOMOTE_VELOCITY_X(mote, i) -= intensity / d * dx;
		NEMOMOTE_VELOCITY_Y(mote, i) -= intensity / d * dy;
	}

	return 0;
}
