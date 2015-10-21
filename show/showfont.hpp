#ifndef __NEMOSHOW_FONT_HPP__
#define __NEMOSHOW_FONT_HPP__

typedef struct _showfont {
	SkTypeface *face;
} showfont_t;

#define NEMOSHOW_FONT_CC(base, name)				(((showfont_t *)((base)->cc))->name)

#endif
