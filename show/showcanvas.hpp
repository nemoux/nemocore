#ifndef __NEMOSHOW_CANVAS_HPP__
#define __NEMOSHOW_CANVAS_HPP__

#include <skiaconfig.hpp>

typedef struct _showcanvas {
	SkBitmapDevice *device;
	SkBitmap *bitmap;
	SkCanvas *canvas;

	SkRegion *damage;
} showcanvas_t;

#define NEMOSHOW_CANVAS_CC(base, name)				(((showcanvas_t *)((base)->cc))->name)

#endif
