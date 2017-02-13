#ifndef	__NEMO_EASE_H__
#define	__NEMO_EASE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>
#include <string.h>
#include <math.h>

typedef enum {
	NEMOEASE_LINEAR_TYPE = 0,
	NEMOEASE_QUADRATIC_IN_TYPE = 1,
	NEMOEASE_QUADRATIC_OUT_TYPE = 2,
	NEMOEASE_QUADRATIC_INOUT_TYPE = 3,
	NEMOEASE_CUBIC_IN_TYPE = 4,
	NEMOEASE_CUBIC_OUT_TYPE = 5,
	NEMOEASE_CUBIC_INOUT_TYPE = 6,
	NEMOEASE_QUARTIC_IN_TYPE = 7,
	NEMOEASE_QUARTIC_OUT_TYPE = 8,
	NEMOEASE_QUARTIC_INOUT_TYPE = 9,
	NEMOEASE_QUINTIC_IN_TYPE = 10,
	NEMOEASE_QUINTIC_OUT_TYPE = 11,
	NEMOEASE_QUINTIC_INOUT_TYPE = 12,
	NEMOEASE_SINUSOIDAL_IN_TYPE = 13,
	NEMOEASE_SINUSOIDAL_OUT_TYPE = 14,
	NEMOEASE_SINUSOIDAL_INOUT_TYPE = 15,
	NEMOEASE_EXPONENTIAL_IN_TYPE = 16,
	NEMOEASE_EXPONENTIAL_OUT_TYPE = 17,
	NEMOEASE_EXPONENTIAL_INOUT_TYPE = 18,
	NEMOEASE_CIRCULAR_IN_TYPE = 19,
	NEMOEASE_CIRCULAR_OUT_TYPE = 20,
	NEMOEASE_CIRCULAR_INOUT_TYPE = 21,
	NEMOEASE_BOUNCE_IN_TYPE = 22,
	NEMOEASE_BOUNCE_OUT_TYPE = 23,
	NEMOEASE_BOUNCE_INOUT_TYPE = 24,
	NEMOEASE_LAST_TYPE
} NemoEaseType;

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

static inline double nemoease_bounce_out_in(double t)
{
	float n1 = 7.5625f;
	float d1 = 2.75f;

	if (t < 1.0f / d1) {
		return n1 * t * t;
	} else if (t < 2.0f / d1) {
		t -= 1.5f / d1;
		return n1 * t * t + 0.75f;
	} else if (t < 2.5f / d1) {
		t -= 2.25f / d1;
		return n1 * t * t + 0.9375f;
	} else {
		t -= 2.625f / d1;
		return n1 * t * t + 0.984375f;
	}
}

static inline double nemoease_bounce_in_function(double t, double d)
{
	t /= d;
	return 1.0f - nemoease_bounce_out_in(1.0f - t);
}

static inline double nemoease_bounce_out_function(double t, double d)
{
	t /= d;
	return nemoease_bounce_out_in(t);
}

static inline double nemoease_bounce_inout_function(double t, double d)
{
	t /= d;
	if (t < 0.5f)
		return (1.0f - nemoease_bounce_out_in(1.0f - 2 * t)) / 2.0f;
	return (1.0f + nemoease_bounce_out_in(2.0f * t - 1.0f)) / 2.0f;
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
		nemoease_circular_inout_function,
		nemoease_bounce_in_function,
		nemoease_bounce_out_function,
		nemoease_bounce_inout_function
	};

	ease->dispatch = functions[type];
}

static inline uint32_t nemoease_get_type(const char *name)
{
	uint32_t type = NEMOEASE_LINEAR_TYPE;

	if (strcmp(name, "linear") == 0)
		type = NEMOEASE_LINEAR_TYPE;
	else if (strcmp(name, "quadratic_in") == 0)
		type = NEMOEASE_QUADRATIC_IN_TYPE;
	else if (strcmp(name, "quadratic_out") == 0)
		type = NEMOEASE_QUADRATIC_OUT_TYPE;
	else if (strcmp(name, "quadratic_inout") == 0)
		type = NEMOEASE_QUADRATIC_INOUT_TYPE;
	else if (strcmp(name, "cubic_in") == 0)
		type = NEMOEASE_CUBIC_IN_TYPE;
	else if (strcmp(name, "cubic_out") == 0)
		type = NEMOEASE_CUBIC_OUT_TYPE;
	else if (strcmp(name, "cubic_inout") == 0)
		type = NEMOEASE_CUBIC_INOUT_TYPE;
	else if (strcmp(name, "quartic_in") == 0)
		type = NEMOEASE_QUARTIC_IN_TYPE;
	else if (strcmp(name, "quartic_out") == 0)
		type = NEMOEASE_QUARTIC_OUT_TYPE;
	else if (strcmp(name, "quartic_inout") == 0)
		type = NEMOEASE_QUARTIC_INOUT_TYPE;
	else if (strcmp(name, "quintic_in") == 0)
		type = NEMOEASE_QUINTIC_IN_TYPE;
	else if (strcmp(name, "quintic_out") == 0)
		type = NEMOEASE_QUINTIC_OUT_TYPE;
	else if (strcmp(name, "quintic_inout") == 0)
		type = NEMOEASE_QUINTIC_INOUT_TYPE;
	else if (strcmp(name, "sinusoidal_in") == 0)
		type = NEMOEASE_SINUSOIDAL_IN_TYPE;
	else if (strcmp(name, "sinusoidal_out") == 0)
		type = NEMOEASE_SINUSOIDAL_OUT_TYPE;
	else if (strcmp(name, "sinusoidal_inout") == 0)
		type = NEMOEASE_SINUSOIDAL_INOUT_TYPE;
	else if (strcmp(name, "exponential_in") == 0)
		type = NEMOEASE_EXPONENTIAL_IN_TYPE;
	else if (strcmp(name, "exponential_out") == 0)
		type = NEMOEASE_EXPONENTIAL_OUT_TYPE;
	else if (strcmp(name, "exponential_inout") == 0)
		type = NEMOEASE_EXPONENTIAL_INOUT_TYPE;
	else if (strcmp(name, "circular_in") == 0)
		type = NEMOEASE_CIRCULAR_IN_TYPE;
	else if (strcmp(name, "circular_out") == 0)
		type = NEMOEASE_CIRCULAR_OUT_TYPE;
	else if (strcmp(name, "circular_inout") == 0)
		type = NEMOEASE_CIRCULAR_INOUT_TYPE;
	else if (strcmp(name, "bounce_in") == 0)
		type = NEMOEASE_BOUNCE_IN_TYPE;
	else if (strcmp(name, "bounce_out") == 0)
		type = NEMOEASE_BOUNCE_OUT_TYPE;
	else if (strcmp(name, "bounce_inout") == 0)
		type = NEMOEASE_BOUNCE_INOUT_TYPE;

	return type;
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
	if (elapsed >= duration)
		return 1.0f;

	if (ease->dispatch != NULL)
		return ease->dispatch(elapsed, duration);

	return nemoease_get_y_bezier(ease, nemoease_get_x_offset(ease, elapsed / duration));
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
