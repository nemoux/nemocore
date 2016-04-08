#ifndef __NEMOSHOW_ITEM_HPP__
#define __NEMOSHOW_ITEM_HPP__

#include <skiaconfig.hpp>

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
	SkPath *fillpath;

	SkBitmap *bitmap;
	uint32_t width, height;

	bool needs_free;

	SkPathMeasure *measure;

	SkPoint *points;

	SkTextBox *textbox;
} showitem_t;

#define NEMOSHOW_ITEM_CC(base, name)				(((showitem_t *)((base)->cc))->name)
#define NEMOSHOW_ITEM_ATCC(one, name)				(NEMOSHOW_ITEM_CC(NEMOSHOW_ITEM(one), name))

static inline SkPath *nemoshow_item_get_skia_path(struct showone *one)
{
	return NEMOSHOW_ITEM_CC(NEMOSHOW_ITEM(one), path);
}

extern int nemoshow_item_set_bitmap(struct showone *one, SkBitmap *bitmap);

#endif
