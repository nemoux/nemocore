#ifndef __NEMOSHOW_SVG_HPP__
#define __NEMOSHOW_SVG_HPP__

#include <skiaconfig.hpp>

typedef struct _showsvg {
	SkMatrix *matrix;
	SkMatrix *viewbox;
} showsvg_t;

#define NEMOSHOW_SVG_CC(base, name)				(((showsvg_t *)((base)->cc))->name)

#endif
