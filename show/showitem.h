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
	NEMOSHOW_STYLE_ITEM = 3,
	NEMOSHOW_LAST_ITEM
} NemoShowItemType;

struct showmatrix;

struct showitem {
	struct showone base;

	double x, y;
	double width, height;
	double r;

	double from, to;

	uint32_t stroke;
	double strokes[4];
	double stroke_width;
	uint32_t fill;
	double fills[4];

	double alpha;

	char style[NEMOSHOW_ID_MAX];
	struct showitem *stone;

	char matrix[NEMOSHOW_ID_MAX];
	struct showmatrix *mtone;

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
