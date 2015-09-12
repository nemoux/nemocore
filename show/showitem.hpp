#ifndef __NEMOSHOW_ITEM_HPP__
#define __NEMOSHOW_ITEM_HPP__

#include <skiaconfig.hpp>

#include <showitem.h>

typedef struct _showitem {
	SkPaint *fill;
	SkPaint *stroke;

	SkMatrix *matrix;
	SkMatrix *viewbox;

	SkPath *path;
	SkBitmap *bitmap;

	SkPoint *points;
} showitem_t;

#define NEMOSHOW_ITEM_CC(base, name)				(((showitem_t *)((base)->cc))->name)

static inline SkPath *nemoshow_item_get_skia_path(struct showone *one)
{
	return NEMOSHOW_ITEM_CC(NEMOSHOW_ITEM(one), path);
}

#endif
