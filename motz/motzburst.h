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

	struct toyzstyle *style;
	struct toyzmatrix *matrix;
	struct toyzmatrix *inverse;

	float tx, ty;

	float size;

	float r, g, b, a;

	float stroke_width;
};

#define NEMOMOTZ_BURST(one)			((struct motzburst *)container_of(one, struct motzburst, one))

extern struct motzone *nemomotz_burst_create(void);

#define NEMOMOTZ_BURST_DECLARE_SET_ATTRIBUTE(type, attr, name, dirty)	\
	static inline void nemomotz_burst_set_##name(struct motzone *one, type attr) {	\
		struct motzburst *burst = NEMOMOTZ_BURST(one);	\
		burst->attr = attr;	\
		nemomotz_one_set_dirty(one, dirty);	\
	}
#define NEMOMOTZ_BURST_DECLARE_SET_ATTRIBUTE_WITH_FLAGS(type, attr, name, dirty, flags)	\
	static inline void nemomotz_burst_set_##name(struct motzone *one, type attr) {	\
		struct motzburst *burst = NEMOMOTZ_BURST(one);	\
		burst->attr = attr;	\
		nemomotz_one_set_dirty(one, dirty);	\
		nemomotz_one_set_flags(one, flags);	\
	}
#define NEMOMOTZ_BURST_DECLARE_GET_ATTRIBUTE(type, attr, name)	\
	static inline type nemomotz_burst_get_##name(struct motzone *one, type attr) {	\
		struct motzburst *burst = NEMOMOTZ_BURST(one);	\
		return burst->attr;	\
	}

NEMOMOTZ_BURST_DECLARE_SET_ATTRIBUTE_WITH_FLAGS(float, tx, tx, NEMOMOTZ_BURST_TRANSFORM_DIRTY, NEMOMOTZ_BURST_TRANSFORM_FLAG);
NEMOMOTZ_BURST_DECLARE_GET_ATTRIBUTE(float, tx, tx);
NEMOMOTZ_BURST_DECLARE_SET_ATTRIBUTE_WITH_FLAGS(float, ty, ty, NEMOMOTZ_BURST_TRANSFORM_DIRTY, NEMOMOTZ_BURST_TRANSFORM_FLAG);
NEMOMOTZ_BURST_DECLARE_GET_ATTRIBUTE(float, ty, ty);

NEMOMOTZ_BURST_DECLARE_SET_ATTRIBUTE(float, size, size, NEMOMOTZ_BURST_SHAPE_DIRTY);
NEMOMOTZ_BURST_DECLARE_GET_ATTRIBUTE(float, size, size);

NEMOMOTZ_BURST_DECLARE_SET_ATTRIBUTE(float, r, red, NEMOMOTZ_BURST_COLOR_DIRTY);
NEMOMOTZ_BURST_DECLARE_GET_ATTRIBUTE(float, r, red);
NEMOMOTZ_BURST_DECLARE_SET_ATTRIBUTE(float, g, green, NEMOMOTZ_BURST_COLOR_DIRTY);
NEMOMOTZ_BURST_DECLARE_GET_ATTRIBUTE(float, g, green);
NEMOMOTZ_BURST_DECLARE_SET_ATTRIBUTE(float, b, blue, NEMOMOTZ_BURST_COLOR_DIRTY);
NEMOMOTZ_BURST_DECLARE_GET_ATTRIBUTE(float, b, blue);
NEMOMOTZ_BURST_DECLARE_SET_ATTRIBUTE(float, a, alpha, NEMOMOTZ_BURST_COLOR_DIRTY);
NEMOMOTZ_BURST_DECLARE_GET_ATTRIBUTE(float, a, alpha);
NEMOMOTZ_BURST_DECLARE_SET_ATTRIBUTE(float, stroke_width, stroke_width, NEMOMOTZ_BURST_STROKE_WIDTH_DIRTY);
NEMOMOTZ_BURST_DECLARE_GET_ATTRIBUTE(float, stroke_width, stroke_width);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
