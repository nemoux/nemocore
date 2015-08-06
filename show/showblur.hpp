#ifndef __NEMOSHOW_BLUR_HPP__
#define __NEMOSHOW_BLUR_HPP__

#include <skiaconfig.hpp>

typedef struct _showblur {
	SkMaskFilter *filter;
} showblur_t;

#define NEMOSHOW_BLUR_CC(base, name)				(((showblur_t *)((base)->cc))->name)

#endif
