#ifndef	__NEMOTALE_NODE_H__
#define	__NEMOTALE_NODE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>
#include <pixman.h>
#include <cairo.h>

#include <nemolist.h>
#include <nemolistener.h>
#include <nemomatrix.h>

struct nemotale;
struct talenode;

typedef int (*nemotale_node_dispatch_flush_t)(struct talenode *node);
typedef int (*nemotale_node_dispatch_resize_t)(struct talenode *node, int32_t width, int32_t height);
typedef void *(*nemotale_node_dispatch_map_t)(struct talenode *node);
typedef int (*nemotale_node_dispatch_unmap_t)(struct talenode *node);

struct talenode {
	struct nemosignal destroy_signal;

	struct nemolist link;

	void *pmcontext;
	void *glcontext;

	void *userdata;

	struct nemotale *tale;

	struct {
		int32_t width, height;

		float x, y;
		float px, py;
		float r;
		float sx, sy;
	} geometry;

	pixman_region32_t region;
	pixman_region32_t boundingbox;
	pixman_region32_t opaque, blend;
	pixman_region32_t damage;
	int dirty;
	int needs_flush;
	int needs_full_upload;

	nemotale_node_dispatch_flush_t dispatch_flush;
	nemotale_node_dispatch_resize_t dispatch_resize;
	nemotale_node_dispatch_map_t dispatch_map;
	nemotale_node_dispatch_unmap_t dispatch_unmap;

	struct {
		int enable;
		int dirty;
		int custom;

		float cosr, sinr;

		struct nemomatrix matrix, inverse;
	} transform;

	double alpha;

	int has_smooth;
};

extern void nemotale_node_destroy(struct talenode *node);

extern int nemotale_node_prepare(struct talenode *node);
extern void nemotale_node_finish(struct talenode *node);

extern void nemotale_node_update_boundingbox(struct talenode *node, int32_t x, int32_t y, int32_t width, int32_t height, pixman_region32_t *bbox);
extern void nemotale_node_update_transform(struct talenode *node);

extern void nemotale_node_correct_pivot(struct talenode *node, float px, float py);
extern int nemotale_node_transform(struct talenode *node, float d[9]);

static inline void nemotale_node_opaque(struct talenode *node, int32_t x, int32_t y, int32_t width, int32_t height)
{
	pixman_region32_init_rect(&node->opaque, x, y, width, height);
	pixman_region32_init_rect(&node->blend, 0, 0, node->geometry.width, node->geometry.height);
	pixman_region32_subtract(&node->blend, &node->blend, &node->opaque);
}

static inline void nemotale_node_damage(struct talenode *node, int32_t x, int32_t y, int32_t width, int32_t height)
{
	pixman_region32_union_rect(&node->damage, &node->damage, x, y, width, height);
	node->dirty |= 0x1;
	node->needs_flush = 1;
}

static inline void nemotale_node_damage_below(struct talenode *node)
{
	node->dirty |= 0x2;
}

static inline void nemotale_node_damage_region(struct talenode *node, pixman_region32_t *region)
{
	pixman_region32_union(&node->damage, &node->damage, region);
	node->dirty |= 0x1;
	node->needs_flush = 1;
}

static inline void nemotale_node_damage_all(struct talenode *node)
{
	pixman_region32_union_rect(&node->damage, &node->damage, 0, 0, node->geometry.width, node->geometry.height);
	node->dirty |= 0x1;
	node->needs_flush = 1;
	node->needs_full_upload = 1;
}

static inline void nemotale_node_transform_to_global(struct talenode *node, float sx, float sy, float *x, float *y)
{
	if (node->transform.dirty != 0)
		nemotale_node_update_transform(node);

	if (node->transform.enable) {
		struct nemovector v = { { sx, sy, 0.0f, 1.0f } };

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

static inline void nemotale_node_transform_from_global(struct talenode *node, float x, float y, float *sx, float *sy)
{
	if (node->transform.dirty != 0)
		nemotale_node_update_transform(node);

	if (node->transform.enable) {
		struct nemovector v = { { x, y, 0.0f, 1.0f } };

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

static inline void nemotale_node_translate(struct talenode *node, float x, float y)
{
	if (node->geometry.x == x && node->geometry.y == y)
		return;

	node->geometry.x = x;
	node->geometry.y = y;

	node->transform.dirty = 1;
}

static inline void nemotale_node_rotate(struct talenode *node, float r)
{
	if (node->geometry.r == r)
		return;

	node->geometry.r = r;
	node->transform.cosr = cos(r);
	node->transform.sinr = sin(r);

	node->transform.enable = 1;
	node->transform.dirty = 1;
}

static inline void nemotale_node_pivot(struct talenode *node, float px, float py)
{
	if (node->geometry.px == px && node->geometry.py == py)
		return;

	nemotale_node_correct_pivot(node, px, py);

	node->transform.dirty = 1;
}

static inline void nemotale_node_scale(struct talenode *node, float sx, float sy)
{
	if (node->geometry.sx == sx && node->geometry.sy == sy)
		return;

	node->geometry.sx = sx;
	node->geometry.sy = sy;

	node->transform.enable = 1;
	node->transform.dirty = 1;
}

static inline int nemotale_node_is_dirty(struct talenode *node)
{
	return node->dirty != 0;
}

static inline int nemotale_node_is_mapped(struct talenode *node)
{
	return node->tale != NULL;
}

static inline void nemotale_node_set_alpha(struct talenode *node, double alpha)
{
	node->alpha = alpha;

	node->dirty |= 0x2;
}

static inline void nemotale_node_set_smooth(struct talenode *node, int has_smooth)
{
	node->has_smooth = has_smooth;

	node->dirty |= 0x2;
}

static inline int32_t nemotale_node_get_width(struct talenode *node)
{
	return node->geometry.width;
}

static inline int32_t nemotale_node_get_height(struct talenode *node)
{
	return node->geometry.height;
}

static inline void nemotale_node_set_data(struct talenode *node, void *data)
{
	node->userdata = data;
}

static inline void *nemotale_node_get_data(struct talenode *node)
{
	return node->userdata;
}

static inline int nemotale_node_flush(struct talenode *node)
{
	return node->dispatch_flush(node);
}

static inline int nemotale_node_resize(struct talenode *node, int32_t width, int32_t height)
{
	return node->dispatch_resize(node, width, height);
}

static inline void *nemotale_node_map(struct talenode *node)
{
	return node->dispatch_map(node);
}

static inline int nemotale_node_unmap(struct talenode *node)
{
	return node->dispatch_unmap(node);
}

static inline int nemotale_node_needs_full_upload(struct talenode *node)
{
	return node->needs_full_upload;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
