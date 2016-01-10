#ifndef __NEMOSHELL_CURSOR_H__
#define __NEMOSHELL_CURSOR_H__

#include <nemoconfig.h>

#include <skiahelper.hpp>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

extern int nemocursor_make_circle(
		void *pixels, int width, int height,
		double x, double y, double r,
		SkPaint::Style style,
		SkColor color,
		double blur);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
