#ifndef	__CLIP_HELPER_H__
#define	__CLIP_HELPER_H__

struct polygon8 {
	float x[8];
	float y[8];
	int n;
};

struct clipcontext {
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

extern int clip_simple(struct clipcontext *ctx, struct polygon8 *surf, float *ex, float *ey);
extern int clip_transformed(struct clipcontext *ctx, struct polygon8 *surf, float *ex, float *ey);

#endif
