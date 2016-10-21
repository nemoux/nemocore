#ifndef	__NEMO_POLY_H__
#define	__NEMO_POLY_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <math.h>

#include <nemomatrix.h>

typedef enum {
	NEMOPOLY_NONE_PLANE = 0,
	NEMOPOLY_FRONT_PLANE = 1,
	NEMOPOLY_BACK_PLANE = 2,
	NEMOPOLY_LEFT_PLANE = 3,
	NEMOPOLY_RIGHT_PLANE = 4,
	NEMOPOLY_TOP_PLANE = 5,
	NEMOPOLY_BOTTOM_PLANE = 6,
	NEMOPOLY_LAST_PLANE
} NemoPolyPlaneDirection;

extern int nemopoly_intersect_lines(float x00, float y00, float x01, float y01, float x10, float y10, float x11, float y11);
extern int nemopoly_intersect_boxes(float *b0, float *b1);
extern int nemopoly_intersect_ray_triangle(float *v0, float *v1, float *v2, float *o, float *d, float *t, float *u, float *v);
extern int nemopoly_intersect_ray_cube(float *cube, float *o, float *d, float *mint, float *maxt);
extern int nemopoly_pick_triangle(struct nemomatrix *projection, int32_t width, int32_t height, struct nemomatrix *modelview, float *v0, float *v1, float *v2, float x, float y, float *t, float *u, float *v);
extern int nemopoly_pick_cube(struct nemomatrix *projection, int32_t width, int32_t height, struct nemomatrix *modelview, float *boundingbox, float x, float y, float *mint, float *maxt);

extern int nemopoly_convex_hull(float *vertices, int nvertices, float *hulls);
extern int nemopoly_triangulate(float *vertices, int nvertices, float *triangles, float *edges);

static inline double nemopoly_get_point_distance(double x0, double y0, double x1, double y1)
{
	double dx = x1 - x0;
	double dy = y1 - y0;

	return sqrtf(dx * dx + dy * dy);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
