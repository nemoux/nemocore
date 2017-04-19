#ifndef __NEMOCOOK_TRANSFORM_H__
#define __NEMOCOOK_TRANSFORM_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemomatrix.h>

typedef enum {
	NEMOCOOK_TRANSFORM_2D_MODE = 0,
	NEMOCOOK_TRANSFORM_3D_MODE = 1,
	NEMOCOOK_TRANSFORM_LAST_MODE
} NemoCookTransformMode;

typedef enum {
	NEMOCOOK_TRANSFORM_NORMAL_STATE = 0,
	NEMOCOOK_TRANSFORM_SIMPLE_STATE = 1,
	NEMOCOOK_TRANSFORM_NOPIN_STATE = 2,
	NEMOCOOK_TRANSFORM_COMPLEX_STATE = 3,
	NEMOCOOK_TRANSFORM_LAST_STATE
} NemoCookTransformState;

struct cooktrans;

typedef int (*nemocook_transform_update_t)(struct cooktrans *trans);

struct cooktrans {
	float tx, ty, tz;
	float sx, sy, sz;
	float rx, ry, rz;
	float px, py, pz;

	struct {
		float px, py, pz;
		float ux, uy, uz;
	} scale;

	struct {
		float px, py, pz;
		float ux, uy, uz;
	} rotate;

	struct nemomatrix matrix;
	struct nemomatrix inverse;

	struct cooktrans *parent;

	nemocook_transform_update_t update;
	int mode;
	int state;
};

extern struct cooktrans *nemocook_transform_create(void);
extern void nemocook_transform_destroy(struct cooktrans *trans);

extern void nemocook_transform_set_mode(struct cooktrans *trans, int mode);
extern void nemocook_transform_set_state(struct cooktrans *trans, int state);

extern int nemocook_transform_to_global(struct cooktrans *trans, float sx, float sy, float sz, float *x, float *y, float *z);
extern int nemocook_transform_from_global(struct cooktrans *trans, float x, float y, float z, float *sx, float *sy, float *sz);
extern int nemocook_2d_transform_to_global(struct cooktrans *trans, float sx, float sy, float *x, float *y);
extern int nemocook_2d_transform_from_global(struct cooktrans *trans, float x, float y, float *sx, float *sy);

static inline void nemocook_transform_set_translate(struct cooktrans *trans, float tx, float ty, float tz)
{
	trans->tx = tx;
	trans->ty = ty;
	trans->tz = tz;
}

static inline void nemocook_transform_set_scale(struct cooktrans *trans, float sx, float sy, float sz)
{
	trans->sx = sx;
	trans->sy = sy;
	trans->sz = sz;
}

static inline void nemocook_transform_pin_scale(struct cooktrans *trans, float px, float py, float pz)
{
	trans->scale.px = px;
	trans->scale.py = py;
	trans->scale.pz = pz;
}

static inline void nemocook_transform_unpin_scale(struct cooktrans *trans, float ux, float uy, float uz)
{
	trans->scale.ux = ux;
	trans->scale.uy = uy;
	trans->scale.uz = uz;
}

static inline void nemocook_transform_set_rotate(struct cooktrans *trans, float rx, float ry, float rz)
{
	trans->rx = rx;
	trans->ry = ry;
	trans->rz = rz;
}

static inline void nemocook_transform_pin_rotate(struct cooktrans *trans, float px, float py, float pz)
{
	trans->rotate.px = px;
	trans->rotate.py = py;
	trans->rotate.pz = pz;
}

static inline void nemocook_transform_unpin_rotate(struct cooktrans *trans, float ux, float uy, float uz)
{
	trans->rotate.ux = ux;
	trans->rotate.uy = uy;
	trans->rotate.uz = uz;
}

static inline void nemocook_transform_set_pivot(struct cooktrans *trans, float px, float py, float pz)
{
	trans->px = px;
	trans->py = py;
	trans->pz = pz;
}

static inline void nemocook_transform_set_parent(struct cooktrans *trans, struct cooktrans *parent)
{
	trans->parent = parent;
}

static inline int nemocook_transform_update(struct cooktrans *trans)
{
	return trans->update(trans);
}

static inline struct nemomatrix *nemocook_transform_get_matrix(struct cooktrans *trans)
{
	return &trans->matrix;
}

static inline float *nemocook_transform_get_matrix4fv(struct cooktrans *trans)
{
	return nemomatrix_get_array(&trans->matrix);
}

static inline struct nemomatrix *nemocook_transform_get_inverse_matrix(struct cooktrans *trans)
{
	return &trans->inverse;
}

static inline float *nemocook_transform_get_inverse_matrix4fv(struct cooktrans *trans)
{
	return nemomatrix_get_array(&trans->inverse);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
