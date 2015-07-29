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

	nemomatrix_init_identity(&node->transform.matrix);
	nemomatrix_init_identity(&node->transform.inverse);

	pixman_region32_init(&node->region);
	pixman_region32_init(&node->boundingbox);
	pixman_region32_init(&node->opaque);
	pixman_region32_init(&node->blend);
	pixman_region32_init(&node->input);
	pixman_region32_init(&node->damage);
	node->dirty = 0;

	nemoobject_prepare(&node->object, NEMOTALE_NODE_ATTR_MAX);

	nemoobject_set_reserved(&node->object, "x", &node->geometry.x, sizeof(float));
	nemoobject_set_reserved(&node->object, "y", &node->geometry.y, sizeof(float));
	nemoobject_set_reserved(&node->object, "width", &node->geometry.width, sizeof(int32_t));
	nemoobject_set_reserved(&node->object, "height", &node->geometry.height, sizeof(int32_t));
	nemoobject_set_reserved(&node->object, "px", &node->geometry.px, sizeof(float));
	nemoobject_set_reserved(&node->object, "py", &node->geometry.px, sizeof(float));
	nemoobject_set_reserved(&node->object, "r", &node->geometry.r, sizeof(float));

	return 0;
}

void nemotale_node_finish(struct talenode *node)
{
	nemosignal_emit(&node->destroy_signal, node);

	nemolist_remove(&node->link);

	nemoobject_finish(&node->object);

	pixman_region32_fini(&node->boundingbox);
	pixman_region32_fini(&node->opaque);
	pixman_region32_fini(&node->blend);
	pixman_region32_fini(&node->input);
	pixman_region32_fini(&node->damage);
	pixman_region32_fini(&node->region);
}

void nemotale_node_opaque(struct talenode *node, int32_t x, int32_t y, int32_t width, int32_t height)
{
	pixman_region32_init_rect(&node->opaque, x, y, width, height);
	pixman_region32_init_rect(&node->blend, 0, 0, node->geometry.width, node->geometry.height);
	pixman_region32_subtract(&node->blend, &node->blend, &node->opaque);
}

void nemotale_node_input(struct talenode *node, int32_t x, int32_t y, int32_t width, int32_t height)
{
	pixman_region32_init_rect(&node->input, x, y, width, height);
}

void nemotale_node_damage(struct talenode *node, int32_t x, int32_t y, int32_t width, int32_t height)
{
	pixman_region32_union_rect(&node->damage, &node->damage, x, y, width, height);
	node->dirty = 1;
}

void nemotale_node_damage_region(struct talenode *node, pixman_region32_t *region)
{
	pixman_region32_union(&node->damage, &node->damage, region);
	node->dirty = 1;
}

void nemotale_node_damage_all(struct talenode *node)
{
	pixman_region32_union_rect(&node->damage, &node->damage, 0, 0, node->geometry.width, node->geometry.height);
	node->dirty = 1;
}

void nemotale_node_transform_to_global(struct talenode *node, float sx, float sy, float *x, float *y)
{
	if (node->transform.enable) {
		struct nemovector v = { sx, sy, 0.0f, 1.0f };

		nemomatrix_transform(&node->transform.matrix, &v);

		if (fabsf(v.f[3]) < 1e-6) {
			*x = 0.0f;
			*y = 0.0f;
		} else {
			*x = v.f[0] / v.f[3];
			*y = v.f[1] / v.f[3];
		}
	} else {
		*x = sx + node->geometry.x;
		*y = sy + node->geometry.y;
	}
}

void nemotale_node_transform_from_global(struct talenode *node, float x, float y, float *sx, float *sy)
{
	if (node->transform.enable) {
		struct nemovector v = { x, y, 0.0f, 1.0f };

		nemomatrix_transform(&node->transform.inverse, &v);

		if (fabsf(v.f[3]) < 1e-6) {
			*sx = 0.0f;
			*sy = 0.0f;
		} else {
			*sx = v.f[0] / v.f[3];
			*sy = v.f[1] / v.f[3];
		}
	} else {
		*sx = x - node->geometry.x;
		*sy = y - node->geometry.y;
	}
}

void nemotale_node_boundingbox_update(struct talenode *node, int32_t x, int32_t y, int32_t width, int32_t height, pixman_region32_t *bbox)
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
		float cx = node->geometry.width * node->geometry.px;
		float cy = node->geometry.height * node->geometry.py;

		nemomatrix_translate(matrix, -cx, -cy);
		nemomatrix_rotate(matrix, node->transform.cosr, node->transform.sinr);
		nemomatrix_translate(matrix, cx, cy);
	}

	nemomatrix_translate(matrix, node->geometry.x, node->geometry.y);

	if (nemomatrix_invert(inverse, matrix) < 0)
		return -1;

	nemotale_node_boundingbox_update(node, 0, 0, node->geometry.width, node->geometry.height, &node->boundingbox);

	return 0;
}

void nemotale_node_transform_update(struct talenode *node)
{
	if (node->transform.enable == 0) {
		nemotale_node_transform_disable(node);
	} else if (nemotale_node_transform_enable(node) < 0) {
		nemotale_node_transform_disable(node);
	}
}

void nemotale_node_set_picker(struct talenode *node, nemotale_node_pick_t pick, void *data)
{
	node->pick = pick;
	node->pickdata = data;

	node->picktype = NEMOTALE_PICK_CUSTOM_TYPE;
}

void nemotale_node_translate(struct talenode *node, float x, float y)
{
	if (node->geometry.x == x && node->geometry.y == y)
		return;

	node->geometry.x = x;
	node->geometry.y = y;

	node->transform.dirty = 1;
}

void nemotale_node_rotate(struct talenode *node, float r)
{
	if (node->geometry.r == r)
		return;

	node->geometry.r = r;
	node->transform.cosr = cos(r);
	node->transform.sinr = sin(r);

	node->transform.enable = 1;
	node->transform.dirty = 1;
}

void nemotale_node_pivot(struct talenode *node, float px, float py)
{
	node->geometry.px = px;
	node->geometry.py = py;
}
