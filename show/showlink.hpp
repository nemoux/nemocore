#ifndef	__NEMOSHOW_LINK_HPP__
#define	__NEMOSHOW_LINK_HPP__

#include <skiaconfig.hpp>

typedef struct _showlink {
	SkPaint *stroke;
} showlink_t;

#define	NEMOSHOW_LINK_CC(base, name)				(((showlink_t *)((base)->cc))->name)

#endif
