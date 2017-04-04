#ifndef	__NEMOMOTZ_GROUP_H__
#define	__NEMOMOTZ_GROUP_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemolist.h>

#include <nemomotz.h>

typedef enum {
	NEMOMOTZ_GROUP_TRANSFORM_DIRTY = (1 << 8)
} NemoMotzGroupDirty;

struct motzgroup {
	struct motzone one;

	struct tozzmatrix *matrix;
	struct tozzmatrix *inverse;

	float tx, ty;
	float sx, sy;
	float rz;
};

#define NEMOMOTZ_GROUP(one)		((struct motzgroup *)container_of(one, struct motzgroup, one))

extern struct motzone *nemomotz_group_create(void);

NEMOMOTZ_DECLARE_SET_ATTRIBUTE(group, float, tx, tx, NEMOMOTZ_GROUP_TRANSFORM_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(group, float, tx, tx);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(group, float, ty, ty, NEMOMOTZ_GROUP_TRANSFORM_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(group, float, ty, ty);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(group, float, sx, sx, NEMOMOTZ_GROUP_TRANSFORM_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(group, float, sx, sx);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(group, float, sy, sy, NEMOMOTZ_GROUP_TRANSFORM_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(group, float, sy, sy);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(group, float, rz, rz, NEMOMOTZ_GROUP_TRANSFORM_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(group, float, rz, rz);

NEMOMOTZ_DECLARE_SET_TRANSITION(group, tx, tx, NEMOMOTZ_GROUP_TRANSFORM_DIRTY);
NEMOMOTZ_DECLARE_SET_TRANSITION(group, ty, ty, NEMOMOTZ_GROUP_TRANSFORM_DIRTY);
NEMOMOTZ_DECLARE_SET_TRANSITION(group, sx, sx, NEMOMOTZ_GROUP_TRANSFORM_DIRTY);
NEMOMOTZ_DECLARE_SET_TRANSITION(group, sy, sy, NEMOMOTZ_GROUP_TRANSFORM_DIRTY);
NEMOMOTZ_DECLARE_SET_TRANSITION(group, rz, rz, NEMOMOTZ_GROUP_TRANSFORM_DIRTY);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
