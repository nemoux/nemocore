#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <polyhelper.h>
#include <nemolist.h>
#include <nemomisc.h>

static inline void poly_unproject(struct nemomatrix *projection, int32_t width, int32_t height, struct nemomatrix *modelview, float x, float y, float z, float *out)
{
	struct nemomatrix matrix;
	struct nemomatrix inverse;
	struct nemovector in;

	matrix = *modelview;
	nemomatrix_multiply(&matrix, projection);

	nemomatrix_invert(&inverse, &matrix);

	in.f[0] = x * 2.0f / width - 1.0f;
	in.f[1] = y * 2.0f / height - 1.0f;
	in.f[2] = z * 2.0f - 1.0f;
	in.f[3] = 1.0f;

	nemomatrix_transform_vector(&inverse, &in);

	if (fabsf(in.f[3]) < 1e-6) {
		out[0] = 0.0f;
		out[1] = 0.0f;
		out[2] = 0.0f;
	} else {
		out[0] = in.f[0] / in.f[3];
		out[1] = in.f[1] / in.f[3];
		out[2] = in.f[2] / in.f[3];
	}
}

int poly_intersect_lines(float x00, float y00, float x01, float y01, float x10, float y10, float x11, float y11)
{
	float sx = x01 - x00;
	float sy = y01 - y00;
	float dx = x11 - x10;
	float dy = y11 - y10;
	float d = sx * dy - sy * dx;
	float cx, cy;
	float t;

	if (d == 0.0f)
		return 0;

	cx = x10 - x00;
	cy = y10 - y00;

	t = (cx * dy - cy * dx) / d;
	if (t < 0.0f || t > 1.0f)
		return 0;

	t = (cx * sy - cy * sx) / d;
	if (t < 0.0f || t > 1.0f)
		return 0;

	return 1;
}

int poly_intersect_boxes(float *b0, float *b1)
{
	float c0[2], c1[2];
	float dx, dy;
	float ax, ay;
	float l, l0, l1;

	c0[0] = (b0[2*0+0] + b0[2*1+0] + b0[2*2+0] + b0[2*3+0]) / 4.0f;
	c0[1] = (b0[2*0+1] + b0[2*1+1] + b0[2*2+1] + b0[2*3+1]) / 4.0f;
	c1[0] = (b1[2*0+0] + b1[2*1+0] + b1[2*2+0] + b1[2*3+0]) / 4.0f;
	c1[1] = (b1[2*0+1] + b1[2*1+1] + b1[2*2+1] + b1[2*3+1]) / 4.0f;

	dx = c0[0] - c1[0];
	dy = c0[1] - c1[1];

	ax = b0[2*1+0] - b0[2*0+0];
	ay = b0[2*1+1] - b0[2*0+1];
	l0 = sqrtf(ax * ax + ay * ay);
	ax /= l0;
	ay /= l0;
	l = fabs((ax * dx + ay * dy) * 2.0f);
	l1 = fabs((b1[2*1+0] - b1[2*0+0]) * ax + (b1[2*1+1] - b1[2*0+1]) * ay) + fabs((b1[2*2+0] - b1[2*1+0]) * ax + (b1[2*2+1] - b1[2*1+1]) * ay);
	if (l > l0 + l1)
		return 0;

	ax = b0[2*2+0] - b0[2*1+0];
	ay = b0[2*2+1] - b0[2*1+1];
	l0 = sqrtf(ax * ax + ay * ay);
	ax /= l0;
	ay /= l0;
	l = fabs((ax * dx + ay * dy) * 2.0f);
	l1 = fabs((b1[2*1+0] - b1[2*0+0]) * ax + (b1[2*1+1] - b1[2*0+1]) * ay) + fabs((b1[2*2+0] - b1[2*1+0]) * ax + (b1[2*2+1] - b1[2*1+1]) * ay);
	if (l > l0 + l1)
		return 0;

	ax = b1[2*1+0] - b1[2*0+0];
	ay = b1[2*1+1] - b1[2*0+1];
	l0 = sqrtf(ax * ax + ay * ay);
	ax /= l0;
	ay /= l0;
	l = fabs((ax * dx + ay * dy) * 2.0f);
	l1 = fabs((b0[2*1+0] - b0[2*0+0]) * ax + (b0[2*1+1] - b0[2*0+1]) * ay) + fabs((b0[2*2+0] - b0[2*1+0]) * ax + (b0[2*2+1] - b0[2*1+1]) * ay);
	if (l > l0 + l1)
		return 0;

	ax = b1[2*2+0] - b1[2*1+0];
	ay = b1[2*2+1] - b1[2*1+1];
	l0 = sqrtf(ax * ax + ay * ay);
	ax /= l0;
	ay /= l0;
	l = fabs((ax * dx + ay * dy) * 2.0f);
	l1 = fabs((b0[2*1+0] - b0[2*0+0]) * ax + (b0[2*1+1] - b0[2*0+1]) * ay) + fabs((b0[2*2+0] - b0[2*1+0]) * ax + (b0[2*2+1] - b0[2*1+1]) * ay);
	if (l > l0 + l1)
		return 0;

	return 1;
}

int poly_intersect_ray_triangle(float *v0, float *v1, float *v2, float *o, float *d, float *t, float *u, float *v)
{
	float edge1[3], edge2[3];
	float tvec[3], pvec[3], qvec[3];
	float det, inv;

	nemovector_sub_xyz(edge1, v1, v0);
	nemovector_sub_xyz(edge2, v2, v0);

	nemovector_cross_xyz(pvec, d, edge2);

	det = nemovector_dot_xyz(edge1, pvec);
	if (fabsf(det) < 1e-6)
		return 0;
	inv = 1.0f / det;

	nemovector_sub_xyz(tvec, o, v0);

	*u = nemovector_dot_xyz(tvec, pvec) * inv;
	if (*u < 0.0f || *u > 1.0f)
		return 0;

	nemovector_cross_xyz(qvec, tvec, edge1);

	*v = nemovector_dot_xyz(d, qvec) * inv;
	if (*v < 0.0f || *u + *v > 1.0f)
		return 0;

	*t = nemovector_dot_xyz(edge2, qvec) * inv;

	return 1;
}

int poly_intersect_ray_cube(float *cube, float *o, float *d, float *mint, float *maxt)
{
	float nmin[3], nmax[3];
	float tmin, tmax;
	float tx1 = (cube[0] - o[0]) / d[0];
	float tx2 = (cube[1] - o[0]) / d[0];
	int plane = POLY_NONE_PLANE;

	if (tx1 < tx2) {
		tmin = tx1;
		tmax = tx2;

		nmin[0] = -1.0f;
		nmin[1] = 0.0f;
		nmin[2] = 0.0f;
		nmax[0] = 1.0f;
		nmax[1] = 0.0f;
		nmax[2] = 0.0f;
	} else {
		tmin = tx2;
		tmax = tx1;

		nmin[0] = 1.0f;
		nmin[1] = 0.0f;
		nmin[2] = 0.0f;
		nmax[0] = -1.0f;
		nmax[1] = 0.0f;
		nmax[2] = 0.0f;
	}

	if (tmin > tmax)
		return POLY_NONE_PLANE;

	float ty1 = (cube[2] - o[1]) / d[1];
	float ty2 = (cube[3] - o[1]) / d[1];

	if (ty1 < ty2) {
		if (ty1 > tmin) {
			tmin = ty1;

			nmin[0] = 0.0f;
			nmin[1] = -1.0f;
			nmin[2] = 0.0f;
		}
		if (ty2 < tmax) {
			tmax = ty2;

			nmax[0] = 0.0f;
			nmax[1] = 1.0f;
			nmax[2] = 0.0f;
		}
	} else {
		if (ty2 > tmin) {
			tmin = ty2;

			nmin[0] = 0.0f;
			nmin[1] = 1.0f;
			nmin[2] = 0.0f;
		}
		if (ty1 < tmax) {
			tmax = ty1;

			nmax[0] = 0.0f;
			nmax[1] = -1.0f;
			nmax[2] = 0.0f;
		}
	}

	if (tmin > tmax)
		return POLY_NONE_PLANE;

	float tz1 = (cube[4] - o[2]) / d[2];
	float tz2 = (cube[5] - o[2]) / d[2];

	if (tz1 < tz2) {
		if (tz1 > tmin) {
			tmin = tz1;

			nmin[0] = 0.0f;
			nmin[1] = 0.0f;
			nmin[2] = -1.0f;
		}
		if (tz2 < tmax) {
			tmax = tz2;

			nmax[0] = 0.0f;
			nmax[1] = 0.0f;
			nmax[2] = 1.0f;
		}
	} else {
		if (tz2 > tmin) {
			tmin = tz2;

			nmin[0] = 0.0f;
			nmin[1] = 0.0f;
			nmin[2] = 1.0f;
		}
		if (tz1 < tmax) {
			tmax = tz1;

			nmax[0] = 0.0f;
			nmax[1] = 0.0f;
			nmax[2] = -1.0f;
		}
	}

	if (tmin > tmax)
		return POLY_NONE_PLANE;

	if (tmin < 0.0f && tmax > 0.0f)
		tmin = 0.0f;

	if (tmin >= 0.0f) {
		if (nmin[0] == 1.0f)
			plane = POLY_RIGHT_PLANE;
		else if (nmin[0] == -1.0f)
			plane = POLY_LEFT_PLANE;
		else if (nmin[1] == 1.0f)
			plane = POLY_TOP_PLANE;
		else if (nmin[1] == -1.0f)
			plane = POLY_BOTTOM_PLANE;
		else if (nmin[2] == 1.0f)
			plane = POLY_FRONT_PLANE;
		else if (nmin[2] == -1.0f)
			plane = POLY_BACK_PLANE;

		*mint = tmin;
		*maxt = tmax;
	}

	return plane;
}

int poly_pick_triangle(struct nemomatrix *projection, int32_t width, int32_t height, struct nemomatrix *modelview, float *v0, float *v1, float *v2, float x, float y, float *t, float *u, float *v)
{
	float near[3], far[3];
	float rayorg[3];
	float rayvec[3];
	float raylen;

	poly_unproject(projection, width, height, modelview, x, y, -1.0f, near);
	poly_unproject(projection, width, height, modelview, x, y, 1.0f, far);

	rayvec[0] = far[0] - near[0];
	rayvec[1] = far[1] - near[1];
	rayvec[2] = far[2] - near[2];

	raylen = sqrtf(rayvec[0] * rayvec[0] + rayvec[1] * rayvec[1] + rayvec[2] * rayvec[2]);

	rayvec[0] /= raylen;
	rayvec[1] /= raylen;
	rayvec[2] /= raylen;

	rayorg[0] = near[0];
	rayorg[1] = near[1];
	rayorg[2] = near[2];

	return poly_intersect_ray_triangle(v0, v1, v2, rayorg, rayvec, t, u, v);
}

int poly_pick_cube(struct nemomatrix *projection, int32_t width, int32_t height, struct nemomatrix *modelview, float *boundingbox, float x, float y, float *mint, float *maxt)
{
	float near[3], far[3];
	float rayorg[3];
	float rayvec[3];
	float raylen;

	poly_unproject(projection, width, height, modelview, x, y, -1.0f, near);
	poly_unproject(projection, width, height, modelview, x, y, 1.0f, far);

	rayvec[0] = far[0] - near[0];
	rayvec[1] = far[1] - near[1];
	rayvec[2] = far[2] - near[2];

	raylen = sqrtf(rayvec[0] * rayvec[0] + rayvec[1] * rayvec[1] + rayvec[2] * rayvec[2]);

	rayvec[0] /= raylen;
	rayvec[1] /= raylen;
	rayvec[2] /= raylen;

	rayorg[0] = near[0];
	rayorg[1] = near[1];
	rayorg[2] = near[2];

	return poly_intersect_ray_cube(boundingbox, rayorg, rayvec, mint, maxt);
}

static float poly_convex_hull_ccw(float x0, float y0, float x1, float y1, float x2, float y2)
{
	return (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0);
}

static int poly_convex_hull_compare(const void *a, const void *b)
{
	float *v0 = (float *)a;
	float *v1 = (float *)b;

	return v0[0] < v1[0] || (v0[0] == v1[0] && v0[1] < v1[1]);
}

int poly_convex_hull(float *vertices, int nvertices, float *hulls)
{
	int i, t, k = 0;

	qsort(vertices, nvertices, sizeof(float[2]), poly_convex_hull_compare);

	for (i = 0; i < nvertices; i++) {
		while (k >= 2 && poly_convex_hull_ccw(hulls[(k - 2) * 2 + 0], hulls[(k - 2) * 2 + 1], hulls[(k - 1) * 2 + 0], hulls[(k - 1) * 2 + 1], vertices[i * 2 + 0], vertices[i * 2 + 1]) <= 0.0f) --k;
		hulls[k * 2 + 0] = vertices[i * 2 + 0];
		hulls[k * 2 + 1] = vertices[i * 2 + 1];
		k++;
	}

	for (i = nvertices - 2, t = k + 1; i >= 0; i--) {
		while (k >= t && poly_convex_hull_ccw(hulls[(k - 2) * 2 + 0], hulls[(k - 2) * 2 + 1], hulls[(k - 1) * 2 + 0], hulls[(k - 1) * 2 + 1], vertices[i * 2 + 0], vertices[i * 2 + 1]) <= 0.0f) --k;
		hulls[k * 2 + 0] = vertices[i * 2 + 0];
		hulls[k * 2 + 1] = vertices[i * 2 + 1];
		k++;
	}

	return k;
}

struct trione {
	float x[3];
	float y[3];

	struct nemolist link;
};

struct edgeone {
	float x[2];
	float y[2];

	int is_bad;

	struct nemolist link;
};

static inline int poly_triangle_circumcircle_contain_vertex(struct trione *tone, float x, float y)
{
	float ab = (tone->x[0] * tone->x[0]) + (tone->y[0] * tone->y[0]);
	float cd = (tone->x[1] * tone->x[1]) + (tone->y[1] * tone->y[1]);
	float ef = (tone->x[2] * tone->x[2]) + (tone->y[2] * tone->y[2]);

	float cx = (ab * (tone->y[2] - tone->y[1]) + cd * (tone->y[0] - tone->y[2]) + ef * (tone->y[1] - tone->y[0])) / (tone->x[0] * (tone->y[2] - tone->y[1]) + tone->x[1] * (tone->y[0] - tone->y[2]) + tone->x[2] * (tone->y[1] - tone->y[0])) / 2.0f;
	float cy = (ab * (tone->x[2] - tone->x[1]) + cd * (tone->x[0] - tone->x[2]) + ef * (tone->x[1] - tone->x[0])) / (tone->y[0] * (tone->x[2] - tone->x[1]) + tone->y[1] * (tone->x[0] - tone->x[2]) + tone->y[2] * (tone->x[1] - tone->x[0])) / 2.0f;
	float cr = sqrtf(((tone->x[0] - cx) * (tone->x[0] - cx)) + ((tone->y[0] - cy) * (tone->y[0] - cy)));
	float dist = sqrtf(((x - cx) * (x - cx)) + ((y - cy) * (y - cy)));

	return dist <= cr;
}

static inline int poly_triangle_contain_vertex(struct trione *tone, float x, float y)
{
	return (tone->x[0] == x && tone->y[0] == y) || (tone->x[1] == x && tone->y[1] == y) || (tone->x[2] == x && tone->y[2] == y);
}

static inline int poly_triangle_compare(struct trione *tone0, struct trione *tone1)
{
	return
		((tone0->x[0] == tone1->x[0] && tone0->y[0] == tone1->y[0]) || (tone0->x[0] == tone1->x[1] && tone0->y[0] == tone1->y[1]) || (tone0->x[0] == tone1->x[2] && tone0->y[0] == tone1->y[2])) &&
		((tone0->x[1] == tone1->x[0] && tone0->y[1] == tone1->y[0]) || (tone0->x[1] == tone1->x[1] && tone0->y[1] == tone1->y[1]) || (tone0->x[1] == tone1->x[2] && tone0->y[1] == tone1->y[2])) &&
		((tone0->x[2] == tone1->x[0] && tone0->y[2] == tone1->y[0]) || (tone0->x[2] == tone1->x[1] && tone0->y[2] == tone1->y[1]) || (tone0->x[2] == tone1->x[2] && tone0->y[2] == tone1->y[2]));
}

static inline int poly_edge_compare(struct edgeone *eone0, struct edgeone *eone1)
{
	return
		(eone0->x[0] == eone1->x[0] && eone0->y[0] == eone1->y[0] && eone0->x[1] == eone1->x[1] && eone0->y[1] == eone1->y[1]) ||
		(eone0->x[0] == eone1->x[1] && eone0->y[0] == eone1->y[1] && eone0->x[1] == eone1->x[0] && eone0->y[1] == eone1->y[0]);
}

static inline struct trione *poly_create_triangle(struct nemolist *list, float x0, float y0, float x1, float y1, float x2, float y2)
{
	struct trione *tone;

	tone = (struct trione *)malloc(sizeof(struct trione));
	tone->x[0] = x0;
	tone->y[0] = y0;
	tone->x[1] = x1;
	tone->y[1] = y1;
	tone->x[2] = x2;
	tone->y[2] = y2;

	nemolist_insert_tail(list, &tone->link);

	return tone;
}

static inline void poly_destroy_triangle(struct trione *tone)
{
	nemolist_remove(&tone->link);

	free(tone);
}

static inline struct edgeone *poly_create_edge(struct nemolist *list, float x0, float y0, float x1, float y1)
{
	struct edgeone *eone;

	eone = (struct edgeone *)malloc(sizeof(struct edgeone));
	eone->x[0] = x0;
	eone->y[0] = y0;
	eone->x[1] = x1;
	eone->y[1] = y1;

	eone->is_bad = 0;

	nemolist_insert_tail(list, &eone->link);

	return 0;
}

static inline void poly_destroy_edge(struct edgeone *eone)
{
	nemolist_remove(&eone->link);

	free(eone);
}

int poly_triangulate(float *vertices, int nvertices, float *triangles, float *edges)
{
	struct nemolist tri_list;
	struct trione *tone, *ntone;
	float minx = vertices[0 * 2 + 0];
	float miny = vertices[0 * 2 + 1];
	float maxx = minx;
	float maxy = miny;
	int i, n;

	for (i = 0; i < nvertices; i++) {
		if (vertices[i * 2 + 0] < minx)
			minx = vertices[i * 2 + 0];
		if (vertices[i * 2 + 1] < miny)
			miny = vertices[i * 2 + 1];
		if (vertices[i * 2 + 0] > maxx)
			maxx = vertices[i * 2 + 0];
		if (vertices[i * 2 + 1] > maxy)
			maxy = vertices[i * 2 + 1];
	}

	float dx = maxx - minx;
	float dy = maxy - miny;
	float dm = MAX(dx, dy);
	float mx = (minx + maxx) / 2.0f;
	float my = (miny + maxy) / 2.0f;

	float x0 = mx - 20 * dm;
	float y0 = my - dm;
	float x1 = mx;
	float y1 = my + 20 * dm;
	float x2 = mx + 20 * dm;
	float y2 = my - dm;

	nemolist_init(&tri_list);

	poly_create_triangle(&tri_list, x0, y0, x1, y1, x2, y2);

	for (i = 0; i < nvertices; i++) {
		struct nemolist edge_list;
		struct edgeone *eone, *neone;
		struct edgeone *eone0, *eone1;
		float x = vertices[i * 2 + 0];
		float y = vertices[i * 2 + 1];

		nemolist_init(&edge_list);

		nemolist_for_each_safe(tone, ntone, &tri_list, link) {
			if (poly_triangle_circumcircle_contain_vertex(tone, x, y) != 0) {
				poly_create_edge(&edge_list, tone->x[0], tone->y[0], tone->x[1], tone->y[1]);
				poly_create_edge(&edge_list, tone->x[1], tone->y[1], tone->x[2], tone->y[2]);
				poly_create_edge(&edge_list, tone->x[2], tone->y[2], tone->x[0], tone->y[0]);

				poly_destroy_triangle(tone);
			}
		}

		nemolist_for_each(eone0, &edge_list, link) {
			nemolist_for_each(eone1, &edge_list, link) {
				if (eone0 == eone1)
					continue;

				if (poly_edge_compare(eone0, eone1) != 0) {
					eone0->is_bad = 1;
					eone1->is_bad = 1;
				}
			}
		}

		nemolist_for_each_safe(eone, neone, &edge_list, link) {
			if (eone->is_bad == 0) {
				poly_create_triangle(&tri_list,
						eone->x[0], eone->y[0],
						eone->x[1], eone->y[1],
						x, y);
			}

			poly_destroy_edge(eone);
		}
	}

	nemolist_for_each_safe(tone, ntone, &tri_list, link) {
		if (poly_triangle_contain_vertex(tone, x0, y0) ||
				poly_triangle_contain_vertex(tone, x1, y1) ||
				poly_triangle_contain_vertex(tone, x2, y2))
			poly_destroy_triangle(tone);
	}

	if (triangles != NULL) {
		int c = 0;

		nemolist_for_each_safe(tone, ntone, &tri_list, link) {
			triangles[c++] = tone->x[0];
			triangles[c++] = tone->y[0];
			triangles[c++] = tone->x[1];
			triangles[c++] = tone->y[1];
			triangles[c++] = tone->x[2];
			triangles[c++] = tone->y[2];
		}
	}

	if (edges != NULL) {
		int c = 0;

		nemolist_for_each_safe(tone, ntone, &tri_list, link) {
			edges[c++] = tone->x[0];
			edges[c++] = tone->y[0];
			edges[c++] = tone->x[1];
			edges[c++] = tone->y[1];
			edges[c++] = tone->x[1];
			edges[c++] = tone->y[1];
			edges[c++] = tone->x[2];
			edges[c++] = tone->y[2];
			edges[c++] = tone->x[2];
			edges[c++] = tone->y[2];
			edges[c++] = tone->x[0];
			edges[c++] = tone->y[0];
		}
	}

	n = nemolist_length(&tri_list);

	nemolist_for_each_safe(tone, ntone, &tri_list, link) {
		nemolist_remove(&tone->link);

		free(tone);
	}

	return n;
}
