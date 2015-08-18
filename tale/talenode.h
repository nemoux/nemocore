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

struct talenode;

struct talenode {
	struct nemosignal destroy_signal;

	void *pmcontext;
	void *glcontext;

	void *userdata;
	void *pickdata;

	int picktype;
	nemotale_node_pick_t pick;

	uint32_t id;

	struct nemoobject object;

	struct {
		float x, y;
		int32_t width, height;
		float px, py;
		float r;
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
	int needs_full_upload;

	struct {
		int enable;
		int dirty;
		int custom;

		float cosr, sinr;

		struct nemomatrix matrix, inverse;
	} transform;
};

#define	NTNODE_OBJECT(node)						(&node->object)
#define	NTNODE_DESTROY_SIGNAL(node)		(&node->destroy_signal)
#define	NTNODE_DAMAGE(node)						(&node->damage)

extern void nemotale_node_destroy(struct talenode *node);

extern int nemotale_node_prepare(struct talenode *node);
extern void nemotale_node_finish(struct talenode *node);

extern void nemotale_node_opaque(struct talenode *node, int32_t x, int32_t y, int32_t width, int32_t height);
extern void nemotale_node_input(struct talenode *node, int32_t x, int32_t y, int32_t width, int32_t height);
extern void nemotale_node_damage(struct talenode *node, int32_t x, int32_t y, int32_t width, int32_t height);
extern void nemotale_node_damage_region(struct talenode *node, pixman_region32_t *region);
extern void nemotale_node_damage_all(struct talenode *node);
extern void nemotale_node_transform_to_global(struct talenode *node, float sx, float sy, float *x, float *y);
extern void nemotale_node_transform_from_global(struct talenode *node, float x, float y, float *sx, float *sy);
extern void nemotale_node_boundingbox_update(struct talenode *node, int32_t x, int32_t y, int32_t width, int32_t height, pixman_region32_t *bbox);
extern void nemotale_node_transform_update(struct talenode *node);

extern void nemotale_node_set_picker(struct talenode *node, nemotale_node_pick_t pick, void *data);

extern void nemotale_node_translate(struct talenode *node, float x, float y);
extern void nemotale_node_rotate(struct talenode *node, float r);
extern void nemotale_node_pivot(struct talenode *node, float px, float py);
extern int nemotale_node_transform(struct talenode *node, float d[9]);

static inline void nemotale_node_dirty(struct talenode *node)
{
	node->dirty = 1;
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
