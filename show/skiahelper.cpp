#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <skiahelper.h>
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

int skia_draw_text(void *pixels, int32_t width, int32_t height, const char *font, double fontsize, const char *text)
{
	SkBitmap bitmap;
	bitmap.setInfo(
			SkImageInfo::Make(width, height, kN32_SkColorType, kPremul_SkAlphaType));
	bitmap.setPixels(pixels);

	SkBitmapDevice device(bitmap);
	SkCanvas canvas(&device);

	canvas.clear(SK_ColorTRANSPARENT);

	SkPaint paint;
	paint.setAntiAlias(true);
	paint.setStyle(SkPaint::kFill_Style);
	paint.setColor(SK_ColorWHITE);
	paint.setTypeface(SkTypeface::CreateFromFile(font, 0));
	paint.setTextSize(fontsize);

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
