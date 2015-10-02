#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomote.h>
#include <actors/tween.h>
#include <nemoease.h>

int nemomote_tween_update(struct nemomote *mote, uint32_t type, double secs, struct nemoease *ease, uint32_t dtype, uint32_t tween)
{
	double t;
	int done = 0;
	int i;

	for (i = 0; i < mote->lcount; i++) {
		if (mote->types[i] != type)
			continue;

		NEMOMOTE_TWEEN_RT(mote, i) -= secs;

		if (NEMOMOTE_TWEEN_RT(mote, i) > 0.0f) {
			t = nemoease_get(ease,
					NEMOMOTE_TWEEN_DT(mote, i) - NEMOMOTE_TWEEN_RT(mote, i),
					NEMOMOTE_TWEEN_DT(mote, i));
		} else {
			t = 1.0f;

			mote->types[i] = dtype;

			done = 1;
		}

		if (tween & NEMOMOTE_POSITION_TWEEN) {
			NEMOMOTE_POSITION_X(mote, i) = (NEMOMOTE_TWEEN_DX(mote, i) - NEMOMOTE_TWEEN_SX(mote, i)) * t + NEMOMOTE_TWEEN_SX(mote, i);
			NEMOMOTE_POSITION_Y(mote, i) = (NEMOMOTE_TWEEN_DY(mote, i) - NEMOMOTE_TWEEN_SY(mote, i)) * t + NEMOMOTE_TWEEN_SY(mote, i);
		}

		if (tween & NEMOMOTE_ALPHA_TWEEN) {
			NEMOMOTE_COLOR_A(mote, i) = (NEMOMOTE_TWEEN_DA(mote, i) - NEMOMOTE_TWEEN_SA(mote, i)) * t + NEMOMOTE_TWEEN_SA(mote, i);
		}

		if (tween & NEMOMOTE_MASS_TWEEN) {
			NEMOMOTE_MASS(mote, i) = (NEMOMOTE_TWEEN_DM(mote, i) - NEMOMOTE_TWEEN_SM(mote, i)) * t + NEMOMOTE_TWEEN_SM(mote, i);
		}
	}

	return done;
}
