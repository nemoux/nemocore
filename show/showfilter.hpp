#ifndef __NEMOSHOW_FILTER_HPP__
#define __NEMOSHOW_FILTER_HPP__

typedef struct _showfilter {
	SkMaskFilter *filter;

	SkBlurMaskFilter::BlurFlags flags;
	SkBlurStyle style;
} showfilter_t;

#define NEMOSHOW_FILTER_CC(base, name)				(((showfilter_t *)((base)->cc))->name)
#define NEMOSHOW_FILTER_ATCC(one, name)				(NEMOSHOW_FILTER_CC(NEMOSHOW_FILTER(one), name))

#endif
