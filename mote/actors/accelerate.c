#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomote.h>
#include <actors/accelerate.h>

int nemomote_accelerate_update(struct nemomote *mote, uint32_t type, double secs, double x, double y)
{
	int i;

	for (i = 0; i < mote->lcount; i++) {
		if (mote->types[i] != type)
			continue;

		NEMOMOTE_VELOCITY_X(mote, i) += x * secs;
		NEMOMOTE_VELOCITY_Y(mote, i) += y * secs;
	}

	return 0;
}
