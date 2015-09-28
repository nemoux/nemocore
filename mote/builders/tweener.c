#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomote.h>
#include <builders/tweener.h>

int nemomote_tweener_update(struct nemomote *mote, uint32_t type, double x, double y, double s)
{
	int i;

	for (i = mote->lcount; i < mote->lcount + mote->rcount; i++) {
		NEMOMOTE_TWEEN_SX(mote, i) = NEMOMOTE_POSITION_X(mote, i);
		NEMOMOTE_TWEEN_SY(mote, i) = NEMOMOTE_POSITION_Y(mote, i);

		NEMOMOTE_TWEEN_DX(mote, i) = x;
		NEMOMOTE_TWEEN_DY(mote, i) = y;

		NEMOMOTE_TWEEN_DT(mote, i) = s;
		NEMOMOTE_TWEEN_RT(mote, i) = s;
	}

	return 0;
}

int nemomote_tweener_set(struct nemomote *mote, uint32_t type, double x, double y, double s)
{
	int i;

	for (i = 0; i < mote->lcount; i++) {
		if (mote->types[i] == type) {
			NEMOMOTE_TWEEN_SX(mote, i) = NEMOMOTE_POSITION_X(mote, i);
			NEMOMOTE_TWEEN_SY(mote, i) = NEMOMOTE_POSITION_Y(mote, i);

			NEMOMOTE_TWEEN_DX(mote, i) = x;
			NEMOMOTE_TWEEN_DY(mote, i) = y;

			NEMOMOTE_TWEEN_DT(mote, i) = s;
			NEMOMOTE_TWEEN_RT(mote, i) = s;
		}
	}

	return 0;
}

int nemomote_tweener_set_one(struct nemomote *mote, int index, double x, double y, double s)
{
	NEMOMOTE_TWEEN_SX(mote, index) = NEMOMOTE_POSITION_X(mote, index);
	NEMOMOTE_TWEEN_SY(mote, index) = NEMOMOTE_POSITION_Y(mote, index);

	NEMOMOTE_TWEEN_DX(mote, index) = x;
	NEMOMOTE_TWEEN_DY(mote, index) = y;

	NEMOMOTE_TWEEN_DT(mote, index) = s;
	NEMOMOTE_TWEEN_RT(mote, index) = s;

	return 0;
}
