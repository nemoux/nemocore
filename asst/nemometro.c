#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemometro.h>

static inline void nemometro_unproject(struct nemomatrix *projection, int32_t width, int32_t height, struct nemomatrix *modelview, float x, float y, float z, float *out)
{
	struct nemomatrix matrix;
	struct nemomatrix inverse;
	struct nemovector in;

	matrix = *modelview;
	nemomatrix_multiply(&matrix, projection);

	nemomatrix_invert(&inverse, &matrix);

	in.f[0] = x * 2.0f / width - 1.0f;
	in.f[1] = y * 2.0f / height - 1.0f;
	in.f[2] = z * 2.0f - 1.0f;
	in.f[3] = 1.0f;

	nemomatrix_transform(&inverse, &in);

	if (fabsf(in.f[3]) < 1e-6) {
		out[0] = 0.0f;
		out[1] = 0.0f;
		out[2] = 0.0f;
	} else {
		out[0] = in.f[0] / in.f[3];
		out[1] = in.f[1] / in.f[3];
		out[2] = in.f[2] / in.f[3];
	}
}

int nemometro_intersect_triangle(float *v0, float *v1, float *v2, float *o, float *d, float *t, float *u, float *v)
{
	float edge1[3], edge2[3];
	float tvec[3], pvec[3], qvec[3];
	float det, inv;

	NEMOVECTOR_SUB(edge1, v1, v0);
	NEMOVECTOR_SUB(edge2, v2, v0);

	NEMOVECTOR_CROSS(pvec, d, edge2);

	det = NEMOVECTOR_DOT(edge1, pvec);
	if (fabsf(det) < 1e-6)
		return 0;
	inv = 1.0f / det;

	NEMOVECTOR_SUB(tvec, o, v0);

	*u = NEMOVECTOR_DOT(tvec, pvec) * inv;
	if (*u < 0.0f || *u > 1.0f)
		return 0;

	NEMOVECTOR_CROSS(qvec, tvec, edge1);

	*v = NEMOVECTOR_DOT(d, qvec) * inv;
	if (*v < 0.0f || *u + *v > 1.0f)
		return 0;

	*t = NEMOVECTOR_DOT(edge2, qvec) * inv;

	return 1;
}

int nemometro_pick_triangle(struct nemomatrix *projection, int32_t width, int32_t height, struct nemomatrix *modelview, float *v0, float *v1, float *v2, float x, float y, float *t, float *u, float *v)
{
	float near[3], far[3];
	float rayorg[3];
	float rayvec[3];
	float raylen;

	nemometro_unproject(projection, width, height, modelview, x, y, -1.0f, near);
	nemometro_unproject(projection, width, height, modelview, x, y, 1.0f, far);

	rayvec[0] = far[0] - near[0];
	rayvec[1] = far[1] - near[1];
	rayvec[2] = far[2] - near[2];

	raylen = sqrtf(rayvec[0] * rayvec[0] + rayvec[1] * rayvec[1] + rayvec[2] * rayvec[2]);

	rayvec[0] /= raylen;
	rayvec[1] /= raylen;
	rayvec[2] /= raylen;

	rayorg[0] = near[0];
	rayorg[1] = near[1];
	rayorg[2] = near[2];

	return nemometro_intersect_triangle(v0, v1, v2, rayorg, rayvec, t, u, v);
}

int nemometro_intersect_cube(float *cube, float *o, float *d, float *mint, float *maxt)
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

int nemometro_pick_cube(struct nemomatrix *projection, int32_t width, int32_t height, struct nemomatrix *modelview, float *boundingbox, float x, float y, float *mint, float *maxt)
{
	float near[3], far[3];
	float rayorg[3];
	float rayvec[3];
	float raylen;

	nemometro_unproject(projection, width, height, modelview, x, y, -1.0f, near);
	nemometro_unproject(projection, width, height, modelview, x, y, 1.0f, far);

	rayvec[0] = far[0] - near[0];
	rayvec[1] = far[1] - near[1];
	rayvec[2] = far[2] - near[2];

	raylen = sqrtf(rayvec[0] * rayvec[0] + rayvec[1] * rayvec[1] + rayvec[2] * rayvec[2]);

	rayvec[0] /= raylen;
	rayvec[1] /= raylen;
	rayvec[2] /= raylen;

	rayorg[0] = near[0];
	rayorg[1] = near[1];
	rayorg[2] = near[2];

	return nemometro_intersect_cube(boundingbox, rayorg, rayvec, mint, maxt);
}
