#ifndef __NEMOSHOW_PATH_HPP__
#define __NEMOSHOW_PATH_HPP__

#include <skiaconfig.hpp>

#include <showpath.h>

typedef struct _showpath {
	SkPaint *fill;
	SkPaint *stroke;

	SkPath *path;
} showpath_t;

#define NEMOSHOW_PATH_CC(base, name)				(((showpath_t *)((base)->cc))->name)
#define NEMOSHOW_PATH_ATCC(one, name)				(NEMOSHOW_PATH_CC(NEMOSHOW_PATH(one), name))

#endif
