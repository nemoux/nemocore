#ifndef	__NEMO_METRO_H__
#define	__NEMO_METRO_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <math.h>

#include <nemomatrix.h>

typedef enum {
	NEMO_METRO_NONE_PLANE = 0,
	NEMO_METRO_FRONT_PLANE = 1,
	NEMO_METRO_BACK_PLANE = 2,
	NEMO_METRO_LEFT_PLANE = 3,
	NEMO_METRO_RIGHT_PLANE = 4,
	NEMO_METRO_TOP_PLANE = 5,
	NEMO_METRO_BOTTOM_PLANE = 6,
	NEMO_METRO_LAST_PLANE
} NemoMetroPlaneDirection;

extern int nemometro_intersect_triangle(float *v0, float *v1, float *v2, float *o, float *d, float *t, float *u, float *v);
extern int nemometro_pick_triangle(struct nemomatrix *projection, int32_t width, int32_t height, struct nemomatrix *modelview, float *v0, float *v1, float *v2, float x, float y, float *t, float *u, float *v);
extern int nemometro_intersect_cube(float *cube, float *o, float *d, float *mint, float *maxt);
extern int nemometro_pick_cube(struct nemomatrix *projection, int32_t width, int32_t height, struct nemomatrix *modelview, float *boundingbox, float x, float y, float *mint, float *maxt);

static inline double nemometro_get_point_angle_on_line(double x1, double y1, double x2, double y2, double x3, double y3)
{
	double px = x2 - x1, py = y2 - y1, dab = px * px + py * py;
	double k = ((x3 - x1) * px + (y3 - y1) * py) / dab;
	double x4 = x1 + k * px;
	double y4 = y1 + k * py;

	return atan2(y3 - y4, x3 - x4);
}

static inline double nemometro_get_point_distance(double x0, double y0, double x1, double y1)
{
	double dx = x1 - x0;
	double dy = y1 - y0;

	return sqrtf(dx * dx + dy * dy);
}

static inline int nemometro_intersect_lines(float x00, float y00, float x01, float y01, float x10, float y10, float x11, float y11)
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

static inline int nemometro_intersect_boxes(float *b0, float *b1)
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

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
