#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <stdarg.h>

#include <nemotoyz.h>
#include <nemotoyz.hpp>
#include <toyzstyle.hpp>
#include <toyzpath.hpp>
#include <toyzmatrix.hpp>
#include <toyzregion.hpp>
#include <toyzpicture.hpp>
#include <nemomisc.h>

struct nemotoyz *nemotoyz_create(void)
{
	struct nemotoyz *toyz;

	toyz = new nemotoyz;
	toyz->bitmap = NULL;
	toyz->canvas = NULL;

	toyz->type = NEMOTOYZ_CANVAS_NONE_TYPE;

	return toyz;
}

void nemotoyz_destroy(struct nemotoyz *toyz)
{
	if (toyz->type == NEMOTOYZ_CANVAS_BUFFER_TYPE) {
		if (toyz->bitmap != NULL)
			delete toyz->bitmap;
		if (toyz->canvas != NULL)
			delete toyz->canvas;
	}

	delete toyz;
}

int nemotoyz_attach_buffer(struct nemotoyz *toyz, int colortype, int alphatype, void *buffer, int width, int height)
{
	static SkColorType colortypes[] = {
		kN32_SkColorType
	};
	static SkAlphaType alphatypes[] = {
		kPremul_SkAlphaType,
		kUnpremul_SkAlphaType,
		kOpaque_SkAlphaType
	};

	if (toyz->type == NEMOTOYZ_CANVAS_BUFFER_TYPE) {
		if (toyz->bitmap != NULL)
			delete toyz->bitmap;
		if (toyz->canvas != NULL)
			delete toyz->canvas;
	}

	toyz->bitmap = new SkBitmap;
	toyz->bitmap->setInfo(
			SkImageInfo::Make(
				width,
				height,
				colortypes[colortype],
				alphatypes[alphatype]));
	toyz->bitmap->setPixels(buffer);

	toyz->canvas = new SkCanvas(new SkBitmapDevice(*toyz->bitmap));

	toyz->type = NEMOTOYZ_CANVAS_BUFFER_TYPE;

	return 0;
}

void nemotoyz_detach_buffer(struct nemotoyz *toyz)
{
	if (toyz->bitmap != NULL)
		delete toyz->bitmap;
	if (toyz->canvas != NULL)
		delete toyz->canvas;

	toyz->type = NEMOTOYZ_CANVAS_NONE_TYPE;
}

int nemotoyz_attach_picture(struct nemotoyz *toyz, struct toyzpicture *picture, int width, int height)
{
	toyz->canvas = picture->recorder.beginRecording(width, height);

	toyz->type = NEMOTOYZ_CANVAS_PICTURE_TYPE;

	return 0;
}

void nemotoyz_detach_picture(struct nemotoyz *toyz, struct toyzpicture *picture)
{
	picture->picture = picture->recorder.finishRecordingAsPicture();

	toyz->type = NEMOTOYZ_CANVAS_NONE_TYPE;
}

int nemotoyz_load_image(struct nemotoyz *toyz, const char *url)
{
	sk_sp<SkData> data(SkData::MakeFromFileName(url));
	SkCodec *codec(SkCodec::NewFromData(data));
	SkBitmap *bitmap = toyz->bitmap;

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

void nemotoyz_save(struct nemotoyz *toyz)
{
	toyz->canvas->save();
}

void nemotoyz_save_with_alpha(struct nemotoyz *toyz, float a)
{
	toyz->canvas->saveLayerAlpha(NULL, 0xff * a);
}

void nemotoyz_restore(struct nemotoyz *toyz)
{
	toyz->canvas->restore();
}

void nemotoyz_clear(struct nemotoyz *toyz)
{
	toyz->canvas->clear(SK_ColorTRANSPARENT);
}

void nemotoyz_clear_color(struct nemotoyz *toyz, float r, float g, float b, float a)
{
	toyz->canvas->clear(SkColorSetARGB(a, r, g, b));
}

void nemotoyz_identity(struct nemotoyz *toyz)
{
	toyz->canvas->resetMatrix();
}

void nemotoyz_translate(struct nemotoyz *toyz, float tx, float ty)
{
	toyz->canvas->translate(tx, ty);
}

void nemotoyz_scale(struct nemotoyz *toyz, float sx, float sy)
{
	toyz->canvas->scale(sx, sy);
}

void nemotoyz_rotate(struct nemotoyz *toyz, float rz)
{
	toyz->canvas->rotate(rz);
}

void nemotoyz_concat(struct nemotoyz *toyz, struct toyzmatrix *matrix)
{
	toyz->canvas->concat(*matrix->matrix);
}

void nemotoyz_matrix(struct nemotoyz *toyz, struct toyzmatrix *matrix)
{
	toyz->canvas->setMatrix(*matrix->matrix);
}

void nemotoyz_clip_rectangle(struct nemotoyz *toyz, float x, float y, float w, float h)
{
	toyz->canvas->clipRect(SkRect::MakeXYWH(x, y, w, h));
}

void nemotoyz_clip_region(struct nemotoyz *toyz, struct toyzregion *region)
{
	toyz->canvas->clipRegion(*region->region);
}

void nemotoyz_clip_path(struct nemotoyz *toyz, struct toyzpath *path)
{
	toyz->canvas->clipPath(*path->path);
}

void nemotoyz_draw_line(struct nemotoyz *toyz, struct toyzstyle *style, float x, float y, float w, float h)
{
	toyz->canvas->drawLine(x, y, w, h, *style->paint);
}

void nemotoyz_draw_rect(struct nemotoyz *toyz, struct toyzstyle *style, float x, float y, float w, float h)
{
	SkRect rect = SkRect::MakeXYWH(x, y, w, h);

	toyz->canvas->drawRect(rect, *style->paint);
}

void nemotoyz_draw_round_rect(struct nemotoyz *toyz, struct toyzstyle *style, float x, float y, float w, float h, float ox, float oy)
{
	SkRect rect = SkRect::MakeXYWH(x, y, w, h);

	toyz->canvas->drawRoundRect(rect, ox, oy, *style->paint);
}

void nemotoyz_draw_circle(struct nemotoyz *toyz, struct toyzstyle *style, float x, float y, float r)
{
	toyz->canvas->drawCircle(x, y, r, *style->paint);
}

void nemotoyz_draw_arc(struct nemotoyz *toyz, struct toyzstyle *style, float x, float y, float w, float h, float from, float to)
{
	SkRect rect = SkRect::MakeXYWH(x, y, w, h);

	toyz->canvas->drawArc(rect, from, to - from, false, *style->paint);
}

void nemotoyz_draw_path(struct nemotoyz *toyz, struct toyzstyle *style, struct toyzpath *path)
{
	toyz->canvas->drawPath(*path->path, *style->paint);
}

void nemotoyz_draw_points(struct nemotoyz *toyz, struct toyzstyle *style, int npoints, ...)
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

	toyz->canvas->drawPoints(SkCanvas::kPoints_PointMode, npoints, points, *style->paint);
}

void nemotoyz_draw_polyline(struct nemotoyz *toyz, struct toyzstyle *style, int npoints, ...)
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

	toyz->canvas->drawPoints(SkCanvas::kLines_PointMode, npoints, points, *style->paint);
}

void nemotoyz_draw_polygon(struct nemotoyz *toyz, struct toyzstyle *style, int npoints, ...)
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

	toyz->canvas->drawPoints(SkCanvas::kPolygon_PointMode, npoints, points, *style->paint);
}

void nemotoyz_draw_bitmap(struct nemotoyz *toyz, struct toyzstyle *style, struct nemotoyz *bitmap, float x, float y, float w, float h)
{
	SkRect rect = SkRect::MakeXYWH(x, y, w, h);

	if (style != NULL)
		toyz->canvas->drawBitmapRect(*bitmap->bitmap, rect, style->paint);
	else
		toyz->canvas->drawBitmapRect(*bitmap->bitmap, rect, NULL);
}

void nemotoyz_draw_bitmap_with_alpha(struct nemotoyz *toyz, struct toyzstyle *style, struct nemotoyz *bitmap, float x, float y, float w, float h, float alpha)
{
	SkRect rect = SkRect::MakeXYWH(x, y, w, h);

	toyz->canvas->saveLayerAlpha(&rect, 0xff * alpha);

	if (style != NULL)
		toyz->canvas->drawBitmapRect(*bitmap->bitmap, rect, style->paint);
	else
		toyz->canvas->drawBitmapRect(*bitmap->bitmap, rect, NULL);

	toyz->canvas->restore();
}

void nemotoyz_draw_picture(struct nemotoyz *toyz, struct toyzpicture *picture)
{
	toyz->canvas->drawPicture(picture->picture);
}
