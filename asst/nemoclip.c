#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <assert.h>
#include <float.h>
#include <math.h>

#include <nemoclip.h>
#include <nemomisc.h>

static float nemoclip_float_difference(float a, float b)
{
	static const float max_diff = 4.0f * FLT_MIN;
	static const float max_rel_diff = 4.0e-5;
	float diff = a - b;
	float adiff = fabsf(diff);

	if (adiff <= max_diff)
		return 0.0f;

	a = fabsf(a);
	b = fabsf(b);
	if (adiff <= (a > b ? a : b) * max_rel_diff)
		return 0.0f;

	return diff;
}

static float nemoclip_intersect_y(float p1x, float p1y, float p2x, float p2y, float x_arg)
{
	float a;
	float diff = nemoclip_float_difference(p1x, p2x);

	if (diff == 0.0f)
		return p2y;

	a = (x_arg - p2x) / diff;

	return p2y + (p1y - p2y) * a;
}

static float nemoclip_intersect_x(float p1x, float p1y, float p2x, float p2y, float y_arg)
{
	float a;
	float diff = nemoclip_float_difference(p1y, p2y);

	if (diff == 0.0f)
		return p2x;

	a = (y_arg - p2y) / diff;

	return p2x + (p1x - p2x) * a;
}

enum nemoclip_path_transition {
	PATH_TRANSITION_OUT_TO_OUT = 0,
	PATH_TRANSITION_OUT_TO_IN = 1,
	PATH_TRANSITION_IN_TO_OUT = 2,
	PATH_TRANSITION_IN_TO_IN = 3,
};

static void nemoclip_append_vertex(struct nemoclip *clip, float x, float y)
{
	*clip->vertices.x++ = x;
	*clip->vertices.y++ = y;
}

static enum nemoclip_path_transition nemoclip_path_transition_left_edge(struct nemoclip *clip, float x, float y)
{
	return ((clip->prev.x >= clip->clip.x1) << 1) | (x >= clip->clip.x1);
}

static enum nemoclip_path_transition nemoclip_path_transition_right_edge(struct nemoclip *clip, float x, float y)
{
	return ((clip->prev.x < clip->clip.x2) << 1) | (x < clip->clip.x2);
}

static enum nemoclip_path_transition nemoclip_path_transition_top_edge(struct nemoclip *clip, float x, float y)
{
	return ((clip->prev.y >= clip->clip.y1) << 1) | (y >= clip->clip.y1);
}

static enum nemoclip_path_transition nemoclip_path_transition_bottom_edge(struct nemoclip *clip, float x, float y)
{
	return ((clip->prev.y < clip->clip.y2) << 1) | (y < clip->clip.y2);
}

static void nemoclip_polygon_leftright(struct nemoclip *clip, enum nemoclip_path_transition transition, float x, float y, float clip_x)
{
	float yi;

	switch (transition) {
		case PATH_TRANSITION_IN_TO_IN:
			nemoclip_append_vertex(clip, x, y);
			break;
		case PATH_TRANSITION_IN_TO_OUT:
			yi = nemoclip_intersect_y(clip->prev.x, clip->prev.y, x, y, clip_x);
			nemoclip_append_vertex(clip, clip_x, yi);
			break;
		case PATH_TRANSITION_OUT_TO_IN:
			yi = nemoclip_intersect_y(clip->prev.x, clip->prev.y, x, y, clip_x);
			nemoclip_append_vertex(clip, clip_x, yi);
			nemoclip_append_vertex(clip, x, y);
			break;
		case PATH_TRANSITION_OUT_TO_OUT:
			/* nothing */
			break;
		default:
			assert(0 && "bad enum nemoclip_path_transition");
	}

	clip->prev.x = x;
	clip->prev.y = y;
}

static void nemoclip_polygon_topbottom(struct nemoclip *clip, enum nemoclip_path_transition transition,
		float x, float y, float clip_y)
{
	float xi;

	switch (transition) {
		case PATH_TRANSITION_IN_TO_IN:
			nemoclip_append_vertex(clip, x, y);
			break;
		case PATH_TRANSITION_IN_TO_OUT:
			xi = nemoclip_intersect_x(clip->prev.x, clip->prev.y, x, y, clip_y);
			nemoclip_append_vertex(clip, xi, clip_y);
			break;
		case PATH_TRANSITION_OUT_TO_IN:
			xi = nemoclip_intersect_x(clip->prev.x, clip->prev.y, x, y, clip_y);
			nemoclip_append_vertex(clip, xi, clip_y);
			nemoclip_append_vertex(clip, x, y);
			break;
		case PATH_TRANSITION_OUT_TO_OUT:
			/* nothing */
			break;
		default:
			assert(0 && "bad enum nemoclip_path_transition");
	}

	clip->prev.x = x;
	clip->prev.y = y;
}

static void nemoclip_prepare(struct nemoclip *clip, const struct polygon8 *src, float *dst_x, float *dst_y)
{
	clip->prev.x = src->x[src->n - 1];
	clip->prev.y = src->y[src->n - 1];
	clip->vertices.x = dst_x;
	clip->vertices.y = dst_y;
}

static int nemoclip_polygon_left(struct nemoclip *clip, const struct polygon8 *src, float *dst_x, float *dst_y)
{
	enum nemoclip_path_transition trans;
	int i;

	nemoclip_prepare(clip, src, dst_x, dst_y);

	for (i = 0; i < src->n; i++) {
		trans = nemoclip_path_transition_left_edge(clip, src->x[i], src->y[i]);
		nemoclip_polygon_leftright(clip, trans, src->x[i], src->y[i], clip->clip.x1);
	}

	return clip->vertices.x - dst_x;
}

static int nemoclip_polygon_right(struct nemoclip *clip, const struct polygon8 *src, float *dst_x, float *dst_y)
{
	enum nemoclip_path_transition trans;
	int i;

	nemoclip_prepare(clip, src, dst_x, dst_y);

	for (i = 0; i < src->n; i++) {
		trans = nemoclip_path_transition_right_edge(clip, src->x[i], src->y[i]);
		nemoclip_polygon_leftright(clip, trans, src->x[i], src->y[i], clip->clip.x2);
	}

	return clip->vertices.x - dst_x;
}

static int nemoclip_polygon_top(struct nemoclip *clip, const struct polygon8 *src, float *dst_x, float *dst_y)
{
	enum nemoclip_path_transition trans;
	int i;

	nemoclip_prepare(clip, src, dst_x, dst_y);

	for (i = 0; i < src->n; i++) {
		trans = nemoclip_path_transition_top_edge(clip, src->x[i], src->y[i]);
		nemoclip_polygon_topbottom(clip, trans, src->x[i], src->y[i], clip->clip.y1);
	}

	return clip->vertices.x - dst_x;
}

static int nemoclip_polygon_bottom(struct nemoclip *clip, const struct polygon8 *src, float *dst_x, float *dst_y)
{
	enum nemoclip_path_transition trans;
	int i;

	nemoclip_prepare(clip, src, dst_x, dst_y);

	for (i = 0; i < src->n; i++) {
		trans = nemoclip_path_transition_bottom_edge(clip, src->x[i], src->y[i]);
		nemoclip_polygon_topbottom(clip, trans, src->x[i], src->y[i], clip->clip.y2);
	}

	return clip->vertices.x - dst_x;
}

int nemoclip_simple(struct nemoclip *clip, struct polygon8 *poly, float *ex, float *ey)
{
	int i;

	for (i = 0; i < poly->n; i++) {
		ex[i] = CLIP(poly->x[i], clip->clip.x1, clip->clip.x2);
		ey[i] = CLIP(poly->y[i], clip->clip.y1, clip->clip.y2);
	}

	return poly->n;
}

int nemoclip_transformed(struct nemoclip *clip, struct polygon8 *poly, float *ex, float *ey)
{
	struct polygon8 polygon;
	int i, n;

	polygon.n = nemoclip_polygon_left(clip, poly, polygon.x, polygon.y);
	poly->n = nemoclip_polygon_right(clip, &polygon, poly->x, poly->y);
	polygon.n = nemoclip_polygon_top(clip, poly, polygon.x, polygon.y);
	poly->n = nemoclip_polygon_bottom(clip, &polygon, poly->x, poly->y);

	ex[0] = poly->x[0];
	ey[0] = poly->y[0];
	n = 1;

	for (i = 1; i < poly->n; i++) {
		if (nemoclip_float_difference(ex[n - 1], poly->x[i]) == 0.0f &&
				nemoclip_float_difference(ey[n - 1], poly->y[i]) == 0.0f)
			continue;
		ex[n] = poly->x[i];
		ey[n] = poly->y[i];
		n++;
	}

	if (nemoclip_float_difference(ex[n - 1], poly->x[0]) == 0.0f &&
			nemoclip_float_difference(ey[n - 1], poly->y[0]) == 0.0f)
		n--;

	return n;
}
