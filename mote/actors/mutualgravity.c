#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <math.h>

#include <nemomote.h>
#include <actors/mutualgravity.h>

int nemomote_mutualgravity_update(struct nemomote *mote, uint32_t type, double secs, double maxdist, double intensity, double epsilon)
{
	double dx, dy;
	double factor;
	double dsq, d;
	int i, j;

	for (i = 0; i < mote->lcount; i++) {
		if (mote->types[i] != type)
			continue;

		for (j = 0; j < mote->lcount; j++) {
			if (mote->types[j] != type)
				continue;

			dx = NEMOMOTE_POSITION_X(mote, j) - NEMOMOTE_POSITION_X(mote, i);
			if (dx <= 0.0f || dx > maxdist)
				continue;
			dy = NEMOMOTE_POSITION_Y(mote, j) - NEMOMOTE_POSITION_Y(mote, i);
			if (dy > maxdist || dy < -maxdist)
				continue;

			dsq = dx * dx + dy * dy;
			if (dsq <= (maxdist * maxdist) && dsq > 0.0f) {
				d = sqrtf(dsq);

				if (dsq < epsilon)
					dsq = epsilon;

				factor = (intensity * secs) / (dsq * d);
				dx = dx * factor;
				dy = dy * factor;

				NEMOMOTE_VELOCITY_X(mote, i) += dx * NEMOMOTE_MASS(mote, j);
				NEMOMOTE_VELOCITY_Y(mote, i) += dy * NEMOMOTE_MASS(mote, j);
				NEMOMOTE_VELOCITY_X(mote, j) -= dx * NEMOMOTE_MASS(mote, i);
				NEMOMOTE_VELOCITY_Y(mote, j) -= dy * NEMOMOTE_MASS(mote, i);
			}
		}
	}

	return 0;
}
