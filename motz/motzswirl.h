#ifndef __NEMOMOTZ_SWIRL_H__
#define __NEMOMOTZ_SWIRL_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemolist.h>

#include <nemomotz.h>

typedef enum {
	NEMOMOTZ_SWIRL_TRANSFORM_FLAG = (1 << 0)
} NemoMotzSwirlFlag;

typedef enum {
	NEMOMOTZ_SWIRL_SHAPE_DIRTY = (1 << 8),
	NEMOMOTZ_SWIRL_TRANSFORM_DIRTY = (1 << 9),
	NEMOMOTZ_SWIRL_COLOR_DIRTY = (1 << 10),
	NEMOMOTZ_SWIRL_STROKE_WIDTH_DIRTY = (1 << 11)
} NemoMotzSwirlDirty;

struct motzswirl {
	struct motzone one;

	struct tozzstyle *style;
	struct tozzmatrix *matrix;
	struct tozzmatrix *inverse;

	float tx, ty;

	float r, g, b, a;

	float stroke_width;

	float x, y;
	float size;

	float frequence;
	float scale;

	uint32_t duration;
	uint32_t delay;
};

#define NEMOMOTZ_SWIRL(one)			((struct motzswirl *)container_of(one, struct motzswirl, one))

extern struct motzone *nemomotz_swirl_create(void);

NEMOMOTZ_DECLARE_SET_ATTRIBUTE_WITH_FLAGS(swirl, float, tx, tx, NEMOMOTZ_SWIRL_TRANSFORM_DIRTY, NEMOMOTZ_SWIRL_TRANSFORM_FLAG);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(swirl, float, tx, tx);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE_WITH_FLAGS(swirl, float, ty, ty, NEMOMOTZ_SWIRL_TRANSFORM_DIRTY, NEMOMOTZ_SWIRL_TRANSFORM_FLAG);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(swirl, float, ty, ty);

NEMOMOTZ_DECLARE_SET_ATTRIBUTE(swirl, float, r, red, NEMOMOTZ_SWIRL_COLOR_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(swirl, float, r, red);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(swirl, float, g, green, NEMOMOTZ_SWIRL_COLOR_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(swirl, float, g, green);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(swirl, float, b, blue, NEMOMOTZ_SWIRL_COLOR_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(swirl, float, b, blue);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(swirl, float, a, alpha, NEMOMOTZ_SWIRL_COLOR_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(swirl, float, a, alpha);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(swirl, float, stroke_width, stroke_width, NEMOMOTZ_SWIRL_STROKE_WIDTH_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(swirl, float, stroke_width, stroke_width);

NEMOMOTZ_DECLARE_SET_ATTRIBUTE(swirl, uint32_t, duration, duration, 0x0);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(swirl, uint32_t, duration, duration);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(swirl, uint32_t, delay, delay, 0x0);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(swirl, uint32_t, delay, delay);

NEMOMOTZ_DECLARE_SET_ATTRIBUTE(swirl, float, x, x, NEMOMOTZ_SWIRL_SHAPE_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(swirl, float, x, x);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(swirl, float, y, y, NEMOMOTZ_SWIRL_SHAPE_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(swirl, float, y, y);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(swirl, float, size, size, NEMOMOTZ_SWIRL_SHAPE_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(swirl, float, size, size);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(swirl, float, frequence, frequence, NEMOMOTZ_SWIRL_SHAPE_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(swirl, float, frequence, frequence);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(swirl, float, scale, scale, NEMOMOTZ_SWIRL_SHAPE_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(swirl, float, scale, scale);

NEMOMOTZ_DECLARE_CHECK_TRANSITION_DESTROY(swirl);
NEMOMOTZ_DECLARE_CHECK_TRANSITION_REVOKE(swirl);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
