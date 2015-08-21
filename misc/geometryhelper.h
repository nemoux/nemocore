#ifndef	__GEOMETRY_HELPER_H__
#define	__GEOMETRY_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <math.h>

extern double point_get_angle_on_line(double x1, double y1, double x2, double y2, double x3, double y3);

static inline double point_get_distance(double x0, double y0, double x1, double y1)
{
	double dx = x1 - x0;
	double dy = y1 - y0;

	return sqrtf(dx * dx + dy * dy);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
