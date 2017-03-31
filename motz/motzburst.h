#ifndef __NEMOMOTZ_BURST_H__
#define __NEMOMOTZ_BURST_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemolist.h>

#include <nemomotz.h>

typedef enum {
	NEMOMOTZ_BURST_TRANSFORM_FLAG = (1 << 0)
} NemoMotzBurstFlag;

typedef enum {
	NEMOMOTZ_BURST_SHAPE_DIRTY = (1 << 8),
	NEMOMOTZ_BURST_TRANSFORM_DIRTY = (1 << 9),
	NEMOMOTZ_BURST_COLOR_DIRTY = (1 << 10),
	NEMOMOTZ_BURST_STROKE_WIDTH_DIRTY = (1 << 11)
} NemoMotzBurstDirty;

struct motzburst {
	struct motzone one;

	struct tozzstyle *style;
	struct tozzmatrix *matrix;
	struct tozzmatrix *inverse;

	float tx, ty;

	float r, g, b, a;

	float stroke_width;

	float size;
};

#define NEMOMOTZ_BURST(one)			((struct motzburst *)container_of(one, struct motzburst, one))

extern struct motzone *nemomotz_burst_create(void);

NEMOMOTZ_DECLARE_SET_ATTRIBUTE_WITH_FLAGS(burst, float, tx, tx, NEMOMOTZ_BURST_TRANSFORM_DIRTY, NEMOMOTZ_BURST_TRANSFORM_FLAG);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(burst, float, tx, tx);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE_WITH_FLAGS(burst, float, ty, ty, NEMOMOTZ_BURST_TRANSFORM_DIRTY, NEMOMOTZ_BURST_TRANSFORM_FLAG);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(burst, float, ty, ty);

NEMOMOTZ_DECLARE_SET_ATTRIBUTE(burst, float, r, red, NEMOMOTZ_BURST_COLOR_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(burst, float, r, red);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(burst, float, g, green, NEMOMOTZ_BURST_COLOR_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(burst, float, g, green);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(burst, float, b, blue, NEMOMOTZ_BURST_COLOR_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(burst, float, b, blue);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(burst, float, a, alpha, NEMOMOTZ_BURST_COLOR_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(burst, float, a, alpha);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(burst, float, stroke_width, stroke_width, NEMOMOTZ_BURST_STROKE_WIDTH_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(burst, float, stroke_width, stroke_width);

NEMOMOTZ_DECLARE_SET_ATTRIBUTE(burst, float, size, size, NEMOMOTZ_BURST_SHAPE_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(burst, float, size, size);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
