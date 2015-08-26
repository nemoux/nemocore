#ifndef __NEMOSHOW_FILTER_HPP__
#define __NEMOSHOW_FILTER_HPP__

#include <skiaconfig.hpp>

typedef struct _showfilter {
	SkMaskFilter *filter;
} showfilter_t;

#define NEMOSHOW_FILTER_CC(base, name)				(((showfilter_t *)((base)->cc))->name)

#endif
