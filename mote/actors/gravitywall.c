#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <math.h>

#include <nemomote.h>
#include <actors/gravitywall.h>

int nemomote_gravitywallactor_update(struct nemomote *mote, double secs, double x, double y, double z, double intensity, double epsilon)
{
	int i;

	for (i = 0; i < mote->lcount; i++) {
		double dx = x - NEMOMOTE_POSITION_X(mote, i);
		double dy = y - NEMOMOTE_POSITION_Y(mote, i);
		double dz = z - NEMOMOTE_POSITION_Z(mote, i);
		double dsq = dx * dx + dy * dy + dz * dz;
		double d;

		if (dsq == 0.0f)
			break;

		d = sqrtf(dsq);

		if (dsq < epsilon)
			dsq = epsilon;

		double factor = (intensity * secs) / (dsq * d);

		NEMOMOTE_VELOCITY_X(mote, i) += dx * factor;
		NEMOMOTE_VELOCITY_Y(mote, i) += dy * factor;
		NEMOMOTE_VELOCITY_Z(mote, i) += dz * factor;
	}

	return 0;
}
