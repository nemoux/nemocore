#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemotale.h>
#include <talenode.h>
#include <nemolist.h>
#include <nemolistener.h>
#include <nemomatrix.h>
#include <nemomisc.h>

static int nemotale_node_flush_none(struct talenode *node)
{
	return 0;
}

static int nemotale_node_filter_none(struct talenode *node)
{
	return 0;
}

static int nemotale_node_resize_none(struct talenode *node, int32_t width, int32_t height)
{
	return 0;
}

void *nemotale_node_map_none(struct talenode *node)
{
	return NULL;
}

int nemotale_node_unmap_none(struct talenode *node)
{
	return 0;
}

void nemotale_node_destroy(struct talenode *node)
{
	nemotale_node_finish(node);

	free(node);
}

int nemotale_node_prepare(struct talenode *node)
{
	nemosignal_init(&node->destroy_signal);

	nemolist_init(&node->link);

	node->geometry.x = 0.0f;
	node->geometry.y = 0.0f;
	node->geometry.r = 0.0f;
	node->geometry.px = 0.5f;
	node->geometry.py = 0.5f;
	node->geometry.width = 0;
	node->geometry.height = 0;

	node->transform.enable = 0;
	node->transform.dirty = 1;
	node->transform.custom = 0;

	node->alpha = 1.0f;

	node->has_smooth = 1;

	node->dispatch_flush = nemotale_node_flush_none;
	node->dispatch_filter = nemotale_node_filter_none;
	node->dispatch_resize = nemotale_node_resize_none;
	node->dispatch_map = nemotale_node_map_none;
	node->dispatch_unmap = nemotale_node_unmap_none;

	nemomatrix_init_identity(&node->transform.matrix);
	nemomatrix_init_identity(&node->transform.inverse);

	pixman_region32_init(&node->region);
	pixman_region32_init(&node->boundingbox);
	pixman_region32_init(&node->opaque);
	pixman_region32_init(&node->blend);
	pixman_region32_init(&node->damage);

	return 0;
}

void nemotale_node_finish(struct talenode *node)
{
	nemosignal_emit(&node->destroy_signal, node);

	if (node->tale != NULL)
		nemotale_detach_node(node);

	pixman_region32_fini(&node->boundingbox);
	pixman_region32_fini(&node->opaque);
	pixman_region32_fini(&node->blend);
	pixman_region32_fini(&node->damage);
	pixman_region32_fini(&node->region);

	nemolist_remove(&node->link);
}

void nemotale_node_update_boundingbox(struct talenode *node, int32_t x, int32_t y, int32_t width, int32_t height, pixman_region32_t *bbox)
{
	float minx = HUGE_VALF, miny = HUGE_VALF;
	float maxx = -HUGE_VALF, maxy = -HUGE_VALF;
	int32_t s[4][2] = {
		{ x, y },
		{ x, y + height },
		{ x + width, y },
		{ x + width, y + height }
	};
	float tx, ty;
	float sx, sy, ex, ey;
	int i;

	if (width == 0 || height == 0) {
		pixman_region32_init(bbox);
		return;
	}

	for (i = 0; i < 4; i++) {
		nemotale_node_transform_to_global(node, s[i][0], s[i][1], &tx, &ty);

		if (tx < minx)
			minx = tx;
		if (tx > maxx)
			maxx = tx;
		if (ty < miny)
			miny = ty;
		if (ty > maxy)
			maxy = ty;
	}

	sx = floor(minx);
	sy = floor(miny);
	ex = ceil(maxx);
	ey = ceil(maxy);

	pixman_region32_init_rect(bbox, sx, sy, ex - sx, ey - sy);
}

static void nemotale_node_transform_disable(struct talenode *node)
{
	nemomatrix_init_translate(&node->transform.matrix, node->geometry.x, node->geometry.y);
	nemomatrix_init_translate(&node->transform.inverse, -node->geometry.x, -node->geometry.y);

	pixman_region32_init_rect(&node->boundingbox,
			node->geometry.x, node->geometry.y,
			node->geometry.width, node->geometry.height);
}

static int nemotale_node_transform_enable(struct talenode *node)
{
	struct nemomatrix *matrix = &node->transform.matrix;
	struct nemomatrix *inverse = &node->transform.inverse;

	nemomatrix_init_identity(matrix);

	if (node->geometry.r != 0.0f) {
		nemomatrix_translate(matrix, -node->geometry.width * node->geometry.px, -node->geometry.height * node->geometry.py);
		nemomatrix_rotate(matrix, node->transform.cosr, node->transform.sinr);
		nemomatrix_translate(matrix, node->geometry.width * node->geometry.px, node->geometry.height * node->geometry.py);
	}

	if (node->geometry.sx != 1.0f || node->geometry.sy != 1.0f) {
		nemomatrix_translate(matrix, -node->geometry.width * node->geometry.px, -node->geometry.height * node->geometry.py);
		nemomatrix_scale(matrix, node->geometry.sx, node->geometry.sy);
		nemomatrix_translate(matrix, node->geometry.width * node->geometry.px, node->geometry.height * node->geometry.py);
	}

	nemomatrix_translate(matrix, node->geometry.x, node->geometry.y);

	if (nemomatrix_invert(inverse, matrix) < 0)
		return -1;

	nemotale_node_update_boundingbox(node, 0, 0, node->geometry.width, node->geometry.height, &node->boundingbox);

	return 0;
}

void nemotale_node_update_transform(struct talenode *node)
{
	if (node->transform.dirty == 0)
		return;

	node->transform.dirty = 0;

	if (node->transform.custom != 0)
		return;

	if (node->tale != NULL)
		nemotale_damage_below(node->tale, node);

	if (node->transform.enable == 0) {
		nemotale_node_transform_disable(node);
	} else if (nemotale_node_transform_enable(node) < 0) {
		nemotale_node_transform_disable(node);
	}

	if (node->tale != NULL)
		nemotale_damage_below(node->tale, node);
}

void nemotale_node_correct_pivot(struct talenode *node, float px, float py)
{
	if (node->geometry.px != px || node->geometry.py != py) {
		struct nemomatrix matrix;
		struct nemovector vector;
		float sx, sy, ex, ey;

		nemomatrix_init_identity(&matrix);

		if (node->geometry.r != 0.0f) {
			nemomatrix_translate(&matrix, -node->geometry.width * node->geometry.px, -node->geometry.height * node->geometry.py);
			nemomatrix_rotate(&matrix, node->transform.cosr, node->transform.sinr);
			nemomatrix_translate(&matrix, node->geometry.width * node->geometry.px, node->geometry.height * node->geometry.py);
		}

		if (node->geometry.sx != 1.0f || node->geometry.sy != 1.0f) {
			nemomatrix_translate(&matrix, -node->geometry.width * node->geometry.px, -node->geometry.height * node->geometry.py);
			nemomatrix_scale(&matrix, node->geometry.sx, node->geometry.sy);
			nemomatrix_translate(&matrix, node->geometry.width * node->geometry.px, node->geometry.height * node->geometry.py);
		}

		vector.f[0] = 0.0f;
		vector.f[1] = 0.0f;
		vector.f[2] = 0.0f;
		vector.f[3] = 1.0f;

		nemomatrix_transform(&matrix, &vector);

		if (fabsf(vector.f[3]) < 1e-6) {
			sx = 0.0f;
			sy = 0.0f;
		} else {
			sx = vector.f[0] / vector.f[3];
			sy = vector.f[1] / vector.f[3];
		}

		node->geometry.px = px;
		node->geometry.py = py;

		nemomatrix_init_identity(&matrix);

		if (node->geometry.r != 0.0f) {
			nemomatrix_translate(&matrix, -node->geometry.width * node->geometry.px, -node->geometry.height * node->geometry.py);
			nemomatrix_rotate(&matrix, node->transform.cosr, node->transform.sinr);
			nemomatrix_translate(&matrix, node->geometry.width * node->geometry.px, node->geometry.height * node->geometry.py);
		}

		if (node->geometry.sx != 1.0f || node->geometry.sy != 1.0f) {
			nemomatrix_translate(&matrix, -node->geometry.width * node->geometry.px, -node->geometry.height * node->geometry.py);
			nemomatrix_scale(&matrix, node->geometry.sx, node->geometry.sy);
			nemomatrix_translate(&matrix, node->geometry.width * node->geometry.px, node->geometry.height * node->geometry.py);
		}

		vector.f[0] = 0.0f;
		vector.f[1] = 0.0f;
		vector.f[2] = 0.0f;
		vector.f[3] = 1.0f;

		nemomatrix_transform(&matrix, &vector);

		if (fabsf(vector.f[3]) < 1e-6) {
			ex = 0.0f;
			ey = 0.0f;
		} else {
			ex = vector.f[0] / vector.f[3];
			ey = vector.f[1] / vector.f[3];
		}

		node->geometry.x = node->geometry.x - (ex - sx);
		node->geometry.y = node->geometry.y - (ey - sy);
	}
}

int nemotale_node_transform(struct talenode *node, float d[9])
{
	struct nemomatrix *matrix = &node->transform.matrix;
	struct nemomatrix *inverse = &node->transform.inverse;

	node->transform.enable = 1;
	node->transform.custom = 1;

	nemomatrix_init_3x3(matrix, d);

	if (nemomatrix_invert(inverse, matrix) < 0)
		return -1;

	nemotale_node_update_boundingbox(node, 0, 0, node->geometry.width, node->geometry.height, &node->boundingbox);

	return 0;
}
