#ifndef __NEMOSHOW_ITEM_HPP__
#define __NEMOSHOW_ITEM_HPP__

#include <skiaconfig.hpp>

typedef struct _showitem {
	SkPaint *fill;
	SkPaint *stroke;

	SkMatrix *matrix;

	SkPath *path;
} showitem_t;

#define NEMOSHOW_ITEM_CC(base, name)				(((showitem_t *)((base)->cc))->name)

#endif
