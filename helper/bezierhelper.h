#ifndef	__BEZIER_HELPER_H__
#define	__BEZIER_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

extern double cubicbezier_point(double t, double p0, double p1, double p2, double p3);
extern double cubicbezier_length(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3, int steps);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
