#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <stdarg.h>

#include <nemotozz.h>
#include <nemotozz.hpp>
#include <tozzstyle.hpp>
#include <tozzpath.hpp>
#include <tozzmatrix.hpp>
#include <tozzregion.hpp>
#include <tozzpicture.hpp>
#include <nemomisc.h>

struct nemotozz *nemotozz_create(void)
{
	struct nemotozz *tozz;

	tozz = new nemotozz;
	tozz->bitmap = NULL;
	tozz->canvas = NULL;

	tozz->type = NEMOTOZZ_CANVAS_NONE_TYPE;

	return tozz;
}

void nemotozz_destroy(struct nemotozz *tozz)
{
	if (tozz->type == NEMOTOZZ_CANVAS_BUFFER_TYPE) {
		if (tozz->bitmap != NULL)
			delete tozz->bitmap;
		if (tozz->canvas != NULL)
			delete tozz->canvas;
	}

	delete tozz;
}

int nemotozz_attach_buffer(struct nemotozz *tozz, int colortype, int alphatype, void *buffer, int width, int height)
{
	static SkColorType colortypes[] = {
		kN32_SkColorType
	};
	static SkAlphaType alphatypes[] = {
		kPremul_SkAlphaType,
		kUnpremul_SkAlphaType,
		kOpaque_SkAlphaType
	};

	if (tozz->type == NEMOTOZZ_CANVAS_BUFFER_TYPE) {
		if (tozz->bitmap != NULL)
			delete tozz->bitmap;
		if (tozz->canvas != NULL)
			delete tozz->canvas;
	}

	tozz->bitmap = new SkBitmap;
	tozz->bitmap->setInfo(
			SkImageInfo::Make(
				width,
				height,
				colortypes[colortype],
				alphatypes[alphatype]));
	tozz->bitmap->setPixels(buffer);

	tozz->canvas = new SkCanvas(new SkBitmapDevice(*tozz->bitmap));

	tozz->type = NEMOTOZZ_CANVAS_BUFFER_TYPE;

	return 0;
}

void nemotozz_detach_buffer(struct nemotozz *tozz)
{
	if (tozz->bitmap != NULL)
		delete tozz->bitmap;
	if (tozz->canvas != NULL)
		delete tozz->canvas;

	tozz->type = NEMOTOZZ_CANVAS_NONE_TYPE;
}

int nemotozz_attach_picture(struct nemotozz *tozz, struct tozzpicture *picture, int width, int height)
{
	tozz->canvas = picture->recorder.beginRecording(width, height);

	tozz->type = NEMOTOZZ_CANVAS_PICTURE_TYPE;

	return 0;
}

void nemotozz_detach_picture(struct nemotozz *tozz, struct tozzpicture *picture)
{
	picture->picture = picture->recorder.finishRecordingAsPicture();

	tozz->type = NEMOTOZZ_CANVAS_NONE_TYPE;
}

int nemotozz_load_image(struct nemotozz *tozz, const char *url)
{
	sk_sp<SkData> data(SkData::MakeFromFileName(url));
	SkCodec *codec(SkCodec::NewFromData(data));
	SkBitmap *bitmap = tozz->bitmap;

	if (codec == NULL)
		return -1;

	SkImageInfo info = codec->getInfo().makeColorSpace(NULL);

	bitmap->setInfo(info);
	bitmap->allocPixels();

	SkCodec::Result r = codec->getPixels(info, bitmap->getPixels(), bitmap->rowBytes());

	if (r != SkCodec::kSuccess)
		return -1;

	return 0;
}

void nemotozz_save(struct nemotozz *tozz)
{
	tozz->canvas->save();
}

void nemotozz_save_with_alpha(struct nemotozz *tozz, float a)
{
	tozz->canvas->saveLayerAlpha(NULL, 0xff * a);
}

void nemotozz_restore(struct nemotozz *tozz)
{
	tozz->canvas->restore();
}

void nemotozz_clear(struct nemotozz *tozz)
{
	tozz->canvas->clear(SK_ColorTRANSPARENT);
}

void nemotozz_clear_color(struct nemotozz *tozz, float r, float g, float b, float a)
{
	tozz->canvas->clear(SkColorSetARGB(a, r, g, b));
}

void nemotozz_identity(struct nemotozz *tozz)
{
	tozz->canvas->resetMatrix();
}

void nemotozz_translate(struct nemotozz *tozz, float tx, float ty)
{
	tozz->canvas->translate(tx, ty);
}

void nemotozz_scale(struct nemotozz *tozz, float sx, float sy)
{
	tozz->canvas->scale(sx, sy);
}

void nemotozz_rotate(struct nemotozz *tozz, float rz)
{
	tozz->canvas->rotate(rz);
}

void nemotozz_concat(struct nemotozz *tozz, struct tozzmatrix *matrix)
{
	tozz->canvas->concat(*matrix->matrix);
}

void nemotozz_matrix(struct nemotozz *tozz, struct tozzmatrix *matrix)
{
	tozz->canvas->setMatrix(*matrix->matrix);
}

void nemotozz_clip_rectangle(struct nemotozz *tozz, float x, float y, float w, float h)
{
	tozz->canvas->clipRect(SkRect::MakeXYWH(x, y, w, h));
}

void nemotozz_clip_region(struct nemotozz *tozz, struct tozzregion *region)
{
	tozz->canvas->clipRegion(*region->region);
}

void nemotozz_clip_path(struct nemotozz *tozz, struct tozzpath *path)
{
	tozz->canvas->clipPath(*path->path);
}

void nemotozz_draw_line(struct nemotozz *tozz, struct tozzstyle *style, float x, float y, float w, float h)
{
	tozz->canvas->drawLine(x, y, w, h, *style->paint);
}

void nemotozz_draw_rect(struct nemotozz *tozz, struct tozzstyle *style, float x, float y, float w, float h)
{
	SkRect rect = SkRect::MakeXYWH(x, y, w, h);

	tozz->canvas->drawRect(rect, *style->paint);
}

void nemotozz_draw_round_rect(struct nemotozz *tozz, struct tozzstyle *style, float x, float y, float w, float h, float ox, float oy)
{
	SkRect rect = SkRect::MakeXYWH(x, y, w, h);

	tozz->canvas->drawRoundRect(rect, ox, oy, *style->paint);
}

void nemotozz_draw_circle(struct nemotozz *tozz, struct tozzstyle *style, float x, float y, float r)
{
	tozz->canvas->drawCircle(x, y, r, *style->paint);
}

void nemotozz_draw_arc(struct nemotozz *tozz, struct tozzstyle *style, float x, float y, float w, float h, float from, float to)
{
	SkRect rect = SkRect::MakeXYWH(x, y, w, h);

	tozz->canvas->drawArc(rect, from, to - from, false, *style->paint);
}

void nemotozz_draw_path(struct nemotozz *tozz, struct tozzstyle *style, struct tozzpath *path)
{
	tozz->canvas->drawPath(*path->path, *style->paint);
}

void nemotozz_draw_points(struct nemotozz *tozz, struct tozzstyle *style, int npoints, ...)
{
	SkPoint points[npoints];
	va_list vargs;
	int i;

	va_start(vargs, npoints);

	for (i = 0; i < npoints; i++) {
		points[i].set(
				va_arg(vargs, double),
				va_arg(vargs, double));
	}

	va_end(vargs);

	tozz->canvas->drawPoints(SkCanvas::kPoints_PointMode, npoints, points, *style->paint);
}

void nemotozz_draw_polyline(struct nemotozz *tozz, struct tozzstyle *style, int npoints, ...)
{
	SkPoint points[npoints];
	va_list vargs;
	int i;

	va_start(vargs, npoints);

	for (i = 0; i < npoints; i++) {
		points[i].set(
				va_arg(vargs, double),
				va_arg(vargs, double));
	}

	va_end(vargs);

	tozz->canvas->drawPoints(SkCanvas::kLines_PointMode, npoints, points, *style->paint);
}

void nemotozz_draw_polygon(struct nemotozz *tozz, struct tozzstyle *style, int npoints, ...)
{
	SkPoint points[npoints];
	va_list vargs;
	int i;

	va_start(vargs, npoints);

	for (i = 0; i < npoints; i++) {
		points[i].set(
				va_arg(vargs, double),
				va_arg(vargs, double));
	}

	va_end(vargs);

	tozz->canvas->drawPoints(SkCanvas::kPolygon_PointMode, npoints, points, *style->paint);
}

void nemotozz_draw_text(struct nemotozz *tozz, struct tozzstyle *style, float x, float y, const char *text)
{
	tozz->canvas->drawText(text, strlen(text), x, y - style->fontascent, *style->paint);
}

void nemotozz_draw_text_on_path(struct nemotozz *tozz, struct tozzstyle *style, struct tozzpath *path, const char *text)
{
	tozz->canvas->drawTextOnPath(text, strlen(text), *path->path, NULL, *style->paint);
}

void nemotozz_draw_bitmap(struct nemotozz *tozz, struct tozzstyle *style, struct nemotozz *bitmap, float x, float y, float w, float h)
{
	SkRect rect = SkRect::MakeXYWH(x, y, w, h);

	if (style != NULL)
		tozz->canvas->drawBitmapRect(*bitmap->bitmap, rect, style->paint);
	else
		tozz->canvas->drawBitmapRect(*bitmap->bitmap, rect, NULL);
}

void nemotozz_draw_bitmap_with_alpha(struct nemotozz *tozz, struct tozzstyle *style, struct nemotozz *bitmap, float x, float y, float w, float h, float alpha)
{
	SkRect rect = SkRect::MakeXYWH(x, y, w, h);

	tozz->canvas->saveLayerAlpha(&rect, 0xff * alpha);

	if (style != NULL)
		tozz->canvas->drawBitmapRect(*bitmap->bitmap, rect, style->paint);
	else
		tozz->canvas->drawBitmapRect(*bitmap->bitmap, rect, NULL);

	tozz->canvas->restore();
}

void nemotozz_draw_picture(struct nemotozz *tozz, struct tozzpicture *picture)
{
	tozz->canvas->drawPicture(picture->picture);
}
