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
#define NEMOSHOW_ATTR_MAX				(32)
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
	NEMOSHOW_FOLLOW_TYPE = 9,
	NEMOSHOW_LOOP_TYPE = 10,
	NEMOSHOW_MATRIX_TYPE = 11,
	NEMOSHOW_PATH_TYPE = 12,
	NEMOSHOW_CAMERA_TYPE = 13,
	NEMOSHOW_VAR_TYPE = 14,
	NEMOSHOW_BLUR_TYPE = 15,
	NEMOSHOW_SHADER_TYPE = 16,
	NEMOSHOW_STOP_TYPE = 17,
	NEMOSHOW_CONS_TYPE = 18,
	NEMOSHOW_LAST_TYPE
} NemoShowOneType;

typedef enum {
	NEMOSHOW_NORMAL_STATE = 0,
	NEMOSHOW_TRANSITION_STATE = 1,
	NEMOSHOW_LAST_STATE
} NemoShowOneState;

typedef enum {
	NEMOSHOW_NONE_DIRTY = (0 << 0),
	NEMOSHOW_SHAPE_DIRTY = (1 << 0),
	NEMOSHOW_STYLE_DIRTY = (1 << 1),
	NEMOSHOW_CHILD_DIRTY = (1 << 2),
	NEMOSHOW_TEXT_DIRTY = (1 << 3),
	NEMOSHOW_ALL_DIRTY = NEMOSHOW_SHAPE_DIRTY | NEMOSHOW_STYLE_DIRTY | NEMOSHOW_CHILD_DIRTY | NEMOSHOW_TEXT_DIRTY
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

	int state;

	struct nemoobject object;
	uint32_t serial;

	nemoshow_one_update_t update;
	nemoshow_one_destroy_t destroy;

	struct showone *parent;

	struct showone **children;
	int nchildren, schildren;

	struct showone **refs;
	int nrefs, srefs;

	struct showattr **attrs;
	int nattrs, sattrs;

	uint32_t dirty;

	int32_t x, y, width, height;
};

extern void nemoshow_one_prepare(struct showone *one);
extern void nemoshow_one_finish(struct showone *one);

extern void nemoshow_one_destroy(struct showone *one);

extern struct showattr *nemoshow_one_create_attr(const char *name, const char *text, struct nemoattr *ref, uint32_t dirty);
extern void nemoshow_one_destroy_attr(struct showattr *attr);

extern void nemoshow_one_dump(struct showone *one, FILE *out);

static inline void nemoshow_one_dirty(struct showone *one, uint32_t dirty)
{
	if ((one->dirty & dirty) != 0)
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

static inline void nemoshow_one_update(struct nemoshow *show, struct showone *one)
{
	if (one->dirty != 0) {
		int i;

		for (i = 0; i < one->nchildren; i++)
			nemoshow_one_update(show, one->children[i]);

		one->update(show, one);

		one->dirty = 0;
	}
}

static inline void nemoshow_one_setd(struct showone *one, const char *attr, double value, uint32_t dirty)
{
	nemoobject_setd(&one->object, attr, value);

	nemoshow_one_dirty(one, dirty);
}

static inline double nemoshow_one_getd(struct showone *one, const char *attr)
{
	return nemoobject_getd(&one->object, attr);
}

static inline void nemoshow_one_sets(struct showone *one, const char *attr, const char *value, uint32_t dirty)
{
	nemoobject_sets(&one->object, attr, value, strlen(value));

	nemoshow_one_dirty(one, dirty);
}

static inline const char *nemoshow_one_gets(struct showone *one, const char *attr)
{
	return nemoobject_gets(&one->object, attr);
}

static inline struct showone *nemoshow_one_get_canvas(struct showone *one)
{
	struct showone *parent;

	for (parent = one->parent;
			parent != NULL && parent->type != NEMOSHOW_CANVAS_TYPE;
			parent = parent->parent);

	return parent;
}

static inline struct showone *nemoshow_one_get_child(struct showone *one, uint32_t index)
{
	if (index >= one->nchildren)
		return NULL;

	return one->children[index];
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
