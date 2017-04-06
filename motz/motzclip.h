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

NEMOMOTZ_DECLARE_SET_ATTRIBUTE(clip, float, x, x, NEMOMOTZ_CLIP_SHAPE_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(clip, float, x, x);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(clip, float, y, y, NEMOMOTZ_CLIP_SHAPE_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(clip, float, y, y);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(clip, float, w, width, NEMOMOTZ_CLIP_SHAPE_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(clip, float, w, width);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(clip, float, h, height, NEMOMOTZ_CLIP_SHAPE_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(clip, float, h, height);

NEMOMOTZ_DECLARE_SET_TRANSITION(clip, x, x, NEMOMOTZ_CLIP_SHAPE_DIRTY);
NEMOMOTZ_DECLARE_SET_TRANSITION(clip, y, y, NEMOMOTZ_CLIP_SHAPE_DIRTY);
NEMOMOTZ_DECLARE_SET_TRANSITION(clip, w, width, NEMOMOTZ_CLIP_SHAPE_DIRTY);
NEMOMOTZ_DECLARE_SET_TRANSITION(clip, h, height, NEMOMOTZ_CLIP_SHAPE_DIRTY);

NEMOMOTZ_DECLARE_CHECK_TRANSITION_DESTROY(clip);
NEMOMOTZ_DECLARE_CHECK_TRANSITION_REVOKE(clip);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
