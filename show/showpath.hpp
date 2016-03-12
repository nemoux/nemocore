#ifndef __NEMOSHOW_PATH_HPP__
#define __NEMOSHOW_PATH_HPP__

#include <showpath.h>

typedef struct _showpath {
	SkPaint *paint;
	SkPath *path;
} showpath_t;

#define NEMOSHOW_PATH_CC(base, name)				(((showpath_t *)((base)->cc))->name)

#endif
