#ifndef	__NEMOSHOW_HELPER_HPP__
#define	__NEMOSHOW_HELPER_HPP__

#include <skiaconfig.hpp>

extern double nemoshow_helper_get_path_length(SkPath *path);
extern void nemoshow_helper_draw_path(SkPath &dst, SkPath *src, SkPaint *paint, double length, double from, double to);

#endif
