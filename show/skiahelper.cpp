#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <skiahelper.h>
#include <colorhelper.h>
#include <skiaconfig.hpp>

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
		sk_sp<SkMaskFilter> filter = SkBlurMaskFilter::Make(
				kSolid_SkBlurStyle,
				SkBlurMaskFilter::ConvertRadiusToSigma(b),
				SkBlurMaskFilter::kHighQuality_BlurFlag);

		paint.setMaskFilter(filter);
	}

	canvas.drawCircle(x, y, r, paint);

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

int skia_read_image(SkBitmap *bitmap, const char *imagepath)
{
	class NemoShowPngChunkReader : public SkPngChunkReader {
		public:
			NemoShowPngChunkReader()
			{
			}

			bool readChunk(const char tags[], const void *data, size_t length) override
			{
				return true;
			}

			bool allHaveBeenSeen()
			{
				return true;
			}
	};

	NemoShowPngChunkReader chunkReader;

	SkAutoTUnref<SkData> data(SkData::NewFromFileName(imagepath));
	SkAutoTDelete<SkCodec> codec(SkCodec::NewFromData(data, &chunkReader));

	if (codec == NULL)
		return -1;

	bitmap->setInfo(codec->getInfo());
	bitmap->allocPixels();

	SkCodec::Result r = codec->getPixels(codec->getInfo(), bitmap->getPixels(), bitmap->rowBytes());

	if (r != SkCodec::kSuccess)
		return -1;

	return 0;
}
