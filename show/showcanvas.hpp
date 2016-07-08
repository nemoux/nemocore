#ifndef __NEMOSHOW_CANVAS_HPP__
#define __NEMOSHOW_CANVAS_HPP__

#include <skiaconfig.hpp>

#include <showcanvas.h>

typedef struct _showcanvas {
	SkBitmapDevice *device;
	SkBitmap *bitmap;

	SkRegion *damage;
} showcanvas_t;

#define NEMOSHOW_CANVAS_CC(base, name)				(((showcanvas_t *)((base)->cc))->name)
#define NEMOSHOW_CANVAS_ATCC(one, name)				(NEMOSHOW_CANVAS_CC(NEMOSHOW_CANVAS(one), name))

#endif
