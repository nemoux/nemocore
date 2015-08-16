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
	NEMOSHOW_RRECT_ITEM = 2,
	NEMOSHOW_CIRCLE_ITEM = 3,
	NEMOSHOW_ARC_ITEM = 4,
	NEMOSHOW_PIE_ITEM = 5,
	NEMOSHOW_TEXT_ITEM = 6,
	NEMOSHOW_PATH_ITEM = 7,
	NEMOSHOW_STYLE_ITEM = 8,
	NEMOSHOW_GROUP_ITEM = 9,
	NEMOSHOW_LAST_ITEM
} NemoShowItemType;

struct showmatrix;
struct showcanvas;

struct showitem {
	struct showone base;

	struct showone *canvas;
	struct showone *group;

	double x, y;
	double rx, ry;
	double width, height;
	double r;

	double from, to;

	uint32_t stroke;
	double strokes[4];
	double stroke_width;
	uint32_t fill;
	double fills[4];

	double alpha;

	struct showone *style;
	struct showone *blur;
	struct showone *shader;
	struct showone *matrix;
	struct showone *path;
	double length;

	struct showone *font;
	double fontsize;
	double fontascent;
	double fontdescent;
	const char *text;
	double textwidth, textheight;

	void *cc;
};

#define NEMOSHOW_ITEM(one)					((struct showitem *)container_of(one, struct showitem, base))
#define	NEMOSHOW_ITEM_AT(one, at)		(NEMOSHOW_ITEM(one)->at)

extern struct showone *nemoshow_item_create(int type);
extern void nemoshow_item_destroy(struct showone *one);

extern int nemoshow_item_arrange(struct nemoshow *show, struct showone *one);
extern int nemoshow_item_update(struct nemoshow *show, struct showone *one);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
