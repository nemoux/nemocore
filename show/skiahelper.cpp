#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <skiahelper.h>
#include <colorhelper.h>
#include <skiaconfig.hpp>

int skia_get_text_width(const char *font, double fontsize, const char *text)
{
	SkPaint paint;
	paint.setAntiAlias(true);
	paint.setStyle(SkPaint::kFill_Style);
	paint.setColor(SK_ColorWHITE);
	paint.setTypeface(SkTypeface::CreateFromFile(font, 0));
	paint.setTextSize(fontsize);

	SkScalar textwidths[strlen(text)];
	int textwidth = 0;
	int count;

	count = paint.getTextWidths(text, strlen(text), textwidths, NULL);

	for (int i = 0; i < count; i++) {
		textwidth += ceil(textwidths[i]);
	}

	return textwidth;
}

int skia_draw_text(void *pixels, int32_t width, int32_t height, const char *font, double fontsize, const char *text, double w, double b, int32_t c)
{
	SkBitmap bitmap;
	bitmap.setInfo(
			SkImageInfo::Make(width, height, kN32_SkColorType, kPremul_SkAlphaType));
	bitmap.setPixels(pixels);

	SkBitmapDevice device(bitmap);
	SkCanvas canvas(&device);

	SkPaint paint;
	paint.setAntiAlias(true);
	paint.setTypeface(SkTypeface::CreateFromFile(font, 0));
	paint.setTextSize(fontsize);

	if (w > 0.0f) {
		paint.setStyle(SkPaint::kStroke_Style);
		paint.setStrokeWidth(w);
	} else {
		paint.setStyle(SkPaint::kFill_Style);
	}

	paint.setColor(
			SkColorSetARGB(
				COLOR_UINT32_A(c),
				COLOR_UINT32_R(c),
				COLOR_UINT32_G(c),
				COLOR_UINT32_B(c)));

	if (b > 0.0f) {
		SkMaskFilter *filter = SkBlurMaskFilter::Create(
				kSolid_SkBlurStyle,
				SkBlurMask::ConvertRadiusToSigma(b),
				SkBlurMaskFilter::kHighQuality_BlurFlag);

		paint.setMaskFilter(filter);

		filter->unref();
	}

	SkPaint::FontMetrics metrics;
	paint.getFontMetrics(&metrics, 0);

	canvas.drawText(
			text, strlen(text),
			SkIntToScalar(0), SkIntToScalar(-metrics.fAscent),
			paint);

	SkScalar textwidths[strlen(text)];
	int textwidth = 0;
	int count;

	count = paint.getTextWidths(text, strlen(text), textwidths, NULL);

	for (int i = 0; i < count; i++) {
		textwidth += ceil(textwidths[i]);
	}

	return textwidth;
}

int skia_draw_circle(void *pixels, int32_t width, int32_t height, double x, double y, double r, double w, double b, int32_t c)
{
	SkBitmap bitmap;
	bitmap.setInfo(
			SkImageInfo::Make(width, height, kN32_SkColorType, kPremul_SkAlphaType));
	bitmap.setPixels(pixels);

	SkBitmapDevice device(bitmap);
	SkCanvas canvas(&device);

	SkPaint paint;
	paint.setAntiAlias(true);

	if (w > 0.0f) {
		paint.setStyle(SkPaint::kStroke_Style);
		paint.setStrokeWidth(w);
	} else {
		paint.setStyle(SkPaint::kFill_Style);
	}

	paint.setColor(
			SkColorSetARGB(
				COLOR_UINT32_A(c),
				COLOR_UINT32_R(c),
				COLOR_UINT32_G(c),
				COLOR_UINT32_B(c)));

	if (b > 0.0f) {
		SkMaskFilter *filter = SkBlurMaskFilter::Create(
				kSolid_SkBlurStyle,
				SkBlurMask::ConvertRadiusToSigma(b),
				SkBlurMaskFilter::kHighQuality_BlurFlag);

		paint.setMaskFilter(filter);

		filter->unref();
	}

	canvas.drawCircle(x, y, r, paint);

	return 0;
}

int skia_draw_image(void *pixels, int32_t width, int32_t height, const char *uri)
{
	SkBitmap back;
	SkImageDecoder::DecodeFile(uri, &back);

	SkBitmap bitmap;
	bitmap.setInfo(
			SkImageInfo::Make(width, height, kN32_SkColorType, kPremul_SkAlphaType));
	bitmap.setPixels(pixels);

	SkBitmapDevice device(bitmap);
	SkCanvas canvas(&device);

	canvas.drawBitmapRect(back,
			SkRect::MakeXYWH(0, 0, width, height), NULL);

	return 0;
}

int skia_clear_canvas(void *pixels, int32_t width, int32_t height)
{
	SkBitmap bitmap;
	bitmap.setInfo(
			SkImageInfo::Make(width, height, kN32_SkColorType, kPremul_SkAlphaType));
	bitmap.setPixels(pixels);

	SkBitmapDevice device(bitmap);
	SkCanvas canvas(&device);

	canvas.clear(SK_ColorTRANSPARENT);

	return 0;
}
