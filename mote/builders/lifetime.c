#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomote.h>
#include <builders/lifetime.h>

int nemomote_lifetime_update(struct nemomote *mote, double max, double min)
{
	int i;

	for (i = mote->lcount; i < mote->lcount + mote->rcount; i++) {
		NEMOMOTE_LIFETIME(mote, i) = min + ((double)rand() / RAND_MAX) * (max - min);
	}

	return 0;
}

int nemomote_lifetime_set(struct nemomote *mote, int base, int count, double max, double min)
{
	int i;

	for (i = base; i < base + count; i++) {
		NEMOMOTE_LIFETIME(mote, i) = min + ((double)rand() / RAND_MAX) * (max - min);
	}

	return 0;
}
