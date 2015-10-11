#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <math.h>

#include <nemomote.h>
#include <actors/collide.h>

int nemomote_collide_update(struct nemomote *mote, uint32_t src, uint32_t dst, double secs, double bounce)
{
	double dx, dy;
	double factor;
	double dsq, d;
	double cd;
	double n1, n2;
	double m1, m2;
	double f1, f2;
	double rn;
	int i, j;

	for (i = 0; i < mote->lcount; i++) {
		if (mote->types[i] != src)
			continue;

		for (j = 0; j < mote->lcount; j++) {
			if (mote->types[j] != dst || i == j)
				continue;

			dx = NEMOMOTE_POSITION_X(mote, j) - NEMOMOTE_POSITION_X(mote, i);
			dy = NEMOMOTE_POSITION_Y(mote, j) - NEMOMOTE_POSITION_Y(mote, i);

			cd = NEMOMOTE_MASS(mote, i) + NEMOMOTE_MASS(mote, j);

			dsq = dx * dx + dy * dy;
			if (dsq <= cd * cd && dsq > 0.0f) {
				factor = 1 / sqrtf(dsq);
				dx *= factor;
				dy *= factor;
				n1 = dx * NEMOMOTE_VELOCITY_X(mote, i) + dy * NEMOMOTE_VELOCITY_Y(mote, i);
				n2 = dx * NEMOMOTE_VELOCITY_X(mote, j) + dy * NEMOMOTE_VELOCITY_Y(mote, j);
				rn = n1 - n2;
				if (rn > 0.0f) {
					m1 = NEMOMOTE_MASS(mote, i);
					m2 = NEMOMOTE_MASS(mote, j);

					factor = ((1 + bounce) * rn) / (m1 + m2);

					f1 = factor * m2;
					f2 = -factor * m1;

					NEMOMOTE_VELOCITY_X(mote, i) -= f1 * dx;
					NEMOMOTE_VELOCITY_Y(mote, i) -= f1 * dy;
					NEMOMOTE_VELOCITY_X(mote, j) -= f2 * dx;
					NEMOMOTE_VELOCITY_Y(mote, j) -= f2 * dy;
				}
			}
		}
	}

	return 0;
}
