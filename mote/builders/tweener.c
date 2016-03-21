#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomote.h>
#include <builders/tweener.h>

int nemomote_tweener_update_position(struct nemomote *mote, uint32_t type, double x, double y)
{
	int i;

	for (i = mote->lcount; i < mote->lcount + mote->rcount; i++) {
		NEMOMOTE_TWEEN_SX(mote, i) = NEMOMOTE_POSITION_X(mote, i);
		NEMOMOTE_TWEEN_SY(mote, i) = NEMOMOTE_POSITION_Y(mote, i);

		NEMOMOTE_TWEEN_DX(mote, i) = x;
		NEMOMOTE_TWEEN_DY(mote, i) = y;
	}

	return 0;
}

int nemomote_tweener_update_color(struct nemomote *mote, uint32_t type, double rmax, double rmin, double gmax, double gmin, double bmax, double bmin, double amax, double amin)
{
	int i;

	for (i = mote->lcount; i < mote->lcount + mote->rcount; i++) {
		NEMOMOTE_TWEEN_SR(mote, i) = NEMOMOTE_COLOR_R(mote, i);
		NEMOMOTE_TWEEN_DR(mote, i) = ((double)rand() / RAND_MAX) * (rmax - rmin) + rmin;
		NEMOMOTE_TWEEN_SG(mote, i) = NEMOMOTE_COLOR_G(mote, i);
		NEMOMOTE_TWEEN_DG(mote, i) = ((double)rand() / RAND_MAX) * (gmax - gmin) + gmin;
		NEMOMOTE_TWEEN_SB(mote, i) = NEMOMOTE_COLOR_B(mote, i);
		NEMOMOTE_TWEEN_DB(mote, i) = ((double)rand() / RAND_MAX) * (bmax - bmin) + bmin;
		NEMOMOTE_TWEEN_SA(mote, i) = NEMOMOTE_COLOR_A(mote, i);
		NEMOMOTE_TWEEN_DA(mote, i) = ((double)rand() / RAND_MAX) * (amax - amin) + amin;
	}

	return 0;
}

int nemomote_tweener_update_mass(struct nemomote *mote, uint32_t type, double max, double min)
{
	int i;

	for (i = mote->lcount; i < mote->lcount + mote->rcount; i++) {
		NEMOMOTE_TWEEN_SM(mote, i) = NEMOMOTE_MASS(mote, i);
		NEMOMOTE_TWEEN_DM(mote, i) = ((double)rand() / RAND_MAX) * (max - min) + min;
	}

	return 0;
}

int nemomote_tweener_update_duration(struct nemomote *mote, uint32_t type, double max, double min)
{
	int i;

	for (i = mote->lcount; i < mote->lcount + mote->rcount; i++) {
		NEMOMOTE_TWEEN_DT(mote, i) = ((double)rand() / RAND_MAX) * (max - min) + min;
		NEMOMOTE_TWEEN_RT(mote, i) = NEMOMOTE_TWEEN_DT(mote, i);
	}

	return 0;
}

int nemomote_tweener_set_position(struct nemomote *mote, uint32_t type, double x, double y)
{
	int i;

	for (i = 0; i < mote->lcount; i++) {
		if (mote->types[i] == type) {
			NEMOMOTE_TWEEN_SX(mote, i) = NEMOMOTE_POSITION_X(mote, i);
			NEMOMOTE_TWEEN_SY(mote, i) = NEMOMOTE_POSITION_Y(mote, i);

			NEMOMOTE_TWEEN_DX(mote, i) = x;
			NEMOMOTE_TWEEN_DY(mote, i) = y;
		}
	}

	return 0;
}

int nemomote_tweener_set_color(struct nemomote *mote, uint32_t type, double rmax, double rmin, double gmax, double gmin, double bmax, double bmin, double amax, double amin)
{
	int i;

	for (i = 0; i < mote->lcount; i++) {
		if (mote->types[i] == type) {
			NEMOMOTE_TWEEN_SR(mote, i) = NEMOMOTE_COLOR_R(mote, i);
			NEMOMOTE_TWEEN_DR(mote, i) = ((double)rand() / RAND_MAX) * (rmax - rmin) + rmin;
			NEMOMOTE_TWEEN_SG(mote, i) = NEMOMOTE_COLOR_G(mote, i);
			NEMOMOTE_TWEEN_DG(mote, i) = ((double)rand() / RAND_MAX) * (gmax - gmin) + gmin;
			NEMOMOTE_TWEEN_SB(mote, i) = NEMOMOTE_COLOR_B(mote, i);
			NEMOMOTE_TWEEN_DB(mote, i) = ((double)rand() / RAND_MAX) * (bmax - bmin) + bmin;
			NEMOMOTE_TWEEN_SA(mote, i) = NEMOMOTE_COLOR_A(mote, i);
			NEMOMOTE_TWEEN_DA(mote, i) = ((double)rand() / RAND_MAX) * (amax - amin) + amin;
		}
	}

	return 0;
}

int nemomote_tweener_set_mass(struct nemomote *mote, uint32_t type, double max, double min)
{
	int i;

	for (i = 0; i < mote->lcount; i++) {
		if (mote->types[i] == type) {
			NEMOMOTE_TWEEN_SM(mote, i) = NEMOMOTE_MASS(mote, i);
			NEMOMOTE_TWEEN_DM(mote, i) = ((double)rand() / RAND_MAX) * (max - min) + min;
		}
	}

	return 0;
}

int nemomote_tweener_set_duration(struct nemomote *mote, uint32_t type, double max, double min)
{
	int i;

	for (i = 0; i < mote->lcount; i++) {
		if (mote->types[i] == type) {
			NEMOMOTE_TWEEN_DT(mote, i) = ((double)rand() / RAND_MAX) * (max - min) + min;
			NEMOMOTE_TWEEN_RT(mote, i) = NEMOMOTE_TWEEN_DT(mote, i);
		}
	}

	return 0;
}

int nemomote_tweener_set_one_position(struct nemomote *mote, int index, double x, double y)
{
	NEMOMOTE_TWEEN_SX(mote, index) = NEMOMOTE_POSITION_X(mote, index);
	NEMOMOTE_TWEEN_SY(mote, index) = NEMOMOTE_POSITION_Y(mote, index);

	NEMOMOTE_TWEEN_DX(mote, index) = x;
	NEMOMOTE_TWEEN_DY(mote, index) = y;

	return 0;
}

int nemomote_tweener_set_one_color(struct nemomote *mote, int index, double rmax, double rmin, double gmax, double gmin, double bmax, double bmin, double amax, double amin)
{
	NEMOMOTE_TWEEN_SR(mote, index) = NEMOMOTE_COLOR_R(mote, index);
	NEMOMOTE_TWEEN_DR(mote, index) = ((double)rand() / RAND_MAX) * (rmax - rmin) + rmin;
	NEMOMOTE_TWEEN_SG(mote, index) = NEMOMOTE_COLOR_G(mote, index);
	NEMOMOTE_TWEEN_DG(mote, index) = ((double)rand() / RAND_MAX) * (gmax - gmin) + gmin;
	NEMOMOTE_TWEEN_SB(mote, index) = NEMOMOTE_COLOR_B(mote, index);
	NEMOMOTE_TWEEN_DB(mote, index) = ((double)rand() / RAND_MAX) * (bmax - bmin) + bmin;
	NEMOMOTE_TWEEN_SA(mote, index) = NEMOMOTE_COLOR_A(mote, index);
	NEMOMOTE_TWEEN_DA(mote, index) = ((double)rand() / RAND_MAX) * (amax - amin) + amin;

	return 0;
}

int nemomote_tweener_set_one_mass(struct nemomote *mote, int index, double max, double min)
{
	NEMOMOTE_TWEEN_SM(mote, index) = NEMOMOTE_MASS(mote, index);
	NEMOMOTE_TWEEN_DM(mote, index) = ((double)rand() / RAND_MAX) * (max - min) + min;

	return 0;
}

int nemomote_tweener_set_one_duration(struct nemomote *mote, int index, double max, double min)
{
	NEMOMOTE_TWEEN_DT(mote, index) = ((double)rand() / RAND_MAX) * (max - min) + min;
	NEMOMOTE_TWEEN_RT(mote, index) = NEMOMOTE_TWEEN_DT(mote, index);

	return 0;
}
