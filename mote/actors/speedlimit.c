#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <math.h>

#include <nemomote.h>
#include <actors/speedlimit.h>

int nemomote_speedlimit_update(struct nemomote *mote, uint32_t type, double secs, double min, double max)
{
	int i;

	for (i = 0; i < mote->lcount; i++) {
		if (mote->types[i] != type)
			continue;

		double speed =
			NEMOMOTE_VELOCITY_X(mote, i) * NEMOMOTE_VELOCITY_X(mote, i) +
			NEMOMOTE_VELOCITY_Y(mote, i) * NEMOMOTE_VELOCITY_Y(mote, i);

		if (speed < min * min) {
			double scale = min / sqrtf(speed);

			NEMOMOTE_VELOCITY_X(mote, i) *= scale;
			NEMOMOTE_VELOCITY_Y(mote, i) *= scale;
		}
		if (speed > max * max) {
			double scale = max / sqrtf(speed);

			NEMOMOTE_VELOCITY_X(mote, i) *= scale;
			NEMOMOTE_VELOCITY_Y(mote, i) *= scale;
		}
	}

	return 0;
}
