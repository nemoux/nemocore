#ifndef __NEMO_MATRIX_H__
#define __NEMO_MATRIX_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

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
	NEMOMATRIX_TRANSFORM_TRANSLATE_TYPE = (1 << 0),
	NEMOMATRIX_TRANSFORM_SCALE_TYPE = (1 << 1),
	NEMOMATRIX_TRANSFORM_ROTATE_TYPE = (1 << 2),
	NEMOMATRIX_TRANSFORM_OTHER_TYPE = (1 << 3),
} NemoMatrixTransformType;

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
extern void nemomatrix_transform_vector(struct nemomatrix *matrix, struct nemovector *v);
extern int nemomatrix_transform(struct nemomatrix *matrix, float *x, float *y);
extern int nemomatrix_transform_xyz(struct nemomatrix *matrix, float *x, float *y, float *z);
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

extern void nemomatrix_perspective(struct nemomatrix *matrix, float left, float right, float bottom, float top, float near, float far);
extern void nemomatrix_orthogonal(struct nemomatrix *matrix, float left, float right, float bottom, float top, float near, float far);
extern void nemomatrix_asymmetric(struct nemomatrix *matrix, float *pa, float *pb, float *pc, float *pe, float near, float far);

extern void nemomatrix_append_command(struct nemomatrix *matrix, const char *str);

extern float nemovector_distance(struct nemovector *v0, struct nemovector *v1);
extern float nemovector_dot(struct nemovector *v0, struct nemovector *v1);
extern void nemovector_cross(struct nemovector *v0, struct nemovector *v1);
extern void nemovector_normalize(struct nemovector *v0);

extern void nemomatrix_init_quaternion(struct nemomatrix *matrix, float qx, float qy, float qz, float qw);
extern void nemomatrix_multiply_quaternion(struct nemomatrix *matrix, float qx, float qy, float qz, float qw);

extern void nemomatrix_init_3x3(struct nemomatrix *matrix, float m[9]);
extern void nemomatrix_init_4x4(struct nemomatrix *matrix, float m[16]);

extern void nemoquaternion_init_identity(struct nemoquaternion *quat);
extern void nemoquaternion_multiply(struct nemoquaternion *l, struct nemoquaternion *r);
extern void nemoquaternion_make_with_angle_axis(struct nemoquaternion *quat, float r, float x, float y, float z);
extern float nemoquaternion_length(struct nemoquaternion *quat);
extern void nemoquaternion_conjugate(struct nemoquaternion *quat);
extern void nemoquaternion_invert(struct nemoquaternion *quat);
extern void nemoquaternion_normalize(struct nemoquaternion *quat);
extern void nemoquaternion_init_matrix(struct nemoquaternion *quat, struct nemomatrix *matrix);
extern void nemoquaternion_multiply_matrix(struct nemoquaternion *quat, struct nemomatrix *matrix);

static inline void nemomatrix_set_factor(struct nemomatrix *matrix, int y, int x, float v)
{
	matrix->d[y * 4 + x] = v;
}

static inline float nemomatrix_get_factor(struct nemomatrix *matrix, int y, int x)
{
	return matrix->d[y * 4 + x];
}

static inline void nemomatrix_set_float(struct nemomatrix *matrix, int i, float v)
{
	matrix->d[i] = v;
}

static inline float nemomatrix_get_float(struct nemomatrix *matrix, int i)
{
	return matrix->d[i];
}

static inline float *nemomatrix_get_array(struct nemomatrix *matrix)
{
	return matrix->d;
}

static inline int nemomatrix_has_translate(struct nemomatrix *matrix)
{
	return matrix->type & NEMOMATRIX_TRANSFORM_TRANSLATE_TYPE;
}

static inline int nemomatrix_has_translate_only(struct nemomatrix *matrix)
{
	return matrix->type == NEMOMATRIX_TRANSFORM_TRANSLATE_TYPE;
}

static inline int nemomatrix_has_scale(struct nemomatrix *matrix)
{
	return matrix->type & NEMOMATRIX_TRANSFORM_SCALE_TYPE;
}

static inline int nemomatrix_has_rotate(struct nemomatrix *matrix)
{
	return matrix->type & NEMOMATRIX_TRANSFORM_ROTATE_TYPE;
}

static inline int nemomatrix_has_other(struct nemomatrix *matrix)
{
	return matrix->type & NEMOMATRIX_TRANSFORM_OTHER_TYPE;
}

static inline void nemovector_normalize_xyz(float *v0)
{
	float l = sqrtf(v0[0] * v0[0] + v0[1] * v0[1] + v0[2] * v0[2]);

	v0[0] /= l;
	v0[1] /= l;
	v0[2] /= l;
}

static inline float nemovector_dot_xyz(float *v1, float *v2)
{
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

static inline void nemovector_cross_xyz(float *v0, float *v1, float *v2)
{
	v0[0] = v1[1] * v2[2] - v1[2] * v2[1];
	v0[1] = v1[2] * v2[0] - v1[0] * v2[2];
	v0[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

static inline void nemovector_add_xyz(float *v0, float *v1, float *v2)
{
	v0[0] = v1[0] + v2[0];
	v0[1] = v1[1] + v2[1];
	v0[2] = v1[2] + v2[2];
}

static inline void nemovector_sub_xyz(float *v0, float *v1, float *v2)
{
	v0[0] = v1[0] - v2[0];
	v0[1] = v1[1] - v2[1];
	v0[2] = v1[2] - v2[2];
}

static inline float nemovector_distance_xy(float x0, float y0, float x1, float y1)
{
	float dx = x1 - x0;
	float dy = y1 - y0;

	return sqrtf(dx * dx + dy * dy);
}

static inline float nemovector_dot_xy(float x0, float y0, float x1, float y1)
{
	return x0 * x1 + y0 * y1;
}

static inline float nemovector_cross_xy(float x0, float y0, float x1, float y1)
{
	return x0 * y1 + x1 * y0;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
