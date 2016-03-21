#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomote.h>
#include <actors/explosion.h>
#include <nemomisc.h>

int nemomote_explosion_update(struct nemomote *mote, uint32_t type, double secs, double maxx, double minx, double maxy, double miny)
{
	int i;

	for (i = 0; i < mote->lcount; i++) {
		if (mote->types[i] != type)
			continue;

		NEMOMOTE_VELOCITY_X(mote, i) += random_get_double(minx, maxx);
		NEMOMOTE_VELOCITY_Y(mote, i) += random_get_double(miny, maxy);
	}

	return 0;
}
