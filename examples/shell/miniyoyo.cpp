#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <miniyoyo.hpp>

#include <showitem.h>
#include <showitem.hpp>
#include <showhelper.h>

struct miniyoyo *minishell_yoyo_create(int32_t width, int32_t height, struct showone *path)
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

void minishell_yoyo_dispatch_down_event(struct miniyoyo *yoyo, double x, double y)
{
	NEMOSHOW_ITEM_CC(NEMOSHOW_ITEM(yoyo->path), path)->moveTo(x, y);

	yoyo->x0 = yoyo->x1 = yoyo->x2 = x;
	yoyo->y0 = yoyo->y1 = yoyo->y2 = y;
}

void minishell_yoyo_dispatch_motion_event(struct miniyoyo *yoyo, double x, double y)
{
	double sx, sy, ex, ey;
	double x0, y0, x1, y1, x2, y2;
	double outer;

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

	NEMOSHOW_ITEM_CC(NEMOSHOW_ITEM(yoyo->path), path)->cubicTo(x0, y0, x1, y1, x2, y2);

	outer = nemoshow_item_get_outer(yoyo->path);

	yoyo->minx = MIN(MIN(sx, x0), MIN(x1, x2)) - outer;
	yoyo->miny = MIN(MIN(sy, y0), MIN(y1, y2)) - outer;
	yoyo->maxx = MAX(MAX(sx, x0), MAX(x1, x2)) + outer;
	yoyo->maxy = MAX(MAX(sy, y0), MAX(y1, y2)) + outer;
}

void minishell_yoyo_dispatch_up_event(struct miniyoyo *yoyo)
{
	SkBitmapDevice device(*yoyo->bitmap);
	SkCanvas canvas(&device);

	SkPaint paint;
	paint.setStyle(SkPaint::kFill_Style);
	paint.setColor(SK_ColorYELLOW);

	canvas.drawPath(*NEMOSHOW_ITEM_CC(NEMOSHOW_ITEM(yoyo->path), path), paint);
}

SkColor minishell_yoyo_get_pixel(struct miniyoyo *yoyo, int x, int y)
{
	return yoyo->bitmap->getColor(x, y);
}
