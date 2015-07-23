#ifndef	__NEMO_EASE_H__
#define	__NEMO_EASE_H__

#include <stdint.h>
#include <math.h>

#define	NEMOEASE_LINEAR_TYPE						(0)
#define	NEMOEASE_QUADRATIC_IN_TYPE			(1)
#define	NEMOEASE_QUADRATIC_OUT_TYPE			(2)
#define	NEMOEASE_QUADRATIC_INOUT_TYPE		(3)
#define	NEMOEASE_CUBIC_IN_TYPE					(4)
#define	NEMOEASE_CUBIC_OUT_TYPE					(5)
#define	NEMOEASE_CUBIC_INOUT_TYPE				(6)
#define	NEMOEASE_QUARTIC_IN_TYPE				(7)
#define	NEMOEASE_QUARTIC_OUT_TYPE				(8)
#define	NEMOEASE_QUARTIC_INOUT_TYPE			(9)
#define	NEMOEASE_QUINTIC_IN_TYPE				(10)
#define	NEMOEASE_QUINTIC_OUT_TYPE				(11)
#define	NEMOEASE_QUINTIC_INOUT_TYPE			(12)
#define	NEMOEASE_SINUSOIDAL_IN_TYPE			(13)
#define	NEMOEASE_SINUSOIDAL_OUT_TYPE		(14)
#define	NEMOEASE_SINUSOIDAL_INOUT_TYPE	(15)
#define	NEMOEASE_EXPONENTIAL_IN_TYPE		(16)
#define	NEMOEASE_EXPONENTIAL_OUT_TYPE		(17)
#define	NEMOEASE_EXPONENTIAL_INOUT_TYPE	(18)
#define	NEMOEASE_CIRCULAR_IN_TYPE				(19)
#define	NEMOEASE_CIRCULAR_OUT_TYPE			(20)
#define	NEMOEASE_CIRCULAR_INOUT_TYPE		(21)

struct nemoease;

typedef double (*nemoease_progress_t)(double t, double d);

struct nemoease {
	nemoease_progress_t dispatch;

	double sx, sy;
	double ex, ey;

	double ax, ay;
	double bx, by;
	double cx, cy;
};

static inline double nemoease_linear_function(double t, double d)
{
	return 1.0f * t / d + 0.0f;
}

static inline double nemoease_quadratic_in_function(double t, double d)
{
	t /= d;
	return 1.0f * t * t + 0.0f;
}

static inline double nemoease_quadratic_out_function(double t, double d)
{
	t /= d;
	return -1.0f * t * (t - 2) + 0.0f;
}

static inline double nemoease_quadratic_inout_function(double t, double d)
{
	t /= d / 2;
	if (t < 1)
		return 1.0f / 2 * t * t + 0.0f;
	t--;
	return -1.0f / 2 * (t * (t - 2) - 1) + 0.0f;
}

static inline double nemoease_cubic_in_function(double t, double d)
{
	t /= d;
	return 1.0f * t * t * t + 0.0f;
}

static inline double nemoease_cubic_out_function(double t, double d)
{
	t /= d;
	t--;
	return 1.0f * (t * t * t + 1) + 0.0f;
}

static inline double nemoease_cubic_inout_function(double t, double d)
{
	t /= d / 2;
	if (t < 1)
		return 1.0f / 2 * t * t * t + 0.0f;
	t -= 2;
	return 1.0f / 2 * (t * t * t + 2) + 0.0f;
}

static inline double nemoease_quartic_in_function(double t, double d)
{
	t /= d;
	return 1.0f * t * t * t * t + 0.0f;
}

static inline double nemoease_quartic_out_function(double t, double d)
{
	t /= d;
	t--;
	return -1.0f * (t * t * t * t - 1) + 0.0f;
}

static inline double nemoease_quartic_inout_function(double t, double d)
{
	t /= d / 2;
	if (t < 1)
		return 1.0f / 2 * t * t * t * t + 0.0f;
	t -= 2;
	return -1.0f / 2 * (t * t * t * t - 2) + 0.0f;
}

static inline double nemoease_quintic_in_function(double t, double d)
{
	t /= d;
	return 1.0f * t * t * t * t * t + 0.0f;
}

static inline double nemoease_quintic_out_function(double t, double d)
{
	t /= d;
	t--;
	return 1.0f * (t * t * t * t * t + 1) + 0.0f;
}

static inline double nemoease_quintic_inout_function(double t, double d)
{
	t /= d / 2;
	if (t < 1)
		return 1.0f / 2 * t * t * t * t * t + 0.0f;
	t -= 2;
	return 1.0f / 2 * (t * t * t * t * t + 2) + 0.0f;
}

static inline double nemoease_sinusoidal_in_function(double t, double d)
{
	return -1.0f * cos(t / d * (M_PI / 2)) + 1.0f + 0.0f;
}

static inline double nemoease_sinusoidal_out_function(double t, double d)
{
	return 1.0f * sin(t / d * (M_PI / 2)) + 0.0f;
}

static inline double nemoease_sinusoidal_inout_function(double t, double d)
{
	return -1.0f / 2 * (cos(M_PI * t / d) - 1) + 0.0f;
}

static inline double nemoease_exponential_in_function(double t, double d)
{
	return 1.0f * pow(2, 10 * (t / d - 1)) + 0.0f;
}

static inline double nemoease_exponential_out_function(double t, double d)
{
	return 1.0f * (-pow(2, -10 * (t / d)) + 1) + 0.0f;
}

static inline double nemoease_exponential_inout_function(double t, double d)
{
	t /= d / 2;
	if (t < 1)
		return 1.0f / 2 * pow(2, 10 * (t - 1)) + 0.0f;
	t--;
	return 1.0f / 2 * (-pow(2, -10 * t) + 2) + 0.0f;
}

static inline double nemoease_circular_in_function(double t, double d)
{
	t /= d;
	return -1.0f * (sqrt(1 - t * t) - 1) + 0.0f;
}

static inline double nemoease_circular_out_function(double t, double d)
{
	t /= d;
	t--;
	return 1.0f + sqrt(1 - t * t) + 0.0f;
}

static inline double nemoease_circular_inout_function(double t, double d)
{
	t /= d / 2;
	if (t < 1)
		return -1.0f / 2 * (sqrt(1 - t * t) - 1) + 0.0f;
	t -= 2;
	return 1.0f / 2 * (sqrt(1 - t * t) + 1) + 0.0f;
}

static inline double nemoease_get_x_bezier(struct nemoease *ease, double progress)
{
	ease->cx = 3 * ease->sx;
	ease->bx = 3 * (ease->ex - ease->sx) - ease->cx;
	ease->ax = 1 - ease->cx - ease->bx;

	return progress * (ease->cx + progress * (ease->bx + progress * ease->ax));
}

static inline double nemoease_get_y_bezier(struct nemoease *ease, double progress)
{
	ease->cy = 3 * ease->sy;
	ease->by = 3 * (ease->ey - ease->sy) - ease->cy;
	ease->ay = 1 - ease->cy - ease->by;

	return progress * (ease->cy + progress * (ease->by + progress * ease->ay));
}

static inline double nemoease_get_x_offset(struct nemoease *ease, double progress)
{
	double x = progress;
	double z;
	int i;

	for (i = 1; i < 14; i++) {
		z = nemoease_get_x_bezier(ease, x) - progress;

		if (fabs(z) < 1e-3)
			break;

		x -= z / (ease->cx + x * (2 * ease->bx + 3 * ease->ax * x));
	}

	return x;
}

static inline void nemoease_set(struct nemoease *ease, uint32_t type)
{
	static nemoease_progress_t functions[] = {
		nemoease_linear_function,
		nemoease_quadratic_in_function,
		nemoease_quadratic_out_function,
		nemoease_quadratic_inout_function,
		nemoease_cubic_in_function,
		nemoease_cubic_out_function,
		nemoease_cubic_inout_function,
		nemoease_quartic_in_function,
		nemoease_quartic_out_function,
		nemoease_quartic_inout_function,
		nemoease_quintic_in_function,
		nemoease_quintic_out_function,
		nemoease_quintic_inout_function,
		nemoease_sinusoidal_in_function,
		nemoease_sinusoidal_out_function,
		nemoease_sinusoidal_inout_function,
		nemoease_exponential_in_function,
		nemoease_exponential_out_function,
		nemoease_exponential_inout_function,
		nemoease_circular_in_function,
		nemoease_circular_out_function,
		nemoease_circular_inout_function
	};

	ease->dispatch = functions[type];
}

static inline void nemoease_set_cubic(struct nemoease *ease, double sx, double sy, double ex, double ey)
{
	ease->sx = sx;
	ease->sy = sy;
	ease->ex = ex;
	ease->ey = ey;

	ease->dispatch = NULL;
}

static inline double nemoease_get(struct nemoease *ease, double elapsed, double duration)
{
	if (ease->dispatch != NULL)
		return ease->dispatch(elapsed, duration);

	return nemoease_get_y_bezier(ease, nemoease_get_x_offset(ease, elapsed / duration));
}

#endif
