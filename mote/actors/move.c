#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomote.h>
#include <actors/move.h>

int nemomote_move_update(struct nemomote *mote, double secs)
{
	int i;

	for (i = 0; i < mote->lcount; i++) {
		NEMOMOTE_POSITION_X(mote, i) += NEMOMOTE_VELOCITY_X(mote, i) * secs;
		NEMOMOTE_POSITION_Y(mote, i) += NEMOMOTE_VELOCITY_Y(mote, i) * secs;
		NEMOMOTE_POSITION_Z(mote, i) += NEMOMOTE_VELOCITY_Z(mote, i) * secs;
	}

	return 0;
}
