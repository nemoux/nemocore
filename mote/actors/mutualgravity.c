#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <math.h>

#include <nemomote.h>
#include <actors/mutualgravity.h>

int nemomote_mutualgravityactor_update(struct nemomote *mote, double secs, double maxdist, double intensity, double epsilon)
{
	double dx, dy, dz;
	double factor;
	double dsq, d;
	int i, j;

	for (i = 0; i < mote->lcount; i++) {
		for (j = 0; j < mote->lcount; j++) {
			if (i == j)
				continue;

			dx = NEMOMOTE_POSITION_X(mote, j) - NEMOMOTE_POSITION_X(mote, i);
			if (dx > maxdist)
				break;
			dy = NEMOMOTE_POSITION_Y(mote, j) - NEMOMOTE_POSITION_Y(mote, i);
			if (dy > maxdist || dy < -maxdist)
				continue;
			dz = NEMOMOTE_POSITION_Z(mote, j) - NEMOMOTE_POSITION_Z(mote, i);
			if (dz > maxdist || dz < -maxdist)
				continue;

			dsq = dx * dx + dy * dy + dz * dz;
			if (dsq <= (maxdist * maxdist) && dsq > 0.0f) {
				d = sqrtf(dsq);

				if (dsq < epsilon)
					dsq = epsilon;

				factor = (intensity * secs) / (dsq * d);
				dx = dx * factor;
				dy = dy * factor;
				dz = dz * factor;

				NEMOMOTE_VELOCITY_X(mote, i) += dx * NEMOMOTE_MASS(mote, j);
				NEMOMOTE_VELOCITY_Y(mote, i) += dy * NEMOMOTE_MASS(mote, j);
				NEMOMOTE_VELOCITY_Z(mote, i) += dz * NEMOMOTE_MASS(mote, j);
				NEMOMOTE_VELOCITY_X(mote, j) += dx * NEMOMOTE_MASS(mote, i);
				NEMOMOTE_VELOCITY_Y(mote, j) += dy * NEMOMOTE_MASS(mote, i);
				NEMOMOTE_VELOCITY_Z(mote, j) += dz * NEMOMOTE_MASS(mote, i);
			}
		}
	}

	return 0;
}
