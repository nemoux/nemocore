#ifndef __NEMOSHOW_FILTER_HPP__
#define __NEMOSHOW_FILTER_HPP__

#include <skiaconfig.hpp>

typedef struct _showfilter {
	sk_sp<SkMaskFilter> maskfilter;
	sk_sp<SkImageFilter> imagefilter;
	sk_sp<SkColorFilter> colorfilter;

	SkBlurStyle style;
} showfilter_t;

#define NEMOSHOW_FILTER_CC(base, name)				(((showfilter_t *)((base)->cc))->name)
#define NEMOSHOW_FILTER_ATCC(one, name)				(NEMOSHOW_FILTER_CC(NEMOSHOW_FILTER(one), name))

#endif
