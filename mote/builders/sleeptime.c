#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomote.h>
#include <builders/sleeptime.h>

int nemomote_sleeptime_update(struct nemomote *mote, double max, double min)
{
	int i;

	for (i = mote->lcount; i < mote->lcount + mote->rcount; i++) {
		NEMOMOTE_SLEEPTIME(mote, i) = min + ((double)rand() / RAND_MAX) * (max - min);
	}

	return 0;
}

int nemomote_sleeptime_set(struct nemomote *mote, uint32_t type, double max, double min)
{
	int i;

	for (i = 0; i < mote->lcount; i++) {
		if (mote->types[i] == type) {
			NEMOMOTE_SLEEPTIME(mote, i) = min + ((double)rand() / RAND_MAX) * (max - min);
		}
	}

	return 0;
}
