#ifndef __NEMO_CLIP_H__
#define __NEMO_CLIP_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct polygon8 {
	float x[8];
	float y[8];
	int n;
};

struct nemoclip {
	struct {
		float x;
		float y;
	} prev;

	struct {
		float x1, y1;
		float x2, y2;
	} clip;

	struct {
		float *x;
		float *y;
	} vertices;
};

extern int nemoclip_simple(struct nemoclip *clip, struct polygon8 *poly, float *ex, float *ey);
extern int nemoclip_transformed(struct nemoclip *clip, struct polygon8 *poly, float *ex, float *ey);

static inline void nemoclip_set_region(struct nemoclip *clip, float x1, float y1, float x2, float y2)
{
	clip->clip.x1 = x1;
	clip->clip.y1 = y1;
	clip->clip.x2 = x2;
	clip->clip.y2 = y2;
}

static inline int nemoclip_check_minmax(struct nemoclip *clip, float minx, float miny, float maxx, float maxy)
{
	return (minx >= clip->clip.x2) || (miny >= clip->clip.y2) || (maxx <= clip->clip.x1) || (maxy <= clip->clip.y1);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
