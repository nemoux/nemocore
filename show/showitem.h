#ifndef	__NEMOSHOW_ITEM_H__
#define	__NEMOSHOW_ITEM_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <showone.h>

typedef enum {
	NEMOSHOW_NONE_ITEM = 0,
	NEMOSHOW_RECT_ITEM = 1,
	NEMOSHOW_TEXT_ITEM = 2,
	NEMOSHOW_LAST_ITEM
} NemoShowItemType;

struct showitem {
	struct showone base;

	double x, y;
	double width, height;
	double alpha;
};

#define NEMOSHOW_ITEM(one)			((struct showitem *)container_of(one, struct showitem, base))

extern struct showone *nemoshow_item_create(int type);
extern void nemoshow_item_destroy(struct showone *one);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
