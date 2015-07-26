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
