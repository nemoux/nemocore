#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <math.h>

#include <nemomote.h>
#include <actors/gravitywell.h>

int nemomote_gravitywell_update(struct nemomote *mote, uint32_t type, double secs, double x, double y, double intensity, double epsilon)
{
	int i;

	for (i = 0; i < mote->lcount; i++) {
		if (mote->types[i] != type)
			continue;

		double dx = x - NEMOMOTE_POSITION_X(mote, i);
		double dy = y - NEMOMOTE_POSITION_Y(mote, i);
		double dsq = dx * dx + dy * dy;
		double d;

		if (dsq == 0.0f)
			break;

		d = sqrtf(dsq);

		if (dsq < epsilon)
			dsq = epsilon;

		double factor = (intensity * secs) / (dsq * d);

		NEMOMOTE_VELOCITY_X(mote, i) += dx * factor;
		NEMOMOTE_VELOCITY_Y(mote, i) += dy * factor;
	}

	return 0;
}
