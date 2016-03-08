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

typedef enum {
	NEMOTALE_PICK_DEFAULT_TYPE = 0,
	NEMOTALE_PICK_CUSTOM_TYPE = 1,
	NEMOTALE_PICK_NO_TYPE = 2,
	NEMOTALE_PICK_LAST_TYPE
} NemoTalePickType;

typedef int (*nemotale_node_pick_t)(struct talenode *node, double x, double y, void *data);

struct nemotale;
struct talenode;

struct talenode {
	struct nemosignal destroy_signal;

	void *pmcontext;
	void *glcontext;

	void *userdata;
	void *pickdata;

	int picktype;
	nemotale_node_pick_t pick;

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
	int needs_full_upload;

	struct {
		int enable;
		int dirty;
		int custom;

		float cosr, sinr;

		struct nemomatrix matrix, inverse;
	} transform;

	double alpha;

	int has_filter;
};

#define	NTNODE_OBJECT(node)						(&node->object)
#define	NTNODE_DESTROY_SIGNAL(node)		(&node->destroy_signal)
#define	NTNODE_DAMAGE(node)						(&node->damage)

extern void nemotale_node_destroy(struct talenode *node);

extern int nemotale_node_prepare(struct talenode *node);
extern void nemotale_node_finish(struct talenode *node);

extern void nemotale_node_boundingbox_update(struct talenode *node, int32_t x, int32_t y, int32_t width, int32_t height, pixman_region32_t *bbox);
extern void nemotale_node_transform_update(struct talenode *node);

extern void nemotale_node_set_picker(struct talenode *node, nemotale_node_pick_t pick, void *data);

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
}

static inline void nemotale_node_set_id(struct talenode *node, uint32_t id)
{
	node->id = id;
}

static inline uint32_t nemotale_node_get_id(struct talenode *node)
{
	return node == NULL ? 0 : node->id;
}

static inline void nemotale_node_set_pick_type(struct talenode *node, int type)
{
	node->picktype = type;
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

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
