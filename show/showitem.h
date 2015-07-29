#ifndef	__NEMOSHOW_ITEM_H__
#define	__NEMOSHOW_ITEM_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <showone.h>

#define	NEMOSHOW_COLOR_STRING_MAX				(32)

typedef enum {
	NEMOSHOW_NONE_ITEM = 0,
	NEMOSHOW_RECT_ITEM = 1,
	NEMOSHOW_TEXT_ITEM = 2,
	NEMOSHOW_STYLE_ITEM = 3,
	NEMOSHOW_LAST_ITEM
} NemoShowItemType;

struct showitem {
	struct showone base;

	double x, y;
	double width, height;
	double r;

	double from, to;

	char stroke[NEMOSHOW_COLOR_STRING_MAX];
	double stroke_width;
	char fill[NEMOSHOW_COLOR_STRING_MAX];

	double alpha;

	char style[NEMOSHOW_ID_MAX];
	struct showitem *stone;

	void *cc;
};

#define NEMOSHOW_ITEM(one)			((struct showitem *)container_of(one, struct showitem, base))

extern struct showone *nemoshow_item_create(int type);
extern void nemoshow_item_destroy(struct showone *one);

extern int nemoshow_item_arrange(struct nemoshow *show, struct showone *one);
extern int nemoshow_item_update(struct nemoshow *show, struct showone *one);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
