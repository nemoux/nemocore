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
#include <nemoattr.h>

#define	NEMOTALE_NODE_ATTR_MAX			(16)

struct nemotale;
struct talenode;

typedef int (*nemotale_node_dispatch_flush_t)(struct talenode *node);
typedef int (*nemotale_node_dispatch_filter_t)(struct talenode *node);
typedef int (*nemotale_node_dispatch_resize_t)(struct talenode *node, int32_t width, int32_t height);
typedef int (*nemotale_node_dispatch_viewport_t)(struct talenode *node, int32_t width, int32_t height);

struct talenode {
	struct nemosignal destroy_signal;

	struct nemolist link;

	void *pmcontext;
	void *glcontext;

	void *userdata;

	struct nemotale *tale;

	uint32_t id;

	struct nemoobject object;

	struct {
		float x, y;
		int32_t width, height;
		float px, py;
		float r;
		float sx, sy;
	} geometry;

	struct {
		int32_t width, height;

		double sx, sy;
		double rx, ry;

		int enable;
	} viewport;

	pixman_region32_t region;
	pixman_region32_t boundingbox;
	pixman_region32_t opaque, blend;
	pixman_region32_t input;
	pixman_region32_t damage;
	int dirty;
	int needs_flush;
	int needs_filter;
	int needs_redraw;
	int needs_full_upload;

	nemotale_node_dispatch_flush_t dispatch_flush;
	nemotale_node_dispatch_filter_t dispatch_filter;
	nemotale_node_dispatch_resize_t dispatch_resize;
	nemotale_node_dispatch_viewport_t dispatch_viewport;

	struct {
		int enable;
		int dirty;
		int custom;

		float cosr, sinr;

		struct nemomatrix matrix, inverse;
	} transform;

	double alpha;

	int has_filter;
	int has_smooth;
};

#define	NTNODE_OBJECT(node)						(&node->object)
#define	NTNODE_DESTROY_SIGNAL(node)		(&node->destroy_signal)
#define	NTNODE_DAMAGE(node)						(&node->damage)

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

static inline void nemotale_node_input(struct talenode *node, int32_t x, int32_t y, int32_t width, int32_t height)
{
	pixman_region32_init_rect(&node->input, x, y, width + 1, height + 1);
}

static inline void nemotale_node_damage(struct talenode *node, int32_t x, int32_t y, int32_t width, int32_t height)
{
	pixman_region32_union_rect(&node->damage, &node->damage, x, y, width, height);
	node->dirty = 1;
	node->needs_flush = 1;
}

static inline void nemotale_node_damage_region(struct talenode *node, pixman_region32_t *region)
{
	pixman_region32_union(&node->damage, &node->damage, region);
	node->dirty = 1;
	node->needs_flush = 1;
}

static inline void nemotale_node_damage_all(struct talenode *node)
{
	pixman_region32_union_rect(&node->damage, &node->damage, 0, 0, node->geometry.width, node->geometry.height);
	node->dirty = 1;
	node->needs_flush = 1;
}

static inline void nemotale_node_damage_filter(struct talenode *node)
{
	node->needs_filter = 1;
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

static inline int nemotale_node_is_mapped(struct talenode *node)
{
	return node->tale != NULL;
}

static inline void nemotale_node_set_alpha(struct talenode *node, double alpha)
{
	node->alpha = alpha;

	node->needs_redraw = 1;
}

static inline void nemotale_node_set_smooth(struct talenode *node, int has_smooth)
{
	node->has_smooth = has_smooth;

	node->needs_redraw = 1;
}

static inline void nemotale_node_set_id(struct talenode *node, uint32_t id)
{
	node->id = id;
}

static inline uint32_t nemotale_node_get_id(struct talenode *node)
{
	return node == NULL ? 0 : node->id;
}

static inline int32_t nemotale_node_get_width(struct talenode *node)
{
	return node->geometry.width;
}

static inline int32_t nemotale_node_get_height(struct talenode *node)
{
	return node->geometry.height;
}

static inline int32_t nemotale_node_get_viewport_width(struct talenode *node)
{
	return node->viewport.width;
}

static inline int32_t nemotale_node_get_viewport_height(struct talenode *node)
{
	return node->viewport.height;
}

static inline void nemotale_node_set_data(struct talenode *node, void *data)
{
	node->userdata = data;
}

static inline void *nemotale_node_get_data(struct talenode *node)
{
	return node->userdata;
}

static inline void nemotale_node_transform_to_viewport(struct talenode *node, float x, float y, float *sx, float *sy)
{
	if (node->viewport.enable != 0) {
		*sx = x * node->viewport.sx;
		*sy = y * node->viewport.sy;
	} else {
		*sx = x;
		*sy = y;
	}
}

static inline void nemotale_node_transform_from_viewport(struct talenode *node, float sx, float sy, float *x, float *y)
{
	if (node->viewport.enable != 0) {
		*x = sx * node->viewport.rx;
		*y = sy * node->viewport.ry;
	} else {
		*x = sx;
		*y = sy;
	}
}

static inline int nemotale_node_flush(struct talenode *node)
{
	return node->dispatch_flush(node);
}

static inline int nemotale_node_filter(struct talenode *node)
{
	return node->dispatch_filter(node);
}

static inline int nemotale_node_resize(struct talenode *node, int32_t width, int32_t height)
{
	return node->dispatch_resize(node, width, height);
}

static inline int nemotale_node_viewport(struct talenode *node, int32_t width, int32_t height)
{
	return node->dispatch_viewport(node, width, height);
}

static inline int nemotale_node_needs_full_upload(struct talenode *node)
{
	return node->needs_full_upload;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
