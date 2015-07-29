#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showshape.h>
#include <nemoxml.h>
#include <nemomisc.h>

struct showone *nemoshow_rect_create(void)
{
	struct showrect *rect;
	struct showone *one;

	rect = (struct showrect *)malloc(sizeof(struct showrect));
	if (rect == NULL)
		return NULL;
	memset(rect, 0, sizeof(struct showrect));

	one = &rect->base;
	one->type = NEMOSHOW_SHAPE_TYPE;
	one->sub = NEMOSHOW_SHAPE_RECT_TYPE;
	one->destroy = nemoshow_rect_destroy;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "x", &rect->x, sizeof(double));
	nemoobject_set_reserved(&one->object, "y", &rect->y, sizeof(double));
	nemoobject_set_reserved(&one->object, "width", &rect->width, sizeof(double));
	nemoobject_set_reserved(&one->object, "height", &rect->height, sizeof(double));
	nemoobject_set_reserved(&one->object, "alpha", &rect->alpha, sizeof(double));

	return one;
}

void nemoshow_rect_destroy(struct showone *one)
{
	struct showrect *rect = NEMOSHOW_RECT(one);

	nemoshow_one_finish(one);

	free(rect);
}

struct showone *nemoshow_text_create(void)
{
	struct showtext *text;
	struct showone *one;

	text = (struct showtext *)malloc(sizeof(struct showtext));
	if (text == NULL)
		return NULL;
	memset(text, 0, sizeof(struct showtext));

	one = &text->base;
	one->type = NEMOSHOW_SHAPE_TYPE;
	one->sub = NEMOSHOW_SHAPE_TEXT_TYPE;
	one->destroy = nemoshow_text_destroy;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "x", &text->x, sizeof(double));
	nemoobject_set_reserved(&one->object, "y", &text->y, sizeof(double));

	return one;
}

void nemoshow_text_destroy(struct showone *one)
{
	struct showtext *text = NEMOSHOW_TEXT(one);

	nemoshow_one_finish(one);

	free(text);
}
