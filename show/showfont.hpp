#ifndef __NEMOSHOW_FONT_HPP__
#define __NEMOSHOW_FONT_HPP__

#include <skiaconfig.hpp>

typedef struct _showfont {
	sk_sp<SkTypeface> face;
} showfont_t;

#define NEMOSHOW_FONT_CC(base, name)				(((showfont_t *)((base)->cc))->name)
#define NEMOSHOW_FONT_ATCC(one, name)				(NEMOSHOW_FONT_CC(NEMOSHOW_FONT(one), name))

#endif
