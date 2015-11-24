#ifndef __NEMOSHOW_ITEM_HPP__
#define __NEMOSHOW_ITEM_HPP__

#include <showitem.h>

typedef struct _showitem {
	SkPaint *fill;
	SkPaint *stroke;

	SkMatrix *matrix;
	SkMatrix *inverse;
	SkMatrix *modelview;
	SkMatrix *viewbox;
	bool has_inverse;

	SkPath *path;
	SkBitmap *bitmap;
	uint32_t width, height;

	SkPath *strokepath;
	SkPath *fillpath;

	SkPoint *points;

	SkTextBox *textbox;
} showitem_t;

#define NEMOSHOW_ITEM_CC(base, name)				(((showitem_t *)((base)->cc))->name)

static inline SkPath *nemoshow_item_get_skia_path(struct showone *one)
{
	return NEMOSHOW_ITEM_CC(NEMOSHOW_ITEM(one), path);
}

#endif
