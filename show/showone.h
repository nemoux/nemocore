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

#define NEMOSHOW_ID_MAX					(32)
#define NEMOSHOW_ATTR_MAX				(48)
#define NEMOSHOW_ATTR_NAME_MAX	(32)
#define NEMOSHOW_SYMBOL_MAX			(32)

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
	NEMOSHOW_FIX_TYPE = 9,
	NEMOSHOW_FOLLOW_TYPE = 10,
	NEMOSHOW_SVG_TYPE = 11,
	NEMOSHOW_MATRIX_TYPE = 12,
	NEMOSHOW_PATH_TYPE = 13,
	NEMOSHOW_CAMERA_TYPE = 14,
	NEMOSHOW_VAR_TYPE = 15,
	NEMOSHOW_FILTER_TYPE = 16,
	NEMOSHOW_SHADER_TYPE = 17,
	NEMOSHOW_STOP_TYPE = 18,
	NEMOSHOW_CONS_TYPE = 19,
	NEMOSHOW_FONT_TYPE = 20,
	NEMOSHOW_DEFS_TYPE = 21,
	NEMOSHOW_LAST_TYPE
} NemoShowOneType;

typedef enum {
	NEMOSHOW_STYLE_REF = 0,
	NEMOSHOW_CLIP_REF = 1,
	NEMOSHOW_FILTER_REF = 2,
	NEMOSHOW_SHADER_REF = 3,
	NEMOSHOW_MATRIX_REF = 4,
	NEMOSHOW_PATH_REF = 5,
	NEMOSHOW_GROUP_REF = 6,
	NEMOSHOW_FONT_REF = 7,
	NEMOSHOW_HEAD_REF = 8,
	NEMOSHOW_TAIL_REF = 9,
	NEMOSHOW_LAST_REF
} NemoShowOneRef;

typedef enum {
	NEMOSHOW_ARRANGE_STATE = (1 << 0),
	NEMOSHOW_TRANSITION_STATE = (1 << 1),
	NEMOSHOW_LAST_STATE
} NemoShowOneState;

typedef enum {
	NEMOSHOW_NONE_DIRTY = (0 << 0),
	NEMOSHOW_SHAPE_DIRTY = (1 << 0),
	NEMOSHOW_STYLE_DIRTY = (1 << 1),
	NEMOSHOW_CHILD_DIRTY = (1 << 2),
	NEMOSHOW_TEXT_DIRTY = (1 << 3),
	NEMOSHOW_FONT_DIRTY = (1 << 4),
	NEMOSHOW_MATRIX_DIRTY = (1 << 5),
	NEMOSHOW_FILTER_DIRTY = (1 << 6),
	NEMOSHOW_SHADER_DIRTY = (1 << 7),
	NEMOSHOW_URI_DIRTY = (1 << 8),
	NEMOSHOW_ALL_DIRTY = NEMOSHOW_SHAPE_DIRTY | NEMOSHOW_STYLE_DIRTY | NEMOSHOW_CHILD_DIRTY | NEMOSHOW_TEXT_DIRTY | NEMOSHOW_FONT_DIRTY | NEMOSHOW_MATRIX_DIRTY | NEMOSHOW_FILTER_DIRTY | NEMOSHOW_SHADER_DIRTY | NEMOSHOW_URI_DIRTY
} NemoShowDirtyType;

struct nemoshow;
struct showone;

typedef int (*nemoshow_one_update_t)(struct nemoshow *show, struct showone *one);
typedef void (*nemoshow_one_destroy_t)(struct showone *one);

struct showattr {
	char name[NEMOSHOW_ATTR_NAME_MAX];

	char *text;

	struct nemoattr *ref;
	uint32_t dirty;
};

struct showone {
	int type, sub;
	char id[NEMOSHOW_ID_MAX];

	uint32_t state;

	struct nemosignal destroy_signal;

	struct nemoobject object;
	uint32_t serial;

	nemoshow_one_update_t update;
	nemoshow_one_destroy_t destroy;

	struct nemoshow *show;
	struct showone *parent;

	struct showone **children;
	int nchildren, schildren;

	struct showone **refs;
	int nrefs, srefs;

	struct showone *rrefs[NEMOSHOW_LAST_REF];

	struct showattr **attrs;
	int nattrs, sattrs;

	uint32_t dirty;

	int32_t x, y, width, height;
	int32_t outer;

	double ax, ay;

	void *userdata;
};

#define	NEMOSHOW_REF(one, index)			(one->rrefs[index])

extern void nemoshow_one_prepare(struct showone *one);
extern void nemoshow_one_finish(struct showone *one);

extern struct showone *nemoshow_one_create(int type);
extern void nemoshow_one_destroy(struct showone *one);
extern void nemoshow_one_destroy_with_children(struct showone *one);

extern void nemoshow_one_attach_one(struct showone *parent, struct showone *one);
extern void nemoshow_one_detach_one(struct showone *parent, struct showone *one);

extern void nemoshow_one_above_one(struct showone *one, struct showone *above);
extern void nemoshow_one_below_one(struct showone *one, struct showone *below);

extern void nemoshow_one_reference_one(struct showone *one, struct showone *ref, int index);
extern void nemoshow_one_unreference_one(struct showone *one, struct showone *ref);
extern void nemoshow_one_unreference_all(struct showone *one);

extern struct showattr *nemoshow_one_create_attr(const char *name, const char *text, struct nemoattr *ref, uint32_t dirty);
extern void nemoshow_one_destroy_attr(struct showattr *attr);

extern void nemoshow_one_dump(struct showone *one, FILE *out);

static inline void nemoshow_one_dirty(struct showone *one, uint32_t dirty)
{
	if ((one->dirty & dirty) == dirty)
		return;

	one->dirty |= dirty;

	if (one->parent != NULL)
		nemoshow_one_dirty(one->parent, NEMOSHOW_CHILD_DIRTY);

	if (one->nrefs > 0) {
		int i;

		for (i = 0; i < one->nrefs; i++)
			nemoshow_one_dirty(one->refs[i], dirty);
	}
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

static inline void nemoshow_one_update(struct nemoshow *show, struct showone *one)
{
	if (one->dirty != 0) {
		int i;

		for (i = 0; i < one->nchildren; i++)
			nemoshow_one_update(show, one->children[i]);
	}

	if (one->dirty != 0) {
		one->update(show, one);

		one->dirty = 0;
	}
}

static inline void nemoshow_one_update_alone(struct nemoshow *show, struct showone *one)
{
	if (one->dirty != 0) {
		one->update(show, one);

		one->dirty = 0;
	}
}

static inline void nemoshow_one_update_with_dirty(struct nemoshow *show, struct showone *one, uint32_t dirty)
{
	int i;

	for (i = 0; i < one->nchildren; i++)
		nemoshow_one_update_with_dirty(show, one->children[i], dirty);

	one->dirty = dirty;

	one->update(show, one);

	one->dirty = 0;
}

static inline void nemoshow_one_setd(struct showone *one, const char *attr, double value)
{
	nemoobject_setd(&one->object, attr, value);
}

static inline double nemoshow_one_getd(struct showone *one, const char *attr)
{
	return nemoobject_getd(&one->object, attr);
}

static inline void nemoshow_one_sets(struct showone *one, const char *attr, const char *value)
{
	nemoobject_sets(&one->object, attr, value, strlen(value));
}

static inline const char *nemoshow_one_gets(struct showone *one, const char *attr)
{
	return nemoobject_gets(&one->object, attr);
}

static inline void nemoshow_one_set_id(struct showone *one, const char *id)
{
	strncpy(one->id, id, NEMOSHOW_ID_MAX);
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

static inline struct showone *nemoshow_one_get_child(struct showone *one, uint32_t index)
{
	if (index >= one->nchildren)
		return NULL;

	return one->children[index];
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
