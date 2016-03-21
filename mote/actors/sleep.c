#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomote.h>
#include <actors/sleep.h>

int nemomote_sleep_update(struct nemomote *mote, uint32_t type, double secs, uint32_t dtype)
{
	int i;

	for (i = 0; i < mote->lcount; i++) {
		if (mote->types[i] != type)
			continue;

		NEMOMOTE_SLEEPTIME(mote, i) -= secs;

		if (NEMOMOTE_SLEEPTIME(mote, i) <= 0.0f) {
			mote->types[i] = dtype;
		}
	}

	return 0;
}
