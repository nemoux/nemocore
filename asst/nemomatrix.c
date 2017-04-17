#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomatrix.h>
#include <nemotoken.h>
#include <nemomisc.h>

static inline void nemomatrix_swap_rows(float *a, float *b)
{
	unsigned k;
	float tmp;

	for (k = 0; k < 13; k += 4) {
		tmp = a[k];
		a[k] = b[k];
		b[k] = tmp;
	}
}

static inline void nemomatrix_swap_unsigned(unsigned *a, unsigned *b)
{
	unsigned tmp;

	tmp = *a;
	*a = *b;
	*b = tmp;
}

static inline unsigned nemomatrix_find_pivot(float *column, unsigned k)
{
	unsigned p = k;

	for (++k; k < 4; ++k)
		if (fabs(column[p]) < fabs(column[k]))
			p = k;

	return p;
}

static inline int nemomatrix_inverse_matrix(float *A, unsigned *p, const struct nemomatrix *matrix)
{
	unsigned i, j, k;
	unsigned pivot;
	float pv;

	for (i = 0; i < 4; ++i)
		p[i] = i;
	for (i = 16; i--; )
		A[i] = matrix->d[i];

	for (k = 0; k < 4; ++k) {
		pivot = nemomatrix_find_pivot(&A[k * 4], k);
		if (pivot != k) {
			nemomatrix_swap_unsigned(&p[k], &p[pivot]);
			nemomatrix_swap_rows(&A[k], &A[pivot]);
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

static inline void nemomatrix_inverse_transform(const float *LU, const unsigned *p, float *v)
{
	float b[4];
	unsigned j;

	b[0] = v[p[0]];
	b[1] = (float)v[p[1]] - b[0] * LU[1 + 0 * 4];
	b[2] = (float)v[p[2]] - b[0] * LU[2 + 0 * 4];
	b[3] = (float)v[p[3]] - b[0] * LU[3 + 0 * 4];
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

void nemomatrix_transform_vector(struct nemomatrix *matrix, struct nemovector *v)
{
	struct nemovector t;
	int i, j;

	for (i = 0; i < 4; i++) {
		t.f[i] = 0;
		for (j = 0; j < 4; j++)
			t.f[i] += v->f[j] * matrix->d[i + j * 4];
	}

	*v = t;
}

int nemomatrix_transform(struct nemomatrix *matrix, float *x, float *y)
{
	struct nemovector v = { { *x, *y, 0.0f, 1.0f } };
	struct nemovector t;
	int i, j;

	for (i = 0; i < 4; i++) {
		t.f[i] = 0;
		for (j = 0; j < 4; j++)
			t.f[i] += v.f[j] * matrix->d[i + j * 4];
	}

	if (fabsf(t.f[3]) < 1e-6)
		return -1;

	*x = t.f[0];
	*y = t.f[1];

	return 0;
}

int nemomatrix_transform_xyz(struct nemomatrix *matrix, float *x, float *y, float *z)
{
	struct nemovector v = { { *x, *y, *z, 1.0f } };
	struct nemovector t;
	int i, j;

	for (i = 0; i < 4; i++) {
		t.f[i] = 0;
		for (j = 0; j < 4; j++)
			t.f[i] += v.f[j] * matrix->d[i + j * 4];
	}

	if (fabsf(t.f[3]) < 1e-6)
		return -1;

	*x = t.f[0];
	*y = t.f[1];
	*z = t.f[2];

	return 0;
}

int nemomatrix_invert(struct nemomatrix *inverse, const struct nemomatrix *matrix)
{
	float LU[16];
	unsigned perm[4];
	unsigned c;

	if (nemomatrix_inverse_matrix(LU, perm, matrix) < 0)
		return -1;

	nemomatrix_init_identity(inverse);

	for (c = 0; c < 4; ++c)
		nemomatrix_inverse_transform(LU, perm, &inverse->d[c * 4]);
	inverse->type = matrix->type;

	return 0;
}

void nemomatrix_init_3x3(struct nemomatrix *matrix, float m[9])
{
	struct nemomatrix target = {
		.d = {
			m[0], m[3], 0, m[6],
			m[1], m[4], 0, m[7],
			0, 0, 1, 0,
			m[2], m[5], 0, 1
		},
		.type = NEMOMATRIX_TRANSFORM_OTHER_TYPE
	};

	memcpy(matrix, &target, sizeof(target));
}

void nemomatrix_init_4x4(struct nemomatrix *matrix, float m[16])
{
	struct nemomatrix target = {
		.d = {
			m[0], m[4], m[8], m[12],
			m[1], m[5], m[9], m[13],
			m[2], m[6], m[10], m[14],
			m[3], m[7], m[11], m[15]
		},
		.type = NEMOMATRIX_TRANSFORM_OTHER_TYPE
	};

	memcpy(matrix, &target, sizeof(target));
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
		.type = NEMOMATRIX_TRANSFORM_TRANSLATE_TYPE,
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
		.type = NEMOMATRIX_TRANSFORM_TRANSLATE_TYPE,
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
		.type = NEMOMATRIX_TRANSFORM_SCALE_TYPE,
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
		.type = NEMOMATRIX_TRANSFORM_SCALE_TYPE,
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
		.type = NEMOMATRIX_TRANSFORM_ROTATE_TYPE,
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
		.type = NEMOMATRIX_TRANSFORM_ROTATE_TYPE,
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
		.type = NEMOMATRIX_TRANSFORM_ROTATE_TYPE,
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
		.type = NEMOMATRIX_TRANSFORM_ROTATE_TYPE,
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
		.type = NEMOMATRIX_TRANSFORM_TRANSLATE_TYPE,
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
		.type = NEMOMATRIX_TRANSFORM_TRANSLATE_TYPE,
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
		.type = NEMOMATRIX_TRANSFORM_SCALE_TYPE,
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
		.type = NEMOMATRIX_TRANSFORM_SCALE_TYPE,
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
		.type = NEMOMATRIX_TRANSFORM_ROTATE_TYPE,
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
		.type = NEMOMATRIX_TRANSFORM_ROTATE_TYPE,
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
		.type = NEMOMATRIX_TRANSFORM_ROTATE_TYPE,
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
		.type = NEMOMATRIX_TRANSFORM_ROTATE_TYPE,
	};

	nemomatrix_multiply(matrix, &rotate);
}

void nemomatrix_perspective(struct nemomatrix *matrix, float left, float right, float bottom, float top, float near, float far)
{
	struct nemomatrix perspective = {
		.d = {
			2.0f * near / (right - left), 0.0f, 0.0f, 0.0f,
			0.0f, 2.0f * near / (top - bottom), 0.0f, 0.0f,
			(right + left) / (right - left), (top + bottom) / (top - bottom), (far + near) / (near - far), -1.0f,
			0.0f, 0.0f, 2.0f * far * near / (near - far), 0.0f
		},
		.type = NEMOMATRIX_TRANSFORM_OTHER_TYPE
	};

	nemomatrix_multiply(matrix, &perspective);
}

void nemomatrix_orthogonal(struct nemomatrix *matrix, float left, float right, float bottom, float top, float near, float far)
{
	struct nemomatrix orthogonal = {
		.d = {
			2.0f / (right - left), 0.0f, 0.0f, 0.0f,
			0.0f, 2.0f / (top - bottom), 0.0f, 0.0f,
			0.0f, 0.0f, 2.0f / (far - near), 0.0f,
			(right + left) / (left - right), (top + bottom) / (bottom - top), (far + near) / (near - far), 0.0f
		},
		.type = NEMOMATRIX_TRANSFORM_OTHER_TYPE
	};

	nemomatrix_multiply(matrix, &orthogonal);
}

void nemomatrix_asymmetric(struct nemomatrix *matrix, float *pa, float *pb, float *pc, float *pe, float near, float far)
{
	struct nemomatrix transform;
	struct nemomatrix projection;
	struct nemomatrix eyetranslate;
	float va[3], vb[3], vc[3];
	float vr[3], vu[3], vn[3];
	float left, right;
	float bottom, top;
	float eyedist;

	nemovector_sub_xyz(vr, pb, pa);
	nemovector_normalize_xyz(vr);
	nemovector_sub_xyz(vu, pc, pa);
	nemovector_normalize_xyz(vu);
	nemovector_cross_xyz(vn, vr, vu);
	nemovector_normalize_xyz(vn);

	nemovector_sub_xyz(va, pa, pe);
	nemovector_sub_xyz(vb, pb, pe);
	nemovector_sub_xyz(vc, pc, pe);

	eyedist = -nemovector_dot_xyz(va, vn);

	left = nemovector_dot_xyz(vr, va) * near / eyedist;
	right = nemovector_dot_xyz(vr, vb) * near / eyedist;
	bottom = nemovector_dot_xyz(vu, va) * near / eyedist;
	top = nemovector_dot_xyz(vu, vc) * near / eyedist;

	nemomatrix_init_identity(&projection);
	nemomatrix_set_factor(&projection, 0, 0, 2.0f * near / (right - left));
	nemomatrix_set_factor(&projection, 1, 0, 0.0f);
	nemomatrix_set_factor(&projection, 2, 0, (right + left) / (right - left));
	nemomatrix_set_factor(&projection, 3, 0, 0.0f);
	nemomatrix_set_factor(&projection, 0, 1, 0.0f);
	nemomatrix_set_factor(&projection, 1, 1, 2.0f * near / (top - bottom));
	nemomatrix_set_factor(&projection, 2, 1, (top + bottom) / (top - bottom));
	nemomatrix_set_factor(&projection, 3, 1, 0.0f);
	nemomatrix_set_factor(&projection, 0, 2, 0.0f);
	nemomatrix_set_factor(&projection, 1, 2, 0.0f);
	nemomatrix_set_factor(&projection, 2, 2, -(far + near) / (far - near));
	nemomatrix_set_factor(&projection, 3, 2, -2.0f * far * near / (far - near));
	nemomatrix_set_factor(&projection, 0, 3, 0.0f);
	nemomatrix_set_factor(&projection, 1, 3, 0.0f);
	nemomatrix_set_factor(&projection, 2, 3, -1.0f);
	nemomatrix_set_factor(&projection, 3, 3, 0.0f);

	nemomatrix_init_identity(&transform);
	nemomatrix_set_factor(&transform, 0, 0, vr[0]);
	nemomatrix_set_factor(&transform, 1, 0, vr[1]);
	nemomatrix_set_factor(&transform, 2, 0, vr[2]);
	nemomatrix_set_factor(&transform, 3, 0, 0.0f);
	nemomatrix_set_factor(&transform, 0, 1, vu[0]);
	nemomatrix_set_factor(&transform, 1, 1, vu[1]);
	nemomatrix_set_factor(&transform, 2, 1, vu[2]);
	nemomatrix_set_factor(&transform, 3, 1, 0.0f);
	nemomatrix_set_factor(&transform, 0, 2, vn[0]);
	nemomatrix_set_factor(&transform, 1, 2, vn[1]);
	nemomatrix_set_factor(&transform, 2, 2, vn[2]);
	nemomatrix_set_factor(&transform, 3, 2, 0.0f);
	nemomatrix_set_factor(&transform, 0, 3, 0.0f);
	nemomatrix_set_factor(&transform, 1, 3, 0.0f);
	nemomatrix_set_factor(&transform, 2, 3, 0.0f);
	nemomatrix_set_factor(&transform, 3, 3, 1.0f);

	nemomatrix_init_identity(&eyetranslate);
	nemomatrix_set_factor(&eyetranslate, 0, 0, 1.0f);
	nemomatrix_set_factor(&eyetranslate, 1, 0, 0.0f);
	nemomatrix_set_factor(&eyetranslate, 2, 0, 0.0f);
	nemomatrix_set_factor(&eyetranslate, 3, 0, -pe[0]);
	nemomatrix_set_factor(&eyetranslate, 0, 1, 0.0f);
	nemomatrix_set_factor(&eyetranslate, 1, 1, 1.0f);
	nemomatrix_set_factor(&eyetranslate, 2, 1, 0.0f);
	nemomatrix_set_factor(&eyetranslate, 3, 1, -pe[1]);
	nemomatrix_set_factor(&eyetranslate, 0, 2, 0.0f);
	nemomatrix_set_factor(&eyetranslate, 1, 2, 0.0f);
	nemomatrix_set_factor(&eyetranslate, 2, 2, 1.0f);
	nemomatrix_set_factor(&eyetranslate, 3, 2, -pe[2]);
	nemomatrix_set_factor(&eyetranslate, 0, 3, 0.0f);
	nemomatrix_set_factor(&eyetranslate, 1, 3, 0.0f);
	nemomatrix_set_factor(&eyetranslate, 2, 3, 0.0f);
	nemomatrix_set_factor(&eyetranslate, 3, 3, 1.0f);

	nemomatrix_multiply(matrix, &eyetranslate);
	nemomatrix_multiply(matrix, &transform);
	nemomatrix_multiply(matrix, &projection);
}

void nemomatrix_append_command(struct nemomatrix *matrix, const char *str)
{
	struct nemotoken *token;
	const char *cmd;
	float x, y;
	int i;

	token = nemotoken_create(str, strlen(str));
	nemotoken_divide(token, ':');
	nemotoken_divide(token, ';');
	nemotoken_divide(token, ',');
	nemotoken_update(token);

	for (i = 0; i < nemotoken_get_count(token); i++) {
		cmd = nemotoken_get_token(token, i);
		if (strcmp(cmd, "translate") == 0) {
			x = nemotoken_get_float(token, ++i, 0.0f);
			y = nemotoken_get_float(token, ++i, 0.0f);

			nemomatrix_translate(matrix, x, y);
		} else if (strcmp(cmd, "rotate") == 0) {
			x = nemotoken_get_float(token, ++i, 0.0f);

			nemomatrix_rotate(matrix,
					cos(x / 180.0f * M_PI),
					sin(x / 180.0f * M_PI));
		} else if (strcmp(cmd, "scale") == 0) {
			x = nemotoken_get_float(token, ++i, 0.0f);
			y = nemotoken_get_float(token, ++i, 0.0f);

			nemomatrix_scale(matrix, x, y);
		}
	}

	nemotoken_destroy(token);
}

float nemovector_distance(struct nemovector *v0, struct nemovector *v1)
{
	float dx = v0->f[0] - v1->f[0];
	float dy = v0->f[1] - v1->f[1];
	float dz = v0->f[2] - v1->f[2];

	return sqrtf(dx * dx + dy * dy + dz * dz);
}

float nemovector_dot(struct nemovector *v0, struct nemovector *v1)
{
	return v0->f[0] * v1->f[0] + v0->f[1] * v1->f[1] + v0->f[2] * v1->f[2];
}

void nemovector_cross(struct nemovector *v0, struct nemovector *v1)
{
	float lx = v0->f[0];
	float ly = v0->f[1];
	float lz = v0->f[2];
	float rx = v1->f[0];
	float ry = v1->f[1];
	float rz = v1->f[2];

	v0->f[0] = ly * rz - lz * ry;
	v0->f[1] = lz * rx - lx * rz;
	v0->f[2] = lx * ry - ly * rx;
}

void nemovector_normalize(struct nemovector *v0)
{
	float l = sqrtf(v0->f[0] * v0->f[0] + v0->f[1] * v0->f[1] + v0->f[2] * v0->f[2]);

	v0->f[0] /= l;
	v0->f[1] /= l;
	v0->f[2] /= l;
}

void nemomatrix_init_quaternion(struct nemomatrix *matrix, float qx, float qy, float qz, float qw)
{
	struct nemomatrix rotate = {
		.d = {
			1 - 2 * qy * qy - 2 * qz * qz, 2 * qx * qy + 2 * qz * qw, 2 * qx * qz - 2 * qy * qw, 0,
			2 * qx * qy - 2 * qz * qw, 1 - 2 * qx * qx - 2 * qz * qz, 2 * qy * qz + 2 * qx * qw, 0,
			2 * qx * qz + 2 * qy * qw, 2 * qy * qz - 2 * qx * qw, 1 - 2 * qx * qx - 2 * qy * qy, 0,
			0, 0, 0, 1
		},
		.type = NEMOMATRIX_TRANSFORM_ROTATE_TYPE,
	};

	memcpy(matrix, &rotate, sizeof(rotate));
}

void nemomatrix_multiply_quaternion(struct nemomatrix *matrix, float qx, float qy, float qz, float qw)
{
	struct nemomatrix rotate = {
		.d = {
			1 - 2 * qy * qy - 2 * qz * qz, 2 * qx * qy + 2 * qz * qw, 2 * qx * qz - 2 * qy * qw, 0,
			2 * qx * qy - 2 * qz * qw, 1 - 2 * qx * qx - 2 * qz * qz, 2 * qy * qz + 2 * qx * qw, 0,
			2 * qx * qz + 2 * qy * qw, 2 * qy * qz - 2 * qx * qw, 1 - 2 * qx * qx - 2 * qy * qy, 0,
			0, 0, 0, 1
		},
		.type = NEMOMATRIX_TRANSFORM_ROTATE_TYPE,
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

void nemoquaternion_init_matrix(struct nemoquaternion *quat, struct nemomatrix *matrix)
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
		.type = NEMOMATRIX_TRANSFORM_ROTATE_TYPE,
	};

	memcpy(matrix, &rotate, sizeof(rotate));
}

void nemoquaternion_multiply_matrix(struct nemoquaternion *quat, struct nemomatrix *matrix)
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
		.type = NEMOMATRIX_TRANSFORM_ROTATE_TYPE,
	};

	nemomatrix_multiply(matrix, &rotate);
}
