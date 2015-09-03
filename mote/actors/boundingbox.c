#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomote.h>
#include <actors/boundingbox.h>

int nemomote_boundingbox_update(struct nemomote *mote, uint32_t type, double secs, struct nemozone *zone, double bounce)
{
	int i;

	for (i = 0; i < mote->lcount; i++) {
		if (mote->types[i] != type)
			continue;

		if (NEMOMOTE_VELOCITY_X(mote, i) > 0.0f && NEMOMOTE_POSITION_X(mote, i) >= zone->right) {
			NEMOMOTE_VELOCITY_X(mote, i) = -NEMOMOTE_VELOCITY_X(mote, i) * bounce;
			NEMOMOTE_POSITION_X(mote, i) += 2 * (zone->right - NEMOMOTE_POSITION_X(mote, i));
		} else if (NEMOMOTE_VELOCITY_X(mote, i) < 0.0f && NEMOMOTE_POSITION_X(mote, i) <= zone->left) {
			NEMOMOTE_VELOCITY_X(mote, i) = -NEMOMOTE_VELOCITY_X(mote, i) * bounce;
			NEMOMOTE_POSITION_X(mote, i) += 2 * (zone->left - NEMOMOTE_POSITION_X(mote, i));
		} else if (NEMOMOTE_VELOCITY_Y(mote, i) > 0.0f && NEMOMOTE_POSITION_Y(mote, i) >= zone->bottom) {
			NEMOMOTE_VELOCITY_Y(mote, i) = -NEMOMOTE_VELOCITY_Y(mote, i) * bounce;
			NEMOMOTE_POSITION_Y(mote, i) += 2 * (zone->bottom - NEMOMOTE_POSITION_Y(mote, i));
		} else if (NEMOMOTE_VELOCITY_Y(mote, i) < 0.0f && NEMOMOTE_POSITION_Y(mote, i) <= zone->top) {
			NEMOMOTE_VELOCITY_Y(mote, i) = -NEMOMOTE_VELOCITY_Y(mote, i) * bounce;
			NEMOMOTE_POSITION_Y(mote, i) += 2 * (zone->top - NEMOMOTE_POSITION_Y(mote, i));
		}
	}

	return 0;
}
