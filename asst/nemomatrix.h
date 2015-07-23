#ifndef __NEMO_MATRIX_H__
#define __NEMO_MATRIX_H__

#include <stdint.h>

#include <math.h>

#ifndef MIN
#	define MIN(x,y) (((x) < (y)) ? (x) : (y))
#endif

#ifndef MAX
#	define MAX(x,y) (((x) > (y)) ? (x) : (y))
#endif

#ifndef MIN3D
#	define MIN3D(x,y,z)	MIN(MIN(x,y),z)
#endif

#ifndef MAX3D
#	define MAX3D(x,y,z) MAX(MAX(x,y),z)
#endif

typedef enum {
	NEMO_MATRIX_TRANSFORM_TRANSLATE = (1 << 0),
	NEMO_MATRIX_TRANSFORM_SCALE = (1 << 1),
	NEMO_MATRIX_TRANSFORM_ROTATE = (1 << 2),
	NEMO_MATRIX_TRANSFORM_OTHER = (1 << 3),
} NeomMatrixTransformType;

struct nemomatrix {
	float d[16];
	uint32_t type;
};

struct nemovector {
	float f[4];
};

struct nemoquaternion {
	float q[4];
};

extern void nemomatrix_multiply(struct nemomatrix *m, const struct nemomatrix *n);
extern void nemomatrix_transform(struct nemomatrix *matrix, struct nemovector *v);
extern int nemomatrix_invert(struct nemomatrix *inverse, const struct nemomatrix *matrix);

extern void nemomatrix_init_identity(struct nemomatrix *matrix);
extern void nemomatrix_init_translate(struct nemomatrix *matrix, float x, float y);
extern void nemomatrix_init_translate_xyz(struct nemomatrix *matrix, float x, float y, float z);
extern void nemomatrix_init_scale(struct nemomatrix *matrix, float x, float y);
extern void nemomatrix_init_scale_xyz(struct nemomatrix *matrix, float x, float y, float z);
extern void nemomatrix_init_rotate(struct nemomatrix *matrix, float cos, float sin);
extern void nemomatrix_init_rotate_x(struct nemomatrix *matrix, float cos, float sin);
extern void nemomatrix_init_rotate_y(struct nemomatrix *matrix, float cos, float sin);
extern void nemomatrix_init_rotate_z(struct nemomatrix *matrix, float cos, float sin);

extern void nemomatrix_translate(struct nemomatrix *matrix, float x, float y);
extern void nemomatrix_translate_xyz(struct nemomatrix *matrix, float x, float y, float z);
extern void nemomatrix_scale(struct nemomatrix *matrix, float x, float y);
extern void nemomatrix_scale_xyz(struct nemomatrix *matrix, float x, float y, float z);
extern void nemomatrix_rotate(struct nemomatrix *matrix, float cos, float sin);
extern void nemomatrix_rotate_x(struct nemomatrix *matrix, float cos, float sin);
extern void nemomatrix_rotate_y(struct nemomatrix *matrix, float cos, float sin);
extern void nemomatrix_rotate_z(struct nemomatrix *matrix, float cos, float sin);

extern double nemovector_distance(struct nemovector *v0, struct nemovector *v1);
extern double nemovector_dot(struct nemovector *v0, struct nemovector *v1);
extern void nemovector_cross(struct nemovector *v0, struct nemovector *v1);

extern void nemomatrix_init_quaternion(struct nemomatrix *matrix, struct nemoquaternion *quat);
extern void nemomatrix_multiply_quaternion(struct nemomatrix *matrix, struct nemoquaternion *quat);

extern void nemoquaternion_init_identity(struct nemoquaternion *quat);
extern void nemoquaternion_multiply(struct nemoquaternion *l, struct nemoquaternion *r);
extern void nemoquaternion_make_with_angle_axis(struct nemoquaternion *quat, float r, float x, float y, float z);
extern float nemoquaternion_length(struct nemoquaternion *quat);
extern void nemoquaternion_conjugate(struct nemoquaternion *quat);
extern void nemoquaternion_invert(struct nemoquaternion *quat);
extern void nemoquaternion_normalize(struct nemoquaternion *quat);

static inline double nemovector2d_distance(double x0, double y0, double x1, double y1)
{
	double dx = x1 - x0;
	double dy = y1 - y0;

	return sqrtf(dx * dx + dy * dy);
}

static inline double nemovector2d_distance_square(double x0, double y0, double x1, double y1)
{
	double dx = x1 - x0;
	double dy = y1 - y0;

	return dx * dx + dy * dy;
}

static inline double nemovector2d_dot(double x0, double y0, double x1, double y1)
{
	return x0 * x1 + y0 * y1;
}

static inline double nemovector2d_cross(double x0, double y0, double x1, double y1)
{
	return x0 * y1 + x1 * y0;
}

static inline double nemovector2d_distance_line_point(double x0, double y0, double x1, double y1, double p0, double p1)
{
	double l2;
	double t;

	l2 = nemovector2d_distance_square(x0, y0, x1, y1);
	if (l2 < 1e-6)
		return nemovector2d_distance(p0, p1, x0, y0);

	t = nemovector2d_dot(p0 - x0, p1 - y0, x1 - x0, y1 - y0) / l2;
	if (t < 1e-6)
		return nemovector2d_distance(p0, p1, x0, y0);
	else if (t > 1.0f)
		return nemovector2d_distance(p0, p1, x1, y1);

	return nemovector2d_distance(p0, p1, x0 + t * (x1 - x0), y0 + t * (y1 - y0));
}

#endif
