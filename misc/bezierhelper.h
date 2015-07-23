#ifndef	__BEZIER_HELPER_H__
#define	__BEZIER_HELPER_H__

static inline double cubicbezier_point(double t, double p0, double p1, double p2, double p3)
{
	return p0 * (1.0f - t) * (1.0f - t) * (1.0f - t) +
		3.0f * p1 * (1.0f - t) * (1.0f - t) * t +
		3.0f * p2 * (1.0f - t) * t * t +
		p3 * t * t * t;
}

extern double cubicbezier_length(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3, int steps);

#endif
