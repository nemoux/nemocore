#ifndef	__NEMOSHOW_ONE_H__
#define	__NEMOSHOW_ONE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemoattr.h>
#include <nemoxml.h>
#include <nemolist.h>
#include <nemolistener.h>

#define NEMOSHOW_ID_MAX						(32)
#define NEMOSHOW_NAME_MAX					(32)
#define NEMOSHOW_ATTR_MAX					(48)
#define NEMOSHOW_ATTR_NAME_MAX		(32)
#define NEMOSHOW_SYMBOL_MAX				(32)
#define NEMOSHOW_SIGNAL_NAME_MAX	(32)

typedef enum {
	NEMOSHOW_NONE_TYPE = 0,
	NEMOSHOW_SHOW_TYPE = 1,
	NEMOSHOW_SCENE_TYPE = 2,
	NEMOSHOW_CANVAS_TYPE = 3,
	NEMOSHOW_ITEM_TYPE = 4,
	NEMOSHOW_EASE_TYPE = 5,
	NEMOSHOW_SEQUENCE_TYPE = 6,
	NEMOSHOW_FRAME_TYPE = 7,
	NEMOSHOW_SET_TYPE = 8,
	NEMOSHOW_SVG_TYPE = 9,
	NEMOSHOW_MATRIX_TYPE = 10,
	NEMOSHOW_PATH_TYPE = 11,
	NEMOSHOW_PATHCMD_TYPE = 12,
	NEMOSHOW_VAR_TYPE = 13,
	NEMOSHOW_FILTER_TYPE = 14,
	NEMOSHOW_SHADER_TYPE = 15,
	NEMOSHOW_STOP_TYPE = 16,
	NEMOSHOW_CONS_TYPE = 17,
	NEMOSHOW_FONT_TYPE = 18,
	NEMOSHOW_DEFS_TYPE = 19,
	NEMOSHOW_POLY_TYPE = 20,
	NEMOSHOW_PIPE_TYPE = 21,
	NEMOSHOW_LAST_TYPE
} NemoShowOneType;

typedef enum {
	NEMOSHOW_CLIP_REF = 0,
	NEMOSHOW_FILTER_REF = 1,
	NEMOSHOW_SHADER_REF = 2,
	NEMOSHOW_MATRIX_REF = 3,
	NEMOSHOW_PATH_REF = 4,
	NEMOSHOW_FONT_REF = 5,
	NEMOSHOW_CANVAS_REF = 6,
	NEMOSHOW_BITMAP_REF = 7,
	NEMOSHOW_LAST_REF
} NemoShowOneRef;

typedef enum {
	NEMOSHOW_FILL_STATE = (1 << 0),
	NEMOSHOW_STROKE_STATE = (1 << 1),
	NEMOSHOW_TRANSFORM_STATE = (1 << 2),
	NEMOSHOW_SIZE_STATE = (1 << 3),
	NEMOSHOW_ANCHOR_STATE = (1 << 4),
	NEMOSHOW_VIEWPORT_STATE = (1 << 5),
	NEMOSHOW_INHERIT_STATE = (1 << 8),
	NEMOSHOW_BOUNDS_STATE = (1 << 9),
	NEMOSHOW_EFFECT_STATE = (1 << 10),
	NEMOSHOW_CLIP_STATE = (1 << 11),
	NEMOSHOW_PICK_STATE = (1 << 12),
	NEMOSHOW_REDRAW_STATE = (1 << 13),
} NemoShowOneState;

typedef enum {
	NEMOSHOW_NONE_DIRTY = (0 << 0),
	NEMOSHOW_REDRAW_DIRTY = (1 << 0),
	NEMOSHOW_SHAPE_DIRTY = (1 << 1),
	NEMOSHOW_STYLE_DIRTY = (1 << 2),
	NEMOSHOW_TEXT_DIRTY = (1 << 3),
	NEMOSHOW_FONT_DIRTY = (1 << 4),
	NEMOSHOW_PATH_DIRTY = (1 << 5),
	NEMOSHOW_PATHEFFECT_DIRTY = (1 << 6),
	NEMOSHOW_MATRIX_DIRTY = (1 << 7),
	NEMOSHOW_FILTER_DIRTY = (1 << 8),
	NEMOSHOW_SHADER_DIRTY = (1 << 9),
	NEMOSHOW_URI_DIRTY = (1 << 10),
	NEMOSHOW_CANVAS_DIRTY = (1 << 11),
	NEMOSHOW_SIZE_DIRTY = (1 << 12),
	NEMOSHOW_POINTS_DIRTY = (1 << 13),
	NEMOSHOW_BOUNDS_DIRTY = (1 << 14),
	NEMOSHOW_BITMAP_DIRTY = (1 << 15),
	NEMOSHOW_CHILDREN_DIRTY = (1 << 16),
	NEMOSHOW_VIEWPORT_DIRTY = (1 << 17),
	NEMOSHOW_ALL_DIRTY = NEMOSHOW_REDRAW_DIRTY | NEMOSHOW_SHAPE_DIRTY | NEMOSHOW_STYLE_DIRTY | NEMOSHOW_TEXT_DIRTY | NEMOSHOW_FONT_DIRTY | NEMOSHOW_PATH_DIRTY | NEMOSHOW_PATHEFFECT_DIRTY | NEMOSHOW_MATRIX_DIRTY | NEMOSHOW_FILTER_DIRTY | NEMOSHOW_SHADER_DIRTY | NEMOSHOW_URI_DIRTY | NEMOSHOW_CANVAS_DIRTY | NEMOSHOW_SIZE_DIRTY | NEMOSHOW_POINTS_DIRTY | NEMOSHOW_BOUNDS_DIRTY | NEMOSHOW_BITMAP_DIRTY | NEMOSHOW_CHILDREN_DIRTY | NEMOSHOW_VIEWPORT_DIRTY
} NemoShowDirtyType;

typedef enum {
	NEMOSHOW_BLUE_COLOR = 0,
	NEMOSHOW_GREEN_COLOR = 1,
	NEMOSHOW_RED_COLOR = 2,
	NEMOSHOW_ALPHA_COLOR = 3
} NemoShowColor;

typedef enum {
	NEMOSHOW_NONE_TRANSFORM = (0 << 0),
	NEMOSHOW_EXTERN_TRANSFORM = (1 << 0),
	NEMOSHOW_INTERN_TRANSFORM = (1 << 1),
	NEMOSHOW_CHILDREN_TRANSFORM = (1 << 7) | (1 << 1),
	NEMOSHOW_DIRECT_TRANSFORM = (1 << 8) | (1 << 1),
	NEMOSHOW_TSR_TRANSFORM = (1 << 9) | (1 << 1),
	NEMOSHOW_LAST_TRANSFORM
} NemoShowTransform;

typedef enum {
	NEMOSHOW_BUTT_CAP = 0,
	NEMOSHOW_ROUND_CAP = 1,
	NEMOSHOW_SQUARE_CAP = 2,
	NEMOSHOW_LAST_CAP
} NemoShowCap;

typedef enum {
	NEMOSHOW_MITER_JOIN = 0,
	NEMOSHOW_ROUND_JOIN = 1,
	NEMOSHOW_BEVEL_JOIN = 2,
	NEMOSHOW_LAST_JOIN
} NemoShowJoin;

struct showone;

typedef int (*nemoshow_one_update_t)(struct showone *one);
typedef void (*nemoshow_one_destroy_t)(struct showone *one);
typedef void (*nemoshow_one_attach_t)(struct showone *parent, struct showone *one);
typedef void (*nemoshow_one_detach_t)(struct showone *one);
typedef int (*nemoshow_one_above_t)(struct showone *one, struct showone *above);
typedef int (*nemoshow_one_below_t)(struct showone *one, struct showone *below);
typedef int (*nemoshow_one_dattr_t)(struct showone *one, const char *attr, double value);
typedef int (*nemoshow_one_sattr_t)(struct showone *one, const char *attr, const char *value);
typedef int (*nemoshow_one_signal_t)(struct showone *one, const char *name, struct nemoobject *attrs);

struct showattr {
	char name[NEMOSHOW_ATTR_NAME_MAX];

	char *text;

	struct nemoattr *ref;
	uint32_t dirty;
};

struct showref {
	struct nemolist link;

	struct showone *src;
	uint32_t dirty;
	uint32_t state;
	struct showone *one;
	int index;
};

struct showevent {
	uint32_t dirty;
};

struct showsignal {
	struct nemolist link;

	char name[NEMOSHOW_SIGNAL_NAME_MAX];
	nemoshow_one_signal_t callback;
};

struct showone {
	int type, sub;
	char id[NEMOSHOW_ID_MAX];
	char name[NEMOSHOW_NAME_MAX];

	struct nemolist link;
	struct nemolist children_link;
	struct nemolist dirty_link;
	struct nemolist bounds_link;

	uint32_t state;

	uint32_t tag;
	uint32_t zone;

	struct nemosignal unpin_signal;
	struct nemosignal destroy_signal;

	struct nemolist children_list;
	struct nemolist reference_list;
	struct nemolist signal_list;

	struct nemoobject object;
	uint32_t serial;

	uint32_t dirty_serial;

	nemoshow_one_update_t update;
	nemoshow_one_destroy_t destroy;
	nemoshow_one_attach_t attach;
	nemoshow_one_detach_t detach;
	nemoshow_one_above_t above;
	nemoshow_one_below_t below;
	nemoshow_one_dattr_t dattr;
	nemoshow_one_sattr_t sattr;

	struct nemoshow *show;

	struct showone *parent;
	struct showone *canvas;

	struct showref *refs[NEMOSHOW_LAST_REF];

	struct showattr **attrs;
	int nattrs, sattrs;

	uint32_t dirty;

	uint32_t effect;

	int32_t x, y, w, h;
	int32_t sx, sy, sw, sh;
	int32_t outer;

	int32_t x0, y0, x1, y1;

	void *context;
	void *userdata;
};

#define NEMOSHOW_REF(one, index)			(one->refs[index] != NULL ? one->refs[index]->src : NULL)

#define NEMOSHOW_POINTS_OFFSET_X(index)				(index * 2 + 0)
#define NEMOSHOW_POINTS_OFFSET_Y(index)				(index * 2 + 1)
#define NEMOSHOW_PATHARRAY_OFFSET_X0(index)		(index * 6 + 0)
#define NEMOSHOW_PATHARRAY_OFFSET_Y0(index)		(index * 6 + 1)
#define NEMOSHOW_PATHARRAY_OFFSET_X1(index)		(index * 6 + 2)
#define NEMOSHOW_PATHARRAY_OFFSET_Y1(index)		(index * 6 + 3)
#define NEMOSHOW_PATHARRAY_OFFSET_X2(index)		(index * 6 + 4)
#define NEMOSHOW_PATHARRAY_OFFSET_Y2(index)		(index * 6 + 5)

#define nemoshow_children_for_each(child, one)	\
	nemolist_for_each(child, &((one)->children_list), children_link)
#define nemoshow_children_for_each_reverse(child, one)	\
	nemolist_for_each_reverse(child, &((one)->children_list), children_link)
#define nemoshow_children_for_each_safe(child, tmp, one)	\
	nemolist_for_each_safe(child, tmp, &((one)->children_list), children_link)
#define nemoshow_children0(one)	\
	nemolist_node0(&((one)->children_list), struct showone, children_link)
#define nemoshow_children_next(one, child)	\
	nemolist_next(&((one)->children_list), child, struct showone, children_link)
#define nemoshow_children_empty(one)	\
	nemolist_empty(&((one)->children_list))
#define nemoshow_children_count(one)	\
	nemolist_length(&((one)->children_list))

extern void nemoshow_one_prepare(struct showone *one);
extern void nemoshow_one_finish(struct showone *one);

extern struct showone *nemoshow_one_create(int type);
extern void nemoshow_one_destroy(struct showone *one);

extern void nemoshow_one_dirty(struct showone *one, uint32_t dirty);
extern void nemoshow_one_dirty_all(struct showone *one, uint32_t dirty);
extern void nemoshow_one_bounds(struct showone *one);

extern void nemoshow_one_update_bounds(struct showone *one);

extern void nemoshow_one_attach_one(struct showone *parent, struct showone *one);
extern void nemoshow_one_detach_one(struct showone *one);

extern int nemoshow_one_above_one(struct showone *one, struct showone *above);
extern int nemoshow_one_below_one(struct showone *one, struct showone *below);

extern int nemoshow_one_reference_one(struct showone *one, struct showone *src, uint32_t dirty, uint32_t state, int index);
extern void nemoshow_one_unreference_one(struct showone *one, struct showone *src);
extern void nemoshow_one_unreference_all(struct showone *one);

extern struct showattr *nemoshow_one_create_attr(const char *name, const char *text, struct nemoattr *ref, uint32_t dirty);
extern void nemoshow_one_destroy_attr(struct showattr *attr);

extern struct showone *nemoshow_one_search_id(struct showone *one, const char *id);
extern struct showone *nemoshow_one_search_tag(struct showone *one, uint32_t tag);

extern int nemoshow_one_connect_signal(struct showone *one, const char *name, nemoshow_one_signal_t callback);
extern void nemoshow_one_disconnect_signal(struct showone *one, const char *name);
extern int nemoshow_one_emit_signal(struct showone *one, const char *name, struct nemoobject *attrs);

static inline void nemoshow_one_attach(struct showone *parent, struct showone *one)
{
	if (one->attach != NULL) {
		one->attach(parent, one);
	} else {
		nemoshow_one_attach_one(parent, one);
	}
}

static inline void nemoshow_one_detach(struct showone *one)
{
	if (one->detach != NULL) {
		one->detach(one);
	} else {
		nemoshow_one_detach_one(one);
	}
}

static inline int nemoshow_one_above(struct showone *one, struct showone *above)
{
	if (one->above != NULL)
		return one->above(one, above);

	return nemoshow_one_above_one(one, above);
}

static inline int nemoshow_one_below(struct showone *one, struct showone *below)
{
	if (one->below != NULL)
		return one->below(one, below);

	return nemoshow_one_below_one(one, below);
}

static inline void nemoshow_one_update(struct showone *one)
{
	one->update(one);

	one->dirty = 0;

	one->dirty_serial = 0;

	nemolist_remove(&one->dirty_link);
	nemolist_init(&one->dirty_link);
}

static inline int nemoshow_one_is_type(struct showone *one, int type)
{
	return one->type == type;
}

static inline int nemoshow_one_is_subtype(struct showone *one, int type, int sub)
{
	return one->type == type && one->sub == sub;
}

static inline void nemoshow_one_set_state(struct showone *one, uint32_t state)
{
	one->state |= state;
}

static inline void nemoshow_one_put_state(struct showone *one, uint32_t state)
{
	one->state &= ~state;
}

static inline int nemoshow_one_has_state(struct showone *one, uint32_t state)
{
	return one->state & state;
}

static inline int nemoshow_one_has_state_all(struct showone *one, uint32_t state)
{
	return (one->state & state) == state;
}

static inline void nemoshow_one_set_effect(struct showone *one, uint32_t effect)
{
	one->effect = effect;
}

static inline void nemoshow_one_set_tag(struct showone *one, uint32_t tag)
{
	one->tag = tag;
}

static inline uint32_t nemoshow_one_get_tag(struct showone *one)
{
	return one == NULL ? 0 : one->tag;
}

static inline void nemoshow_one_set_zone(struct showone *one, uint32_t zone)
{
	one->zone = zone;
}

static inline uint32_t nemoshow_one_get_zone(struct showone *one)
{
	return one == NULL ? 0 : one->zone;
}

static inline void nemoshow_one_set_dispatch_destroy(struct showone *one, nemoshow_one_destroy_t destroy)
{
	one->destroy = destroy;
}

static inline void nemoshow_one_set_dispatch_dattr(struct showone *one, nemoshow_one_dattr_t dattr)
{
	one->dattr = dattr;
}

static inline void nemoshow_one_set_dispatch_sattr(struct showone *one, nemoshow_one_sattr_t sattr)
{
	one->sattr = sattr;
}

static inline void nemoshow_one_setd(struct showone *one, const char *attr, double value)
{
	nemoobject_setd(&one->object, attr, value);

	if (one->dattr != NULL)
		one->dattr(one, attr, value);
}

static inline double nemoshow_one_getd(struct showone *one, const char *attr)
{
	return nemoobject_getd(&one->object, attr);
}

static inline void nemoshow_one_sets(struct showone *one, const char *attr, const char *value)
{
	nemoobject_sets(&one->object, attr, value, strlen(value));

	if (one->sattr != NULL)
		one->sattr(one, attr, value);
}

static inline const char *nemoshow_one_gets(struct showone *one, const char *attr)
{
	return nemoobject_gets(&one->object, attr);
}

static inline void nemoshow_one_set_id(struct showone *one, const char *id)
{
	strncpy(one->id, id, NEMOSHOW_ID_MAX);
}

static inline const char *nemoshow_one_get_id(struct showone *one)
{
	return one == NULL ? NULL : one->id;
}

static inline void nemoshow_one_set_name(struct showone *one, const char *name)
{
	strncpy(one->name, name, NEMOSHOW_NAME_MAX);
}

static inline const char *nemoshow_one_get_name(struct showone *one)
{
	return one == NULL ? NULL : one->name;
}

static inline int nemoshow_one_has_name(struct showone *one, const char *name)
{
	return one == NULL ? 0 : strcmp(one->name, name) == 0;
}

static inline struct showone *nemoshow_one_get_parent(struct showone *one, int type, int sub)
{
	struct showone *parent;

	if (sub != 0) {
		for (parent = one->parent;
				parent != NULL && (parent->type != type || parent->sub != sub);
				parent = parent->parent);
	} else {
		for (parent = one->parent;
				parent != NULL && parent->type != type;
				parent = parent->parent);
	}

	return parent;
}

static inline struct nemoshow *nemoshow_one_get_show(struct showone *one)
{
	return one->show;
}

static inline void nemoshow_one_set_context(struct showone *one, void *context)
{
	one->context = context;
}

static inline void *nemoshow_one_get_context(struct showone *one)
{
	return one->context;
}

static inline void nemoshow_one_set_userdata(struct showone *one, void *userdata)
{
	one->userdata = userdata;
}

static inline void *nemoshow_one_get_userdata(struct showone *one)
{
	return one->userdata;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
