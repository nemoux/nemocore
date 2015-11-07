#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomote.h>
#include <builders/tweener.h>

int nemomote_tweener_update(struct nemomote *mote, uint32_t type, double x, double y, double c1[4], double c0[4], double m1, double m0, double s1, double s0)
{
	int i;

	for (i = mote->lcount; i < mote->lcount + mote->rcount; i++) {
		NEMOMOTE_TWEEN_SX(mote, i) = NEMOMOTE_POSITION_X(mote, i);
		NEMOMOTE_TWEEN_SY(mote, i) = NEMOMOTE_POSITION_Y(mote, i);

		NEMOMOTE_TWEEN_DX(mote, i) = x;
		NEMOMOTE_TWEEN_DY(mote, i) = y;

		NEMOMOTE_TWEEN_SR(mote, i) = NEMOMOTE_COLOR_R(mote, i);
		NEMOMOTE_TWEEN_DR(mote, i) = ((double)rand() / RAND_MAX) * (c1[0] - c0[0]) + c0[0];
		NEMOMOTE_TWEEN_SG(mote, i) = NEMOMOTE_COLOR_G(mote, i);
		NEMOMOTE_TWEEN_DG(mote, i) = ((double)rand() / RAND_MAX) * (c1[1] - c0[1]) + c0[1];
		NEMOMOTE_TWEEN_SB(mote, i) = NEMOMOTE_COLOR_B(mote, i);
		NEMOMOTE_TWEEN_DB(mote, i) = ((double)rand() / RAND_MAX) * (c1[2] - c0[2]) + c0[2];
		NEMOMOTE_TWEEN_SA(mote, i) = NEMOMOTE_COLOR_A(mote, i);
		NEMOMOTE_TWEEN_DA(mote, i) = ((double)rand() / RAND_MAX) * (c1[3] - c0[3]) + c0[3];

		NEMOMOTE_TWEEN_SM(mote, i) = NEMOMOTE_MASS(mote, i);
		NEMOMOTE_TWEEN_DM(mote, i) = ((double)rand() / RAND_MAX) * (m1 - m0) + m0;

		NEMOMOTE_TWEEN_DT(mote, i) = ((double)rand() / RAND_MAX) * (s1 - s0) + s0;
		NEMOMOTE_TWEEN_RT(mote, i) = NEMOMOTE_TWEEN_DT(mote, i);
	}

	return 0;
}

int nemomote_tweener_set(struct nemomote *mote, uint32_t type, double x, double y, double c1[4], double c0[4], double m1, double m0, double s1, double s0)
{
	int i;

	for (i = 0; i < mote->lcount; i++) {
		if (mote->types[i] == type) {
			NEMOMOTE_TWEEN_SX(mote, i) = NEMOMOTE_POSITION_X(mote, i);
			NEMOMOTE_TWEEN_SY(mote, i) = NEMOMOTE_POSITION_Y(mote, i);

			NEMOMOTE_TWEEN_DX(mote, i) = x;
			NEMOMOTE_TWEEN_DY(mote, i) = y;

			NEMOMOTE_TWEEN_SR(mote, i) = NEMOMOTE_COLOR_R(mote, i);
			NEMOMOTE_TWEEN_DR(mote, i) = ((double)rand() / RAND_MAX) * (c1[0] - c0[0]) + c0[0];
			NEMOMOTE_TWEEN_SG(mote, i) = NEMOMOTE_COLOR_G(mote, i);
			NEMOMOTE_TWEEN_DG(mote, i) = ((double)rand() / RAND_MAX) * (c1[1] - c0[1]) + c0[1];
			NEMOMOTE_TWEEN_SB(mote, i) = NEMOMOTE_COLOR_B(mote, i);
			NEMOMOTE_TWEEN_DB(mote, i) = ((double)rand() / RAND_MAX) * (c1[2] - c0[2]) + c0[2];
			NEMOMOTE_TWEEN_SA(mote, i) = NEMOMOTE_COLOR_A(mote, i);
			NEMOMOTE_TWEEN_DA(mote, i) = ((double)rand() / RAND_MAX) * (c1[3] - c0[3]) + c0[3];

			NEMOMOTE_TWEEN_SM(mote, i) = NEMOMOTE_MASS(mote, i);
			NEMOMOTE_TWEEN_DM(mote, i) = ((double)rand() / RAND_MAX) * (m1 - m0) + m0;

			NEMOMOTE_TWEEN_DT(mote, i) = ((double)rand() / RAND_MAX) * (s1 - s0) + s0;
			NEMOMOTE_TWEEN_RT(mote, i) = NEMOMOTE_TWEEN_DT(mote, i);
		}
	}

	return 0;
}

int nemomote_tweener_set_one(struct nemomote *mote, int index, double x, double y, double c1[4], double c0[4], double m1, double m0, double s1, double s0)
{
	NEMOMOTE_TWEEN_SX(mote, index) = NEMOMOTE_POSITION_X(mote, index);
	NEMOMOTE_TWEEN_SY(mote, index) = NEMOMOTE_POSITION_Y(mote, index);

	NEMOMOTE_TWEEN_DX(mote, index) = x;
	NEMOMOTE_TWEEN_DY(mote, index) = y;

	NEMOMOTE_TWEEN_SR(mote, index) = NEMOMOTE_COLOR_R(mote, index);
	NEMOMOTE_TWEEN_DR(mote, index) = ((double)rand() / RAND_MAX) * (c1[0] - c0[0]) + c0[0];
	NEMOMOTE_TWEEN_SG(mote, index) = NEMOMOTE_COLOR_G(mote, index);
	NEMOMOTE_TWEEN_DG(mote, index) = ((double)rand() / RAND_MAX) * (c1[1] - c0[1]) + c0[1];
	NEMOMOTE_TWEEN_SB(mote, index) = NEMOMOTE_COLOR_B(mote, index);
	NEMOMOTE_TWEEN_DB(mote, index) = ((double)rand() / RAND_MAX) * (c1[2] - c0[2]) + c0[2];
	NEMOMOTE_TWEEN_SA(mote, index) = NEMOMOTE_COLOR_A(mote, index);
	NEMOMOTE_TWEEN_DA(mote, index) = ((double)rand() / RAND_MAX) * (c1[3] - c0[3]) + c0[3];

	NEMOMOTE_TWEEN_SM(mote, index) = NEMOMOTE_MASS(mote, index);
	NEMOMOTE_TWEEN_DM(mote, index) = ((double)rand() / RAND_MAX) * (m1 - m0) + m0;

	NEMOMOTE_TWEEN_DT(mote, index) = ((double)rand() / RAND_MAX) * (s1 - s0) + s0;
	NEMOMOTE_TWEEN_RT(mote, index) = NEMOMOTE_TWEEN_DT(mote, index);

	return 0;
}
