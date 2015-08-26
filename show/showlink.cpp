#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showlink.h>
#include <showlink.hpp>
#include <showfilter.h>
#include <showfilter.hpp>
#include <nemoshow.h>
#include <nemomisc.h>

#define	NEMOSHOW_ANTIALIAS_EPSILON			(3.0f)

struct showone *nemoshow_link_create(void)
{
	struct showlink *link;
	struct showone *one;

	link = (struct showlink *)malloc(sizeof(struct showlink));
	if (link == NULL)
		return NULL;
	memset(link, 0, sizeof(struct showlink));

	link->cc = new showlink_t;
	NEMOSHOW_LINK_CC(link, stroke) = new SkPaint;
	NEMOSHOW_LINK_CC(link, stroke)->setStyle(SkPaint::kStroke_Style);
	NEMOSHOW_LINK_CC(link, stroke)->setAntiAlias(true);

	link->alpha = 1.0f;

	one = &link->base;
	one->type = NEMOSHOW_LINK_TYPE;
	one->update = nemoshow_link_update;
	one->destroy = nemoshow_link_destroy;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "stroke", &link->stroke, sizeof(uint32_t));
	nemoobject_set_reserved(&one->object, "stroke:r", &link->strokes[2], sizeof(double));
	nemoobject_set_reserved(&one->object, "stroke:g", &link->strokes[1], sizeof(double));
	nemoobject_set_reserved(&one->object, "stroke:b", &link->strokes[0], sizeof(double));
	nemoobject_set_reserved(&one->object, "stroke:a", &link->strokes[3], sizeof(double));
	nemoobject_set_reserved(&one->object, "stroke-width", &link->stroke_width, sizeof(double));

	nemoobject_set_reserved(&one->object, "alpha", &link->alpha, sizeof(double));

	return one;
}

void nemoshow_link_destroy(struct showone *one)
{
	struct showlink *link = NEMOSHOW_LINK(one);

	nemoshow_one_finish(one);

	delete static_cast<showlink_t *>(link->cc);

	free(link);
}

int nemoshow_link_arrange(struct nemoshow *show, struct showone *one)
{
	struct showlink *link = NEMOSHOW_LINK(one);

	return 0;
}

int nemoshow_link_update(struct nemoshow *show, struct showone *one)
{
	struct showlink *link = NEMOSHOW_LINK(one);

	if (link->canvas == NULL)
		link->canvas = nemoshow_one_get_parent(one, NEMOSHOW_CANVAS_TYPE, 0);

	if ((one->dirty & NEMOSHOW_STYLE_DIRTY) != 0) {
		NEMOSHOW_LINK_CC(link, stroke)->setStrokeWidth(link->stroke_width);
		NEMOSHOW_LINK_CC(link, stroke)->setColor(
				SkColorSetARGB(255.0f * link->alpha, link->strokes[2], link->strokes[1], link->strokes[0]));
	}

	if ((one->dirty & NEMOSHOW_FILTER_DIRTY) != 0) {
		if (link->filter != NULL) {
			NEMOSHOW_LINK_CC(link, stroke)->setMaskFilter(NEMOSHOW_FILTER_CC(NEMOSHOW_FILTER(link->filter), filter));

			one->dirty |= NEMOSHOW_SHAPE_DIRTY;
		}
	}

	if ((one->dirty & NEMOSHOW_SHAPE_DIRTY) != 0 ||
			(one->dirty & NEMOSHOW_MATRIX_DIRTY) != 0) {
		double outer = 0.0f;

		outer += link->stroke_width;
		outer += NEMOSHOW_ANTIALIAS_EPSILON;
		if (link->filter != NULL)
			outer += NEMOSHOW_FILTER_AT(link->filter, r) * 2.0f;

		one->outer = outer;

		nemoshow_canvas_needs_redraw(link->canvas);
	}

	return 0;
}

void nemoshow_link_set_filter(struct showone *one, struct showone *filter)
{
	struct showlink *link = NEMOSHOW_LINK(one);

	link->filter = filter;

	nemoshow_one_reference_one(one, filter);
}
