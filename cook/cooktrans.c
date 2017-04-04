#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <cooktrans.h>
#include <nemomisc.h>

static int nemocook_transform_update_2d_normal(struct cooktrans *trans)
{
	nemomatrix_init_identity(&trans->matrix);

	if (trans->sx != 1.0f || trans->sy != 1.0f) {
		nemomatrix_translate(&trans->matrix, -trans->px, -trans->py);
		nemomatrix_scale(&trans->matrix, trans->sx, trans->sy);
		nemomatrix_translate(&trans->matrix, trans->px, trans->py);
	}

	if (trans->rz != 0.0f) {
		nemomatrix_translate(&trans->matrix, -trans->px, -trans->py);
		nemomatrix_rotate(&trans->matrix, cos(trans->rz), sin(trans->rz));
		nemomatrix_translate(&trans->matrix, trans->px, trans->py);
	}

	nemomatrix_translate(&trans->matrix, trans->tx, trans->ty);

	if (nemomatrix_invert(&trans->inverse, &trans->matrix) < 0)
		return -1;

	return 0;
}

static int nemocook_transform_update_2d_simple(struct cooktrans *trans)
{
	nemomatrix_init_translate(&trans->matrix, trans->tx, trans->ty);
	nemomatrix_init_translate(&trans->inverse, -trans->tx, -trans->ty);

	return 0;
}

static int nemocook_transform_update_2d_nopin(struct cooktrans *trans)
{
	nemomatrix_init_identity(&trans->matrix);

	if (trans->sx != 1.0f || trans->sy != 1.0f) {
		nemomatrix_scale(&trans->matrix, trans->sx, trans->sy);
	}

	if (trans->rz != 0.0f) {
		nemomatrix_rotate(&trans->matrix, cos(trans->rz), sin(trans->rz));
	}

	nemomatrix_translate(&trans->matrix, trans->tx, trans->ty);

	if (nemomatrix_invert(&trans->inverse, &trans->matrix) < 0)
		return -1;

	return 0;
}

static int nemocook_transform_update_2d_complex(struct cooktrans *trans)
{
	nemomatrix_init_identity(&trans->matrix);

	if (trans->sx != 1.0f || trans->sy != 1.0f) {
		nemomatrix_translate(&trans->matrix, trans->scale.px, trans->scale.py);
		nemomatrix_scale(&trans->matrix, trans->sx, trans->sy);
		nemomatrix_translate(&trans->matrix, trans->scale.ux, trans->scale.uy);
	}

	if (trans->rz != 0.0f) {
		nemomatrix_translate(&trans->matrix, trans->rotate.px, trans->rotate.py);
		nemomatrix_rotate(&trans->matrix, cos(trans->rz), sin(trans->rz));
		nemomatrix_translate(&trans->matrix, trans->rotate.ux, trans->rotate.uy);
	}

	nemomatrix_translate(&trans->matrix, trans->tx, trans->ty);

	if (nemomatrix_invert(&trans->inverse, &trans->matrix) < 0)
		return -1;

	return 0;
}

static int nemocook_transform_update_3d_normal(struct cooktrans *trans)
{
	nemomatrix_init_identity(&trans->matrix);

	if (trans->sx != 1.0f || trans->sy != 1.0f || trans->sz != 1.0f) {
		nemomatrix_translate_xyz(&trans->matrix, -trans->px, -trans->py, -trans->pz);
		nemomatrix_scale_xyz(&trans->matrix, trans->sx, trans->sy, trans->sz);
		nemomatrix_translate_xyz(&trans->matrix, trans->px, trans->py, trans->pz);
	}

	if (trans->rx != 0.0f || trans->ry != 0.0f || trans->rz != 0.0f) {
		nemomatrix_translate_xyz(&trans->matrix, -trans->px, -trans->py, -trans->pz);
		nemomatrix_rotate_x(&trans->matrix, cos(trans->rx), sin(trans->rx));
		nemomatrix_rotate_y(&trans->matrix, cos(trans->ry), sin(trans->ry));
		nemomatrix_rotate_z(&trans->matrix, cos(trans->rz), sin(trans->rz));
		nemomatrix_translate_xyz(&trans->matrix, trans->px, trans->py, trans->pz);
	}

	nemomatrix_translate_xyz(&trans->matrix, trans->tx, trans->ty, trans->tz);

	if (nemomatrix_invert(&trans->inverse, &trans->matrix) < 0)
		return -1;

	return 0;
}

static int nemocook_transform_update_3d_simple(struct cooktrans *trans)
{
	nemomatrix_init_translate_xyz(&trans->matrix, trans->tx, trans->ty, trans->tz);
	nemomatrix_init_translate_xyz(&trans->inverse, -trans->tx, -trans->ty, -trans->tz);

	return 0;
}

static int nemocook_transform_update_3d_nopin(struct cooktrans *trans)
{
	nemomatrix_init_identity(&trans->matrix);

	if (trans->sx != 1.0f || trans->sy != 1.0f || trans->sz != 1.0f) {
		nemomatrix_scale_xyz(&trans->matrix, trans->sx, trans->sy, trans->sz);
	}

	if (trans->rx != 0.0f || trans->ry != 0.0f || trans->rz != 0.0f) {
		nemomatrix_rotate_x(&trans->matrix, cos(trans->rx), sin(trans->rx));
		nemomatrix_rotate_y(&trans->matrix, cos(trans->ry), sin(trans->ry));
		nemomatrix_rotate_z(&trans->matrix, cos(trans->rz), sin(trans->rz));
	}

	nemomatrix_translate_xyz(&trans->matrix, trans->tx, trans->ty, trans->tz);

	if (nemomatrix_invert(&trans->inverse, &trans->matrix) < 0)
		return -1;

	return 0;
}

static int nemocook_transform_update_3d_complex(struct cooktrans *trans)
{
	nemomatrix_init_identity(&trans->matrix);

	if (trans->sx != 1.0f || trans->sy != 1.0f || trans->sz != 1.0f) {
		nemomatrix_translate_xyz(&trans->matrix, trans->scale.px, trans->scale.py, trans->scale.pz);
		nemomatrix_scale_xyz(&trans->matrix, trans->sx, trans->sy, trans->sz);
		nemomatrix_translate_xyz(&trans->matrix, trans->scale.ux, trans->scale.uy, trans->scale.uz);
	}

	if (trans->rx != 0.0f || trans->ry != 0.0f || trans->rz != 0.0f) {
		nemomatrix_translate_xyz(&trans->matrix, trans->rotate.px, trans->rotate.py, trans->rotate.pz);
		nemomatrix_rotate_x(&trans->matrix, cos(trans->rx), sin(trans->rx));
		nemomatrix_rotate_y(&trans->matrix, cos(trans->ry), sin(trans->ry));
		nemomatrix_rotate_z(&trans->matrix, cos(trans->rz), sin(trans->rz));
		nemomatrix_translate_xyz(&trans->matrix, trans->rotate.ux, trans->rotate.uy, trans->rotate.uz);
	}

	nemomatrix_translate_xyz(&trans->matrix, trans->tx, trans->ty, trans->tz);

	if (nemomatrix_invert(&trans->inverse, &trans->matrix) < 0)
		return -1;

	return 0;
}

struct cooktrans *nemocook_transform_create(void)
{
	struct cooktrans *trans;

	trans = (struct cooktrans *)malloc(sizeof(struct cooktrans));
	if (trans == NULL)
		return NULL;
	memset(trans, 0, sizeof(struct cooktrans));

	trans->tx = 0.0f;
	trans->ty = 0.0f;
	trans->tz = 0.0f;
	trans->rx = 0.0f;
	trans->ry = 0.0f;
	trans->rz = 0.0f;
	trans->sx = 1.0f;
	trans->sy = 1.0f;
	trans->sz = 1.0f;
	trans->px = 0.0f;
	trans->py = 0.0f;
	trans->pz = 0.0f;
	trans->scale.px = 0.0f;
	trans->scale.py = 0.0f;
	trans->scale.pz = 0.0f;
	trans->scale.ux = 0.0f;
	trans->scale.uy = 0.0f;
	trans->scale.uz = 0.0f;
	trans->rotate.px = 0.0f;
	trans->rotate.py = 0.0f;
	trans->rotate.pz = 0.0f;
	trans->rotate.ux = 0.0f;
	trans->rotate.uy = 0.0f;
	trans->rotate.uz = 0.0f;

	trans->update = nemocook_transform_update_2d_normal;
	trans->mode = NEMOCOOK_TRANSFORM_2D_MODE;
	trans->state = NEMOCOOK_TRANSFORM_NORMAL_STATE;

	nemomatrix_init_identity(&trans->matrix);
	nemomatrix_init_identity(&trans->inverse);

	return trans;
}

void nemocook_transform_destroy(struct cooktrans *trans)
{
	free(trans);
}

static inline void nemocook_transform_update_callbacks(struct cooktrans *trans)
{
	if (trans->mode == NEMOCOOK_TRANSFORM_2D_MODE) {
		if (trans->state == NEMOCOOK_TRANSFORM_SIMPLE_STATE) {
			trans->update = nemocook_transform_update_2d_simple;
		} else if (trans->state == NEMOCOOK_TRANSFORM_NOPIN_STATE) {
			trans->update = nemocook_transform_update_2d_nopin;
		} else if (trans->state == NEMOCOOK_TRANSFORM_COMPLEX_STATE) {
			trans->update = nemocook_transform_update_2d_complex;
		} else {
			trans->update = nemocook_transform_update_2d_normal;
		}
	} else {
		if (trans->state == NEMOCOOK_TRANSFORM_SIMPLE_STATE) {
			trans->update = nemocook_transform_update_3d_simple;
		} else if (trans->state == NEMOCOOK_TRANSFORM_NOPIN_STATE) {
			trans->update = nemocook_transform_update_3d_nopin;
		} else if (trans->state == NEMOCOOK_TRANSFORM_COMPLEX_STATE) {
			trans->update = nemocook_transform_update_3d_complex;
		} else {
			trans->update = nemocook_transform_update_3d_normal;
		}
	}
}

void nemocook_transform_set_mode(struct cooktrans *trans, int mode)
{
	trans->mode = mode;

	nemocook_transform_update_callbacks(trans);
}

void nemocook_transform_set_state(struct cooktrans *trans, int state)
{
	trans->state = state;

	nemocook_transform_update_callbacks(trans);
}

int nemocook_transform_to_global(struct cooktrans *trans, float sx, float sy, float sz, float *x, float *y, float *z)
{
	struct nemovector v = { { sx, sy, sz, 1.0f } };

	nemomatrix_transform_vector(&trans->matrix, &v);

	if (fabsf(v.f[3]) < 1e-6)
		return -1;

	*x = v.f[0] / v.f[3];
	*y = v.f[1] / v.f[3];
	*z = v.f[2] / v.f[3];

	return 0;
}

int nemocook_transform_from_global(struct cooktrans *trans, float x, float y, float z, float *sx, float *sy, float *sz)
{
	struct nemovector v = { { x, y, z, 1.0f } };

	nemomatrix_transform_vector(&trans->inverse, &v);

	if (fabsf(v.f[3]) < 1e-6)
		return -1;

	*sx = v.f[0] / v.f[3];
	*sy = v.f[1] / v.f[3];
	*sz = v.f[2] / v.f[3];

	return 0;
}

int nemocook_2d_transform_to_global(struct cooktrans *trans, float sx, float sy, float *x, float *y)
{
	struct nemovector v = { { sx, sy, 0.0f, 1.0f } };

	nemomatrix_transform_vector(&trans->matrix, &v);

	if (fabsf(v.f[3]) < 1e-6)
		return -1;

	*x = v.f[0] / v.f[3];
	*y = v.f[1] / v.f[3];

	return 0;
}

int nemocook_2d_transform_from_global(struct cooktrans *trans, float x, float y, float *sx, float *sy)
{
	struct nemovector v = { { x, y, 0.0f, 1.0f } };

	nemomatrix_transform_vector(&trans->inverse, &v);

	if (fabsf(v.f[3]) < 1e-6)
		return -1;

	*sx = v.f[0] / v.f[3];
	*sy = v.f[1] / v.f[3];

	return 0;
}
