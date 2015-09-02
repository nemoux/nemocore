#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <math.h>

#include <nemomote.h>
#include <actors/mousegravity.h>

int nemomote_mousegravityactor_update(struct nemomote *mote, double secs, double x, double y, double z, double intensity)
{
	double dx, dy, dz;
	double dsq, d;
	int i;

	for (i = 0; i < mote->lcount; i++) {
		dx = NEMOMOTE_POSITION_X(mote, i) - x;
		dy = NEMOMOTE_POSITION_Y(mote, i) - y;
		dz = NEMOMOTE_POSITION_Z(mote, i) - z;
		dsq = dx * dx + dy * dy + dz * dz;
		d = sqrtf(dsq);

		NEMOMOTE_VELOCITY_X(mote, i) -= intensity / d * dx;
		NEMOMOTE_VELOCITY_Y(mote, i) -= intensity / d * dy;
		NEMOMOTE_VELOCITY_Z(mote, i) -= intensity / d * dz;
	}

	return 0;
}
