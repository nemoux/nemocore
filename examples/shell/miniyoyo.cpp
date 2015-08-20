#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <miniyoyo.hpp>
#include <showhelper.h>

struct miniyoyo *minishell_yoyo_create(int32_t width, int32_t height, SkPath *path)
{
	struct miniyoyo *yoyo;

	yoyo = (struct miniyoyo *)malloc(sizeof(struct miniyoyo));
	if (yoyo == NULL)
		return NULL;
	memset(yoyo, 0, sizeof(struct miniyoyo));

	yoyo->path = path;
	yoyo->width = width;
	yoyo->height = height;

	yoyo->bitmap = new SkBitmap;
	yoyo->bitmap->allocPixels(SkImageInfo::Make(
				width, height,
				kN32_SkColorType, kPremul_SkAlphaType));
	yoyo->bitmap->eraseColor(SK_ColorTRANSPARENT);

	return yoyo;
}

void minishell_yoyo_destroy(struct miniyoyo *yoyo)
{
	delete yoyo->bitmap;

	free(yoyo);
}

void minishell_yoyo_prepare(struct miniyoyo *yoyo, double x, double y)
{
	yoyo->path->moveTo(x, y);

	yoyo->x0 = yoyo->x1 = yoyo->x2 = x;
	yoyo->y0 = yoyo->y1 = yoyo->y2 = y;
}

void minishell_yoyo_update(struct miniyoyo *yoyo, double x, double y, int32_t *minx, int32_t *miny, int32_t *maxx, int32_t *maxy)
{
	double sx, sy, ex, ey;
	double x0, y0, x1, y1, x2, y2;

	yoyo->x2 = yoyo->x1;
	yoyo->y2 = yoyo->y1;
	yoyo->x1 = yoyo->x0;
	yoyo->y1 = yoyo->y0;
	yoyo->x0 = x;
	yoyo->y0 = y;

	sx = (yoyo->x2 + yoyo->x1) * 0.5f;
	sy = (yoyo->y2 + yoyo->y1) * 0.5f;
	ex = (yoyo->x1 + yoyo->x0) * 0.5f;
	ey = (yoyo->y1 + yoyo->y0) * 0.5f;

	x0 = (sx + 2.0f * yoyo->x1) / 3.0f;
	y0 = (sy + 2.0f * yoyo->y1) / 3.0f;
	x1 = (ex + 2.0f * yoyo->x1) / 3.0f;
	y1 = (ey + 2.0f * yoyo->y1) / 3.0f;
	x2 = ex;
	y2 = ey;

	yoyo->path->cubicTo(x0, y0, x1, y1, x2, y2);

	*minx = MIN(MIN(sx, x0), MIN(x1, x2));
	*miny = MIN(MIN(sy, y0), MIN(y1, y2));
	*maxx = MAX(MAX(sx, x0), MAX(x1, x2));
	*maxy = MAX(MAX(sy, y0), MAX(y1, y2));
}

void minishell_yoyo_finish(struct miniyoyo *yoyo)
{
	SkBitmapDevice device(*yoyo->bitmap);
	SkCanvas canvas(&device);

	SkPaint paint;
	paint.setStyle(SkPaint::kFill_Style);
	paint.setColor(SK_ColorYELLOW);

	canvas.drawPath(*yoyo->path, paint);
}

SkColor minishell_yoyo_get_pixel(struct miniyoyo *yoyo, int x, int y)
{
	return yoyo->bitmap->getColor(x, y);
}
