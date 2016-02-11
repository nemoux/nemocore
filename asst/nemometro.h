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
extern void nemometro_unproject(struct nemomatrix *projection, int32_t width, int32_t height, struct nemomatrix *modelview, float x, float y, float z, float *out);

static inline double nemometro_point_get_angle_on_line(double x1, double y1, double x2, double y2, double x3, double y3)
{
	double px = x2 - x1, py = y2 - y1, dab = px * px + py * py;
	double k = ((x3 - x1) * px + (y3 - y1) * py) / dab;
	double x4 = x1 + k * px;
	double y4 = y1 + k * py;

	return atan2(y3 - y4, x3 - x4);
}

static inline double nemometro_point_get_distance(double x0, double y0, double x1, double y1)
{
	double dx = x1 - x0;
	double dy = y1 - y0;

	return sqrtf(dx * dx + dy * dy);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
