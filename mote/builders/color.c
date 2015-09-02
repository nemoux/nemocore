#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomote.h>
#include <builders/color.h>

int nemomote_colorbuilder_update(struct nemomote *mote, double secs, double rmax, double rmin, double gmax, double gmin, double bmax, double bmin, double amax, double amin)
{
	int i;

	for (i = mote->lcount; i < mote->lcount + mote->rcount; i++) {
		NEMOMOTE_COLOR_R(mote, i) = rmin + ((double)rand() / RAND_MAX) * (rmax - rmin);
		NEMOMOTE_COLOR_G(mote, i) = gmin + ((double)rand() / RAND_MAX) * (gmax - gmin);
		NEMOMOTE_COLOR_B(mote, i) = bmin + ((double)rand() / RAND_MAX) * (bmax - bmin);
		NEMOMOTE_COLOR_A(mote, i) = amin + ((double)rand() / RAND_MAX) * (amax - amin);
	}

	return 0;
}
