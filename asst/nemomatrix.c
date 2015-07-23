#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomatrix.h>

static inline void swap_rows(double *a, double *b)
{
	unsigned k;
	double tmp;

	for (k = 0; k < 13; k += 4) {
		tmp = a[k];
		a[k] = b[k];
		b[k] = tmp;
	}
}

static inline void swap_unsigned(unsigned *a, unsigned *b)
{
	unsigned tmp;

	tmp = *a;
	*a = *b;
	*b = tmp;
}

static inline unsigned find_pivot(double *column, unsigned k)
{
	unsigned p = k;

	for (++k; k < 4; ++k)
		if (fabs(column[p]) < fabs(column[k]))
			p = k;

	return p;
}

static inline int invert_matrix(double *A, unsigned *p, const struct nemomatrix *matrix)
{
	unsigned i, j, k;
	unsigned pivot;
	double pv;

	for (i = 0; i < 4; ++i)
		p[i] = i;
	for (i = 16; i--; )
		A[i] = matrix->d[i];

	for (k = 0; k < 4; ++k) {
		pivot = find_pivot(&A[k * 4], k);
		if (pivot != k) {
			swap_unsigned(&p[k], &p[pivot]);
			swap_rows(&A[k], &A[pivot]);
		}

		pv = A[k * 4 + k];
		if (fabs(pv) < 1e-6)
			return -1;

		for (i = k + 1; i < 4; ++i) {
			A[i + k * 4] /= pv;

			for (j = k + 1; j < 4; ++j)
				A[i + j * 4] -= A[i + k * 4] * A[k + j * 4];
		}
	}

	return 0;
}

static inline void inverse_transform(const double *LU, const unsigned *p, float *v)
{
	double b[4];
	unsigned j;

	b[0] = v[p[0]];
	b[1] = (double)v[p[1]] - b[0] * LU[1 + 0 * 4];
	b[2] = (double)v[p[2]] - b[0] * LU[2 + 0 * 4];
	b[3] = (double)v[p[3]] - b[0] * LU[3 + 0 * 4];
	b[2] -= b[1] * LU[2 + 1 * 4];
	b[3] -= b[1] * LU[3 + 1 * 4];
	b[3] -= b[2] * LU[3 + 2 * 4];

	b[3] /= LU[3 + 3 * 4];
	b[0] -= b[3] * LU[0 + 3 * 4];
	b[1] -= b[3] * LU[1 + 3 * 4];
	b[2] -= b[3] * LU[2 + 3 * 4];

	b[2] /= LU[2 + 2 * 4];
	b[0] -= b[2] * LU[0 + 2 * 4];
	b[1] -= b[2] * LU[1 + 2 * 4];

	b[1] /= LU[1 + 1 * 4];
	b[0] -= b[1] * LU[0 + 1 * 4];

	b[0] /= LU[0 + 0 * 4];

	for (j = 0; j < 4; ++j)
		v[j] = b[j];
}

void nemomatrix_multiply(struct nemomatrix *m, const struct nemomatrix *n)
{
	struct nemomatrix tmp;
	const float *row, *column;
	div_t d;
	int i, j;

	for (i = 0; i < 16; i++) {
		tmp.d[i] = 0;
		d = div(i, 4);
		row = m->d + d.quot * 4;
		column = n->d + d.rem;
		for (j = 0; j < 4; j++)
			tmp.d[i] += row[j] * column[j * 4];
	}

	tmp.type = m->type | n->type;

	memcpy(m, &tmp, sizeof(tmp));
}

void nemomatrix_transform(struct nemomatrix *matrix, struct nemovector *v)
{
	int i, j;
	struct nemovector t;

	for (i = 0; i < 4; i++) {
		t.f[i] = 0;
		for (j = 0; j < 4; j++)
			t.f[i] += v->f[j] * matrix->d[i + j * 4];
	}

	*v = t;
}

int nemomatrix_invert(struct nemomatrix *inverse, const struct nemomatrix *matrix)
{
	double LU[16];
	unsigned perm[4];
	unsigned c;

	if (invert_matrix(LU, perm, matrix) < 0)
		return -1;

	nemomatrix_init_identity(inverse);

	for (c = 0; c < 4; ++c)
		inverse_transform(LU, perm, &inverse->d[c * 4]);
	inverse->type = matrix->type;

	return 0;
}

void nemomatrix_init_identity(struct nemomatrix *matrix)
{
	static const struct nemomatrix identity = {
		.d = {
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		},
		.type = 0,
	};

	memcpy(matrix, &identity, sizeof(identity));
}

void nemomatrix_init_translate(struct nemomatrix *matrix, float x, float y)
{
	struct nemomatrix translate = {
		.d = {
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			x, y, 0, 1
		},
		.type = NEMO_MATRIX_TRANSFORM_TRANSLATE,
	};

	memcpy(matrix, &translate, sizeof(translate));
}

void nemomatrix_init_translate_xyz(struct nemomatrix *matrix, float x, float y, float z)
{
	struct nemomatrix translate = {
		.d = {
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			x, y, z, 1
		},
		.type = NEMO_MATRIX_TRANSFORM_TRANSLATE,
	};

	memcpy(matrix, &translate, sizeof(translate));
}

void nemomatrix_init_scale(struct nemomatrix *matrix, float x, float y)
{
	struct nemomatrix scale = {
		.d = {
			x, 0, 0, 0,
			0, y, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		},
		.type = NEMO_MATRIX_TRANSFORM_SCALE,
	};

	memcpy(matrix, &scale, sizeof(scale));
}

void nemomatrix_init_scale_xyz(struct nemomatrix *matrix, float x, float y, float z)
{
	struct nemomatrix scale = {
		.d = {
			x, 0, 0, 0,
			0, y, 0, 0,
			0, 0, z, 0,
			0, 0, 0, 1
		},
		.type = NEMO_MATRIX_TRANSFORM_SCALE,
	};

	memcpy(matrix, &scale, sizeof(scale));
}

void nemomatrix_init_rotate(struct nemomatrix *matrix, float cos, float sin)
{
	struct nemomatrix rotate = {
		.d = {
			cos, sin, 0, 0,
			-sin, cos, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		},
		.type = NEMO_MATRIX_TRANSFORM_ROTATE,
	};

	memcpy(matrix, &rotate, sizeof(rotate));
}

void nemomatrix_init_rotate_x(struct nemomatrix *matrix, float cos, float sin)
{
	struct nemomatrix rotate = {
		.d = {
			1, 0, 0, 0,
			0, cos, sin, 0,
			0, -sin, cos, 0,
			0, 0, 0, 1
		},
		.type = NEMO_MATRIX_TRANSFORM_ROTATE,
	};

	memcpy(matrix, &rotate, sizeof(rotate));
}

void nemomatrix_init_rotate_y(struct nemomatrix *matrix, float cos, float sin)
{
	struct nemomatrix rotate = {
		.d = {
			cos, 0, -sin, 0,
			0, 1, 0, 0,
			sin, 0, cos, 0,
			0, 0, 0, 1
		},
		.type = NEMO_MATRIX_TRANSFORM_ROTATE,
	};

	memcpy(matrix, &rotate, sizeof(rotate));
}

void nemomatrix_init_rotate_z(struct nemomatrix *matrix, float cos, float sin)
{
	struct nemomatrix rotate = {
		.d = {
			cos, sin, 0, 0,
			-sin, cos, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		},
		.type = NEMO_MATRIX_TRANSFORM_ROTATE,
	};

	memcpy(matrix, &rotate, sizeof(rotate));
}

void nemomatrix_translate(struct nemomatrix *matrix, float x, float y)
{
	struct nemomatrix translate = {
		.d = {
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			x, y, 0, 1
		},
		.type = NEMO_MATRIX_TRANSFORM_TRANSLATE,
	};

	nemomatrix_multiply(matrix, &translate);
}

void nemomatrix_translate_xyz(struct nemomatrix *matrix, float x, float y, float z)
{
	struct nemomatrix translate = {
		.d = {
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			x, y, z, 1
		},
		.type = NEMO_MATRIX_TRANSFORM_TRANSLATE,
	};

	nemomatrix_multiply(matrix, &translate);
}

void nemomatrix_scale(struct nemomatrix *matrix, float x, float y)
{
	struct nemomatrix scale = {
		.d = {
			x, 0, 0, 0,
			0, y, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		},
		.type = NEMO_MATRIX_TRANSFORM_SCALE,
	};

	nemomatrix_multiply(matrix, &scale);
}

void nemomatrix_scale_xyz(struct nemomatrix *matrix, float x, float y, float z)
{
	struct nemomatrix scale = {
		.d = {
			x, 0, 0, 0,
			0, y, 0, 0,
			0, 0, z, 0,
			0, 0, 0, 1
		},
		.type = NEMO_MATRIX_TRANSFORM_SCALE,
	};

	nemomatrix_multiply(matrix, &scale);
}

void nemomatrix_rotate(struct nemomatrix *matrix, float cos, float sin)
{
	struct nemomatrix rotate = {
		.d = {
			cos, sin, 0, 0,
			-sin, cos, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		},
		.type = NEMO_MATRIX_TRANSFORM_ROTATE,
	};

	nemomatrix_multiply(matrix, &rotate);
}

void nemomatrix_rotate_x(struct nemomatrix *matrix, float cos, float sin)
{
	struct nemomatrix rotate = {
		.d = {
			1, 0, 0, 0,
			0, cos, sin, 0,
			0, -sin, cos, 0,
			0, 0, 0, 1
		},
		.type = NEMO_MATRIX_TRANSFORM_ROTATE,
	};

	nemomatrix_multiply(matrix, &rotate);
}

void nemomatrix_rotate_y(struct nemomatrix *matrix, float cos, float sin)
{
	struct nemomatrix rotate = {
		.d = {
			cos, 0, -sin, 0,
			0, 1, 0, 0,
			sin, 0, cos, 0,
			0, 0, 0, 1
		},
		.type = NEMO_MATRIX_TRANSFORM_ROTATE,
	};

	nemomatrix_multiply(matrix, &rotate);
}

void nemomatrix_rotate_z(struct nemomatrix *matrix, float cos, float sin)
{
	struct nemomatrix rotate = {
		.d = {
			cos, sin, 0, 0,
			-sin, cos, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		},
		.type = NEMO_MATRIX_TRANSFORM_ROTATE,
	};

	nemomatrix_multiply(matrix, &rotate);
}

double nemovector_distance(struct nemovector *v0, struct nemovector *v1)
{
	double dx = v0->f[0] - v1->f[0];
	double dy = v0->f[1] - v1->f[1];
	double dz = v0->f[2] - v1->f[2];

	return sqrtf(dx * dx + dy * dy + dz * dz);
}

double nemovector_dot(struct nemovector *v0, struct nemovector *v1)
{
	return v0->f[0] * v1->f[0] + v0->f[1] * v1->f[1] + v0->f[2] * v1->f[2];
}

void nemovector_cross(struct nemovector *v0, struct nemovector *v1)
{
	double lx = v0->f[0];
	double ly = v0->f[1];
	double lz = v0->f[2];
	double rx = v1->f[0];
	double ry = v1->f[1];
	double rz = v1->f[2];

	v0->f[0] = ly * rz - lz * ry;
	v0->f[1] = lz * rx - lx * rz;
	v0->f[2] = lx * ry - ly * rx;
}

void nemomatrix_init_quaternion(struct nemomatrix *matrix, struct nemoquaternion *quat)
{
	float qx = quat->q[0];
	float qy = quat->q[1];
	float qz = quat->q[2];
	float qw = quat->q[3];
	struct nemomatrix rotate = {
		.d = {
			1 - 2 * qy * qy - 2 * qz * qz, 2 * qx * qy + 2 * qz * qw, 2 * qx * qz - 2 * qy * qw, 0,
			2 * qx * qy - 2 * qz * qw, 1 - 2 * qx * qx - 2 * qz * qz, 2 * qy * qz + 2 * qx * qw, 0,
			2 * qx * qz + 2 * qy * qw, 2 * qy * qz - 2 * qx * qw, 1 - 2 * qx * qx - 2 * qy * qy, 0,
			0, 0, 0, 1
		},
		.type = NEMO_MATRIX_TRANSFORM_ROTATE,
	};

	memcpy(matrix, &rotate, sizeof(rotate));
}

void nemomatrix_multiply_quaternion(struct nemomatrix *matrix, struct nemoquaternion *quat)
{
	float qx = quat->q[0];
	float qy = quat->q[1];
	float qz = quat->q[2];
	float qw = quat->q[3];
	struct nemomatrix rotate = {
		.d = {
			1 - 2 * qy * qy - 2 * qz * qz, 2 * qx * qy + 2 * qz * qw, 2 * qx * qz - 2 * qy * qw, 0,
			2 * qx * qy - 2 * qz * qw, 1 - 2 * qx * qx - 2 * qz * qz, 2 * qy * qz + 2 * qx * qw, 0,
			2 * qx * qz + 2 * qy * qw, 2 * qy * qz - 2 * qx * qw, 1 - 2 * qx * qx - 2 * qy * qy, 0,
			0, 0, 0, 1
		},
		.type = NEMO_MATRIX_TRANSFORM_ROTATE,
	};

	nemomatrix_multiply(matrix, &rotate);
}

void nemoquaternion_init_identity(struct nemoquaternion *quat)
{
	quat->q[0] = 0.0f;
	quat->q[1] = 0.0f;
	quat->q[2] = 0.0f;
	quat->q[3] = 1.0f;
}

void nemoquaternion_multiply(struct nemoquaternion *l, struct nemoquaternion *r)
{
	float l0 = l->q[0];
	float l1 = l->q[1];
	float l2 = l->q[2];
	float l3 = l->q[3];
	float r0 = r->q[0];
	float r1 = r->q[1];
	float r2 = r->q[2];
	float r3 = r->q[3];

	l->q[0] = l3 * r0 + l0 * r3 + l1 * r2 - l2 * r1;
	l->q[1] = l3 * r1 + l1 * r3 + l2 * r0 - l0 * r2;
	l->q[2] = l3 * r2 + l2 * r3 + l0 * r1 - l1 * r0;
	l->q[3] = l3 * r3 - l0 * r0 - l1 * r1 - l2 * r2;
}

void nemoquaternion_make_with_angle_axis(struct nemoquaternion *quat, float r, float x, float y, float z)
{
	float hr = r * 0.5f;
	float s = sinf(hr);

	quat->q[0] = s * x;
	quat->q[1] = s * y;
	quat->q[2] = s * z;
	quat->q[3] = cosf(hr);
}

float nemoquaternion_length(struct nemoquaternion *quat)
{
	float qx = quat->q[0];
	float qy = quat->q[1];
	float qz = quat->q[2];
	float qw = quat->q[3];

	return sqrtf(qx * qx + qy * qy + qz * qz + qw * qw);
}

void nemoquaternion_conjugate(struct nemoquaternion *quat)
{
	quat->q[0] = -quat->q[0];
	quat->q[1] = -quat->q[1];
	quat->q[2] = -quat->q[2];
	quat->q[3] = quat->q[3];
}

void nemoquaternion_invert(struct nemoquaternion *quat)
{
	float qx = quat->q[0];
	float qy = quat->q[1];
	float qz = quat->q[2];
	float qw = quat->q[3];
	float s = 1.0f / (qx * qx + qy * qy + qz * qz + qw * qw);

	quat->q[0] = -qx * s;
	quat->q[1] = -qy * s;
	quat->q[2] = -qz * s;
	quat->q[3] = qw * s;
}

void nemoquaternion_normalize(struct nemoquaternion *quat)
{
	float qx = quat->q[0];
	float qy = quat->q[1];
	float qz = quat->q[2];
	float qw = quat->q[3];
	float s = 1.0f / sqrtf(qx * qx + qy * qy + qz * qz + qw * qw);

	quat->q[0] = qx * s;
	quat->q[1] = qy * s;
	quat->q[2] = qz * s;
	quat->q[3] = qw * s;
}
