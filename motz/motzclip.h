#ifndef	__NEMOMOTZ_CLIP_H__
#define	__NEMOMOTZ_CLIP_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemolist.h>

#include <nemomotz.h>

typedef enum {
	NEMOMOTZ_CLIP_SHAPE_DIRTY = (1 << 8)
} NemoMotzClipDirty;

struct motzclip {
	struct motzone one;

	float x, y;
	float w, h;
};

#define NEMOMOTZ_CLIP(one)		((struct motzclip *)container_of(one, struct motzclip, one))

extern struct motzone *nemomotz_clip_create(void);

#define NEMOMOTZ_CLIP_DECLARE_SET_ATTRIBUTE(type, attr, name, dirty)	\
	static inline void nemomotz_clip_set_##name(struct motzone *one, type attr) {	\
		struct motzclip *clip = NEMOMOTZ_CLIP(one);	\
		clip->attr = attr;	\
		nemomotz_one_set_dirty(one, dirty);	\
	}
#define NEMOMOTZ_CLIP_DECLARE_GET_ATTRIBUTE(type, attr, name)	\
	static inline type nemomotz_clip_get_##name(struct motzone *one, type attr) {	\
		struct motzclip *clip = NEMOMOTZ_CLIP(one);	\
		return clip->attr;	\
	}

NEMOMOTZ_CLIP_DECLARE_SET_ATTRIBUTE(float, x, x, NEMOMOTZ_CLIP_SHAPE_DIRTY);
NEMOMOTZ_CLIP_DECLARE_GET_ATTRIBUTE(float, x, x);
NEMOMOTZ_CLIP_DECLARE_SET_ATTRIBUTE(float, y, y, NEMOMOTZ_CLIP_SHAPE_DIRTY);
NEMOMOTZ_CLIP_DECLARE_GET_ATTRIBUTE(float, y, y);
NEMOMOTZ_CLIP_DECLARE_SET_ATTRIBUTE(float, w, w, NEMOMOTZ_CLIP_SHAPE_DIRTY);
NEMOMOTZ_CLIP_DECLARE_GET_ATTRIBUTE(float, w, w);
NEMOMOTZ_CLIP_DECLARE_SET_ATTRIBUTE(float, h, h, NEMOMOTZ_CLIP_SHAPE_DIRTY);
NEMOMOTZ_CLIP_DECLARE_GET_ATTRIBUTE(float, h, h);

#define NEMOMOTZ_CLIP_DECLARE_SET_TRANSITION(type, attr, name, _dirty)	\
	static inline void nemomotz_transition_clip_set_##name(struct motztrans *trans, int index, struct motzone *one) {	\
		struct motzclip *clip = NEMOMOTZ_CLIP(one);	\
		nemomotz_transition_set_attr(trans, index, &clip->attr, clip->attr, &one->dirty, _dirty);	\
	}

NEMOMOTZ_CLIP_DECLARE_SET_TRANSITION(float, x, x, NEMOMOTZ_CLIP_SHAPE_DIRTY);
NEMOMOTZ_CLIP_DECLARE_SET_TRANSITION(float, y, y, NEMOMOTZ_CLIP_SHAPE_DIRTY);
NEMOMOTZ_CLIP_DECLARE_SET_TRANSITION(float, w, w, NEMOMOTZ_CLIP_SHAPE_DIRTY);
NEMOMOTZ_CLIP_DECLARE_SET_TRANSITION(float, h, h, NEMOMOTZ_CLIP_SHAPE_DIRTY);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
