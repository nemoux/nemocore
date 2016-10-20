#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemopoly.h>

static inline void nemopoly_unproject(struct nemomatrix *projection, int32_t width, int32_t height, struct nemomatrix *modelview, float x, float y, float z, float *out)
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

	nemomatrix_transform_vector(&inverse, &in);

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

int nemopoly_intersect_lines(float x00, float y00, float x01, float y01, float x10, float y10, float x11, float y11)
{
	float sx = x01 - x00;
	float sy = y01 - y00;
	float dx = x11 - x10;
	float dy = y11 - y10;
	float d = sx * dy - sy * dx;
	float cx, cy;
	float t;

	if (d == 0.0f)
		return 0;

	cx = x10 - x00;
	cy = y10 - y00;

	t = (cx * dy - cy * dx) / d;
	if (t < 0.0f || t > 1.0f)
		return 0;

	t = (cx * sy - cy * sx) / d;
	if (t < 0.0f || t > 1.0f)
		return 0;

	return 1;
}

int nemopoly_intersect_boxes(float *b0, float *b1)
{
	float c0[2], c1[2];
	float dx, dy;
	float ax, ay;
	float l, l0, l1;

	c0[0] = (b0[2*0+0] + b0[2*1+0] + b0[2*2+0] + b0[2*3+0]) / 4.0f;
	c0[1] = (b0[2*0+1] + b0[2*1+1] + b0[2*2+1] + b0[2*3+1]) / 4.0f;
	c1[0] = (b1[2*0+0] + b1[2*1+0] + b1[2*2+0] + b1[2*3+0]) / 4.0f;
	c1[1] = (b1[2*0+1] + b1[2*1+1] + b1[2*2+1] + b1[2*3+1]) / 4.0f;

	dx = c0[0] - c1[0];
	dy = c0[1] - c1[1];

	ax = b0[2*1+0] - b0[2*0+0];
	ay = b0[2*1+1] - b0[2*0+1];
	l0 = sqrtf(ax * ax + ay * ay);
	ax /= l0;
	ay /= l0;
	l = fabs((ax * dx + ay * dy) * 2.0f);
	l1 = fabs((b1[2*1+0] - b1[2*0+0]) * ax + (b1[2*1+1] - b1[2*0+1]) * ay) + fabs((b1[2*2+0] - b1[2*1+0]) * ax + (b1[2*2+1] - b1[2*1+1]) * ay);
	if (l > l0 + l1)
		return 0;

	ax = b0[2*2+0] - b0[2*1+0];
	ay = b0[2*2+1] - b0[2*1+1];
	l0 = sqrtf(ax * ax + ay * ay);
	ax /= l0;
	ay /= l0;
	l = fabs((ax * dx + ay * dy) * 2.0f);
	l1 = fabs((b1[2*1+0] - b1[2*0+0]) * ax + (b1[2*1+1] - b1[2*0+1]) * ay) + fabs((b1[2*2+0] - b1[2*1+0]) * ax + (b1[2*2+1] - b1[2*1+1]) * ay);
	if (l > l0 + l1)
		return 0;

	ax = b1[2*1+0] - b1[2*0+0];
	ay = b1[2*1+1] - b1[2*0+1];
	l0 = sqrtf(ax * ax + ay * ay);
	ax /= l0;
	ay /= l0;
	l = fabs((ax * dx + ay * dy) * 2.0f);
	l1 = fabs((b0[2*1+0] - b0[2*0+0]) * ax + (b0[2*1+1] - b0[2*0+1]) * ay) + fabs((b0[2*2+0] - b0[2*1+0]) * ax + (b0[2*2+1] - b0[2*1+1]) * ay);
	if (l > l0 + l1)
		return 0;

	ax = b1[2*2+0] - b1[2*1+0];
	ay = b1[2*2+1] - b1[2*1+1];
	l0 = sqrtf(ax * ax + ay * ay);
	ax /= l0;
	ay /= l0;
	l = fabs((ax * dx + ay * dy) * 2.0f);
	l1 = fabs((b0[2*1+0] - b0[2*0+0]) * ax + (b0[2*1+1] - b0[2*0+1]) * ay) + fabs((b0[2*2+0] - b0[2*1+0]) * ax + (b0[2*2+1] - b0[2*1+1]) * ay);
	if (l > l0 + l1)
		return 0;

	return 1;
}

int nemopoly_intersect_ray_triangle(float *v0, float *v1, float *v2, float *o, float *d, float *t, float *u, float *v)
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

int nemopoly_intersect_ray_cube(float *cube, float *o, float *d, float *mint, float *maxt)
{
	float nmin[3], nmax[3];
	float tmin, tmax;
	float tx1 = (cube[0] - o[0]) / d[0];
	float tx2 = (cube[1] - o[0]) / d[0];
	int plane = NEMOPOLY_NONE_PLANE;

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
		return NEMOPOLY_NONE_PLANE;

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
		return NEMOPOLY_NONE_PLANE;

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
		return NEMOPOLY_NONE_PLANE;

	if (tmin < 0.0f && tmax > 0.0f)
		tmin = 0.0f;

	if (tmin >= 0.0f) {
		if (nmin[0] == 1.0f)
			plane = NEMOPOLY_RIGHT_PLANE;
		else if (nmin[0] == -1.0f)
			plane = NEMOPOLY_LEFT_PLANE;
		else if (nmin[1] == 1.0f)
			plane = NEMOPOLY_TOP_PLANE;
		else if (nmin[1] == -1.0f)
			plane = NEMOPOLY_BOTTOM_PLANE;
		else if (nmin[2] == 1.0f)
			plane = NEMOPOLY_FRONT_PLANE;
		else if (nmin[2] == -1.0f)
			plane = NEMOPOLY_BACK_PLANE;

		*mint = tmin;
		*maxt = tmax;
	}

	return plane;
}

int nemopoly_pick_triangle(struct nemomatrix *projection, int32_t width, int32_t height, struct nemomatrix *modelview, float *v0, float *v1, float *v2, float x, float y, float *t, float *u, float *v)
{
	float near[3], far[3];
	float rayorg[3];
	float rayvec[3];
	float raylen;

	nemopoly_unproject(projection, width, height, modelview, x, y, -1.0f, near);
	nemopoly_unproject(projection, width, height, modelview, x, y, 1.0f, far);

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

	return nemopoly_intersect_ray_triangle(v0, v1, v2, rayorg, rayvec, t, u, v);
}

int nemopoly_pick_cube(struct nemomatrix *projection, int32_t width, int32_t height, struct nemomatrix *modelview, float *boundingbox, float x, float y, float *mint, float *maxt)
{
	float near[3], far[3];
	float rayorg[3];
	float rayvec[3];
	float raylen;

	nemopoly_unproject(projection, width, height, modelview, x, y, -1.0f, near);
	nemopoly_unproject(projection, width, height, modelview, x, y, 1.0f, far);

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

	return nemopoly_intersect_ray_cube(boundingbox, rayorg, rayvec, mint, maxt);
}
