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
	NEMOSHOW_LOOP_TYPE = 9,
	NEMOSHOW_MATRIX_TYPE = 10,
	NEMOSHOW_PATH_TYPE = 11,
	NEMOSHOW_CAMERA_TYPE = 12,
	NEMOSHOW_LAST_TYPE
} NemoShowOneType;

struct nemoshow;
struct showone;

typedef int (*nemoshow_one_update_t)(struct nemoshow *show, struct showone *one);
typedef void (*nemoshow_one_destroy_t)(struct showone *one);

struct showattr {
	char name[NEMOSHOW_ATTR_NAME_MAX];

	char *text;

	struct nemoattr *ref;
};

struct showone {
	int type, sub;
	char id[NEMOSHOW_ID_MAX];

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

	int dirty;
	int32_t x, y, width, height;
};

extern void nemoshow_one_prepare(struct showone *one);
extern void nemoshow_one_finish(struct showone *one);

extern void nemoshow_one_destroy(struct showone *one);

extern struct showattr *nemoshow_one_create_attr(const char *name, const char *text, struct nemoattr *ref);
extern void nemoshow_one_destroy_attr(struct showattr *attr);

extern void nemoshow_one_dirty(struct showone *one);

extern void nemoshow_one_dump(struct showone *one, FILE *out);

static inline void nemoshow_one_update(struct nemoshow *show, struct showone *one)
{
	if (one->dirty != 0) {
		one->update(show, one);

		one->dirty = 0;
	}
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

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
