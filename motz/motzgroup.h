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

	struct toyzmatrix *matrix;
	struct toyzmatrix *inverse;

	float tx, ty;
	float sx, sy;
	float rz;
};

#define NEMOMOTZ_GROUP(one)		((struct motzgroup *)container_of(one, struct motzgroup, one))

extern struct motzone *nemomotz_group_create(void);

#define NEMOMOTZ_GROUP_DECLARE_SET_ATTRIBUTE(type, attr, name, dirty)	\
	static inline void nemomotz_group_set_##name(struct motzone *one, type attr) {	\
		struct motzgroup *group = NEMOMOTZ_GROUP(one);	\
		group->attr = attr;	\
		nemomotz_one_set_dirty(one, dirty);	\
	}
#define NEMOMOTZ_GROUP_DECLARE_GET_ATTRIBUTE(type, attr, name)	\
	static inline type nemomotz_group_get_##name(struct motzone *one, type attr) {	\
		struct motzgroup *group = NEMOMOTZ_GROUP(one);	\
		return group->attr;	\
	}
#define NEMOMOTZ_GROUP_DECLARE_DUP_STRING(attr, name, dirty)	\
	static inline void nemomotz_group_set_##name(struct motzone *one, const char *attr) {	\
		struct motzgroup *group = NEMOMOTZ_GROUP(one);	\
		if (group->attr != NULL) free(group->attr);	\
		group->attr = strdup(attr);	\
		nemomotz_one_set_dirty(one, dirty);	\
	}
#define NEMOMOTZ_GROUP_DECLARE_COPY_STRING(attr, name, dirty)	\
	static inline void nemomotz_group_set_##name(struct motzone *one, const char *attr) {	\
		struct motzgroup *group = NEMOMOTZ_GROUP(one);	\
		strcpy(group->attr, attr);	\
		nemomotz_one_set_dirty(one, dirty);	\
	}

NEMOMOTZ_GROUP_DECLARE_SET_ATTRIBUTE(float, tx, tx, NEMOMOTZ_GROUP_TRANSFORM_DIRTY);
NEMOMOTZ_GROUP_DECLARE_GET_ATTRIBUTE(float, tx, tx);
NEMOMOTZ_GROUP_DECLARE_SET_ATTRIBUTE(float, ty, ty, NEMOMOTZ_GROUP_TRANSFORM_DIRTY);
NEMOMOTZ_GROUP_DECLARE_GET_ATTRIBUTE(float, ty, ty);
NEMOMOTZ_GROUP_DECLARE_SET_ATTRIBUTE(float, sx, sx, NEMOMOTZ_GROUP_TRANSFORM_DIRTY);
NEMOMOTZ_GROUP_DECLARE_GET_ATTRIBUTE(float, sx, sx);
NEMOMOTZ_GROUP_DECLARE_SET_ATTRIBUTE(float, sy, sy, NEMOMOTZ_GROUP_TRANSFORM_DIRTY);
NEMOMOTZ_GROUP_DECLARE_GET_ATTRIBUTE(float, sy, sy);
NEMOMOTZ_GROUP_DECLARE_SET_ATTRIBUTE(float, rz, rz, NEMOMOTZ_GROUP_TRANSFORM_DIRTY);
NEMOMOTZ_GROUP_DECLARE_GET_ATTRIBUTE(float, rz, rz);

#define NEMOMOTZ_GROUP_DECLARE_SET_TRANSITION(type, attr, name, _dirty)	\
	static inline void nemomotz_transition_group_set_##name(struct motztrans *trans, int index, struct motzone *one) {	\
		struct motzgroup *group = NEMOMOTZ_GROUP(one);	\
		nemomotz_transition_set_attr(trans, index, &group->attr, group->attr, &one->dirty, _dirty);	\
	}

NEMOMOTZ_GROUP_DECLARE_SET_TRANSITION(float, tx, tx, NEMOMOTZ_GROUP_TRANSFORM_DIRTY);
NEMOMOTZ_GROUP_DECLARE_SET_TRANSITION(float, ty, ty, NEMOMOTZ_GROUP_TRANSFORM_DIRTY);
NEMOMOTZ_GROUP_DECLARE_SET_TRANSITION(float, sx, sx, NEMOMOTZ_GROUP_TRANSFORM_DIRTY);
NEMOMOTZ_GROUP_DECLARE_SET_TRANSITION(float, sy, sy, NEMOMOTZ_GROUP_TRANSFORM_DIRTY);
NEMOMOTZ_GROUP_DECLARE_SET_TRANSITION(float, rz, rz, NEMOMOTZ_GROUP_TRANSFORM_DIRTY);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
