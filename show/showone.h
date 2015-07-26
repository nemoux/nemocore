#ifndef	__NEMOSHOW_ONE_H__
#define	__NEMOSHOW_ONE_H__

#include <stdint.h>

#include <nemoattr.h>
#include <nemoxml.h>
#include <nemolist.h>
#include <nemolistener.h>

#define	NEMOSHOW_ID_MAX					(32)
#define	NEMOSHOW_ATTR_MAX				(32)
#define	NEMOSHOW_ATTR_NAME_MAX	(32)

typedef enum {
	NEMOSHOW_NONE_TYPE = 0,
	NEMOSHOW_SHOW_TYPE = 1,
	NEMOSHOW_SCENE_TYPE = 2,
	NEMOSHOW_CANVAS_TYPE = 3,
	NEMOSHOW_SHAPE_TYPE = 4,
	NEMOSHOW_EASE_TYPE = 5,
	NEMOSHOW_SEQUENCE_TYPE = 6,
	NEMOSHOW_LOOP_TYPE = 7,
	NEMOSHOW_LAST_TYPE
} NemoShowOneType;

typedef enum {
	NEMOSHOW_NONE_ATTR = 0,
	NEMOSHOW_DOUBLE_ATTR = 1,
	NEMOSHOW_INTEGER_ATTR = 2,
	NEMOSHOW_STRING_ATTR = 3,
	NEMOSHOW_LAST_ATTR
} NemoShowAttrType;

struct showone;

typedef void (*nemoshow_one_destroy_t)(struct showone *one);

struct showone {
	int type;
	char id[NEMOSHOW_ID_MAX];

	struct nemoobject object;

	struct nemolist link;

	nemoshow_one_destroy_t destroy;
};

struct showattr {
	char name[NEMOSHOW_ATTR_NAME_MAX];

	int type;
};

extern struct showattr nemoshow_one_attrs[];

extern void nemoshow_one_prepare(struct showone *one);
extern void nemoshow_one_finish(struct showone *one);

extern void nemoshow_one_destroy(struct showone *one);

extern void nemoshow_one_parse_xml(struct showone *one, struct xmlnode *node);

extern struct showattr *nemoshow_one_get_attr(const char *name);

#endif
