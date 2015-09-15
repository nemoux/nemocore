#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemometro.h>

int nemometro_cube_intersect(float *cube, float *o, float *d, float *mint, float *maxt)
{
	float nmin[3], nmax[3];
	float tmin, tmax;
	float tx1 = (cube[0] - o[0]) / d[0];
	float tx2 = (cube[1] - o[0]) / d[0];
	int plane = NEMO_METRO_NONE_PLANE;

	if (tx1 < tx2) {
		tmin = tx1;
		tmax = tx2;

		nmin[0] = -1.0f;
		nmin[1] = 0.0f;
		nmin[2] = 0.0f;
		nmax[0] = 1.0f;
		nmax[1] = 0.0f;
		nmax[2] = 0.0f;
	} else {
		tmin = tx2;
		tmax = tx1;

		nmin[0] = 1.0f;
		nmin[1] = 0.0f;
		nmin[2] = 0.0f;
		nmax[0] = -1.0f;
		nmax[1] = 0.0f;
		nmax[2] = 0.0f;
	}

	if (tmin > tmax)
		return NEMO_METRO_NONE_PLANE;

	float ty1 = (cube[2] - o[1]) / d[1];
	float ty2 = (cube[3] - o[1]) / d[1];

	if (ty1 < ty2) {
		if (ty1 > tmin) {
			tmin = ty1;

			nmin[0] = 0.0f;
			nmin[1] = -1.0f;
			nmin[2] = 0.0f;
		}
		if (ty2 < tmax) {
			tmax = ty2;

			nmax[0] = 0.0f;
			nmax[1] = 1.0f;
			nmax[2] = 0.0f;
		}
	} else {
		if (ty2 > tmin) {
			tmin = ty2;

			nmin[0] = 0.0f;
			nmin[1] = 1.0f;
			nmin[2] = 0.0f;
		}
		if (ty1 < tmax) {
			tmax = ty1;

			nmax[0] = 0.0f;
			nmax[1] = -1.0f;
			nmax[2] = 0.0f;
		}
	}

	if (tmin > tmax)
		return NEMO_METRO_NONE_PLANE;

	float tz1 = (cube[4] - o[2]) / d[2];
	float tz2 = (cube[5] - o[2]) / d[2];

	if (tz1 < tz2) {
		if (tz1 > tmin) {
			tmin = tz1;

			nmin[0] = 0.0f;
			nmin[1] = 0.0f;
			nmin[2] = -1.0f;
		}
		if (tz2 < tmax) {
			tmax = tz2;

			nmax[0] = 0.0f;
			nmax[1] = 0.0f;
			nmax[2] = 1.0f;
		}
	} else {
		if (tz2 > tmin) {
			tmin = tz2;

			nmin[0] = 0.0f;
			nmin[1] = 0.0f;
			nmin[2] = 1.0f;
		}
		if (tz1 < tmax) {
			tmax = tz1;

			nmax[0] = 0.0f;
			nmax[1] = 0.0f;
			nmax[2] = -1.0f;
		}
	}

	if (tmin > tmax)
		return NEMO_METRO_NONE_PLANE;

	if (tmin < 0.0f && tmax > 0.0f)
		tmin = 0.0f;

	if (tmin >= 0.0f) {
		if (nmin[0] == 1.0f)
			plane = NEMO_METRO_RIGHT_PLANE;
		else if (nmin[0] == -1.0f)
			plane = NEMO_METRO_LEFT_PLANE;
		else if (nmin[1] == 1.0f)
			plane = NEMO_METRO_TOP_PLANE;
		else if (nmin[1] == -1.0f)
			plane = NEMO_METRO_BOTTOM_PLANE;
		else if (nmin[2] == 1.0f)
			plane = NEMO_METRO_FRONT_PLANE;
		else if (nmin[2] == -1.0f)
			plane = NEMO_METRO_BACK_PLANE;

		*mint = tmin;
		*maxt = tmax;
	}

	return plane;
}
