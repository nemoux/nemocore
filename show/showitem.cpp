#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showitem.h>
#include <showitem.hpp>
#include <showcolor.h>
#include <nemoshow.h>
#include <nemoxml.h>
#include <nemomisc.h>

struct showone *nemoshow_item_create(int type)
{
	struct showitem *item;
	struct showone *one;

	item = (struct showitem *)malloc(sizeof(struct showitem));
	if (item == NULL)
		return NULL;
	memset(item, 0, sizeof(struct showitem));

	item->cc = new showitem_t;

	one = &item->base;
	one->type = NEMOSHOW_ITEM_TYPE;
	one->sub = type;
	one->destroy = nemoshow_item_destroy;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "x", &item->x, sizeof(double));
	nemoobject_set_reserved(&one->object, "y", &item->y, sizeof(double));
	nemoobject_set_reserved(&one->object, "width", &item->width, sizeof(double));
	nemoobject_set_reserved(&one->object, "height", &item->height, sizeof(double));
	nemoobject_set_reserved(&one->object, "r", &item->r, sizeof(double));

	nemoobject_set_reserved(&one->object, "from", &item->from, sizeof(double));
	nemoobject_set_reserved(&one->object, "to", &item->to, sizeof(double));

	nemoobject_set_reserved(&one->object, "stroke", &item->stroke, sizeof(uint32_t));
	nemoobject_set_reserved(&one->object, "stroke:r", &item->strokes[2], sizeof(double));
	nemoobject_set_reserved(&one->object, "stroke:g", &item->strokes[1], sizeof(double));
	nemoobject_set_reserved(&one->object, "stroke:b", &item->strokes[0], sizeof(double));
	nemoobject_set_reserved(&one->object, "stroke:a", &item->strokes[3], sizeof(double));
	nemoobject_set_reserved(&one->object, "stroke-width", &item->stroke_width, sizeof(double));
	nemoobject_set_reserved(&one->object, "fill", &item->fill, sizeof(uint32_t));
	nemoobject_set_reserved(&one->object, "fill:r", &item->fills[2], sizeof(double));
	nemoobject_set_reserved(&one->object, "fill:g", &item->fills[1], sizeof(double));
	nemoobject_set_reserved(&one->object, "fill:b", &item->fills[0], sizeof(double));
	nemoobject_set_reserved(&one->object, "fill:a", &item->fills[3], sizeof(double));

	nemoobject_set_reserved(&one->object, "alpha", &item->alpha, sizeof(double));

	nemoobject_set_reserved(&one->object, "style", item->style, NEMOSHOW_ID_MAX);

	return one;
}

void nemoshow_item_destroy(struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	nemoshow_one_finish(one);

	delete static_cast<showitem_t *>(item->cc);

	free(item);
}

int nemoshow_item_arrange(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showone *stone;

	stone = nemoshow_search_one(show, item->style);
	if (stone != NULL) {
		item->stone = NEMOSHOW_ITEM(stone);
	} else {
		NEMOSHOW_ITEM_CC(item, fill) = new SkPaint;
		NEMOSHOW_ITEM_CC(item, stroke) = new SkPaint;

		item->stone = item;
	}

	return 0;
}

int nemoshow_item_update(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (item->stone == item) {
		if (item->fill != 0) {
			NEMOSHOW_ITEM_CC(item, fill)->setStyle(SkPaint::kFill_Style);
			NEMOSHOW_ITEM_CC(item, fill)->setColor(
					SkColorSetRGB(item->fills[2], item->fills[1], item->fills[0]));
		}
		if (item->stroke != 0) {
			NEMOSHOW_ITEM_CC(item, stroke)->setStyle(SkPaint::kStroke_Style);
			NEMOSHOW_ITEM_CC(item, stroke)->setStrokeWidth(item->stroke_width);
			NEMOSHOW_ITEM_CC(item, stroke)->setColor(
					SkColorSetRGB(item->strokes[2], item->strokes[1], item->strokes[0]));
		}
	}

	return 0;
}
