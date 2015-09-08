#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <skiahelper.hpp>

int skia_draw_circle(
		void *pixels, int width, int height,
		double x, double y, double r,
		SkPaint::Style style,
		SkColor color,
		double blur)
{
	SkBitmap bitmap;
	bitmap.setInfo(
			SkImageInfo::Make(width, height, kN32_SkColorType, kPremul_SkAlphaType));
	bitmap.setPixels(pixels);

	SkBitmapDevice device(bitmap);
	SkCanvas canvas(&device);

	SkMaskFilter *filter = SkBlurMaskFilter::Create(
			kSolid_SkBlurStyle,
			SkBlurMask::ConvertRadiusToSigma(5.0f),
			SkBlurMaskFilter::kHighQuality_BlurFlag);

	SkPaint paint;
	paint.setStyle(style);
	paint.setColor(color);
	paint.setAntiAlias(true);
	paint.setMaskFilter(filter);

	canvas.clear(SK_ColorTRANSPARENT);
	canvas.drawCircle(x, y, r, paint);

	filter->unref();

	return 0;
}
