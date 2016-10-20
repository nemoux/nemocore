#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <math.h>

#include <nemotrix.h>
#include <nemomisc.h>

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

static inline int nemotrix_triangle_circumcircle_contain_point(struct trione *one, float x, float y)
{
	float ab = (one->x[0] * one->x[0]) + (one->y[0] * one->y[0]);
	float cd = (one->x[1] * one->x[1]) + (one->y[1] * one->y[1]);
	float ef = (one->x[2] * one->x[2]) + (one->y[2] * one->y[2]);

	float cx = (ab * (one->y[2] - one->y[1]) + cd * (one->y[0] - one->y[2]) + ef * (one->y[1] - one->y[0])) / (one->x[0] * (one->y[2] - one->y[1]) + one->x[1] * (one->y[0] - one->y[2]) + one->x[2] * (one->y[1] - one->y[0])) / 2.0f;
	float cy = (ab * (one->x[2] - one->x[1]) + cd * (one->x[0] - one->x[2]) + ef * (one->x[1] - one->x[0])) / (one->y[0] * (one->x[2] - one->x[1]) + one->y[1] * (one->x[0] - one->x[2]) + one->y[2] * (one->x[1] - one->x[0])) / 2.0f;
	float cr = sqrtf(((one->x[0] - cx) * (one->x[0] - cx)) + ((one->y[0] - cy) * (one->y[0] - cy)));
	float dist = sqrtf(((x - cx) * (x - cx)) + ((y - cy) * (y - cy)));

	return dist <= cr;
}

static inline int nemotrix_triangle_contain_point(struct trione *one, float x, float y)
{
	return (one->x[0] == x && one->y[0] == y) || (one->x[1] == x && one->y[1] == y) || (one->x[2] == x && one->y[2] == y);
}

struct nemotrix *nemotrix_create(void)
{
	struct nemotrix *trix;

	trix = (struct nemotrix *)malloc(sizeof(struct nemotrix));
	if (trix == NULL)
		return NULL;
	memset(trix, 0, sizeof(struct nemotrix));

	nemolist_init(&trix->list);

	return trix;
}

void nemotrix_destroy(struct nemotrix *trix)
{
	struct trione *tone, *ntone;

	nemolist_for_each_safe(tone, ntone, &trix->list, link) {
		nemolist_remove(&tone->link);

		free(tone);
	}

	free(trix);
}

static inline struct trione *nemotrix_create_triangle(struct nemolist *list, float x0, float y0, float x1, float y1, float x2, float y2)
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

static inline void nemotrix_destroy_triangle(struct trione *tone)
{
	nemolist_remove(&tone->link);

	free(tone);
}

static inline struct edgeone *nemotrix_create_edge(struct nemolist *list, float x0, float y0, float x1, float y1)
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

static inline void nemotrix_destroy_edge(struct edgeone *eone)
{
	nemolist_remove(&eone->link);

	free(eone);
}

int nemotrix_triangulate(struct nemotrix *trix, float *vertices, int nvertices)
{
	struct trione *tone, *ntone;
	float minx = vertices[0 * 2 + 0];
	float miny = vertices[0 * 2 + 1];
	float maxx = minx;
	float maxy = miny;
	int i;

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

	nemotrix_create_triangle(&trix->list, x0, y0, x1, y1, x2, y2);

	for (i = 0; i < nvertices; i++) {
		struct nemolist edge_list;
		struct edgeone *eone, *neone;
		struct edgeone *eone0, *eone1;
		float x = vertices[i * 2 + 0];
		float y = vertices[i * 2 + 1];

		nemolist_init(&edge_list);

		nemolist_for_each_safe(tone, ntone, &trix->list, link) {
			if (nemotrix_triangle_circumcircle_contain_point(tone, x, y) != 0) {
				nemotrix_create_edge(&edge_list, tone->x[0], tone->y[0], tone->x[1], tone->y[1]);
				nemotrix_create_edge(&edge_list, tone->x[1], tone->y[1], tone->x[2], tone->y[2]);
				nemotrix_create_edge(&edge_list, tone->x[2], tone->y[2], tone->x[0], tone->y[0]);

				nemotrix_destroy_triangle(tone);
			}
		}

		nemolist_for_each(eone0, &edge_list, link) {
			nemolist_for_each(eone1, &edge_list, link) {
				if (eone0 == eone1)
					continue;

				if ((eone0->x[0] == eone1->x[0] && eone0->y[0] == eone1->y[0]) && (eone0->x[1] == eone1->x[1] && eone0->y[1] == eone1->y[1])) {
					eone0->is_bad = 1;
					eone1->is_bad = 1;
				}
			}
		}

		nemolist_for_each_safe(eone, neone, &edge_list, link) {
			if (eone->is_bad == 0) {
				nemotrix_create_triangle(&trix->list,
						eone->x[0], eone->y[0],
						eone->x[1], eone->y[1],
						x, y);
			}

			nemotrix_destroy_edge(eone);
		}
	}

	nemolist_for_each_safe(tone, ntone, &trix->list, link) {
		if (nemotrix_triangle_contain_point(tone, x0, y0) ||
				nemotrix_triangle_contain_point(tone, x1, y1) ||
				nemotrix_triangle_contain_point(tone, x2, y2))
			nemotrix_destroy_triangle(tone);
	}

	return nemolist_length(&trix->list);
}

int nemotrix_get_triangles(struct nemotrix *trix, float *triangles)
{
	struct trione *tone;
	int i = 0;

	nemolist_for_each(tone, &trix->list, link) {
		triangles[i++] = tone->x[0];
		triangles[i++] = tone->y[0];
		triangles[i++] = tone->x[1];
		triangles[i++] = tone->y[1];
		triangles[i++] = tone->x[2];
		triangles[i++] = tone->y[2];
	}

	return i;
}

int nemotrix_get_edges(struct nemotrix *trix, float *edges)
{
	struct trione *tone;
	int i = 0;

	nemolist_for_each(tone, &trix->list, link) {
		edges[i++] = tone->x[0];
		edges[i++] = tone->y[0];
		edges[i++] = tone->x[1];
		edges[i++] = tone->y[1];
		edges[i++] = tone->x[1];
		edges[i++] = tone->y[1];
		edges[i++] = tone->x[2];
		edges[i++] = tone->y[2];
		edges[i++] = tone->x[2];
		edges[i++] = tone->y[2];
		edges[i++] = tone->x[0];
		edges[i++] = tone->y[0];
	}

	return i;
}
