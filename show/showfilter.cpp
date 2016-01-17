#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <skiaconfig.hpp>

#include <showfilter.h>
#include <showfilter.hpp>
#include <nemoshow.h>
#include <nemoxml.h>
#include <nemomisc.h>

struct showone *nemoshow_filter_create(int type)
{
	struct showfilter *filter;
	struct showone *one;

	filter = (struct showfilter *)malloc(sizeof(struct showfilter));
	if (filter == NULL)
		return NULL;
	memset(filter, 0, sizeof(struct showfilter));

	filter->cc = new showfilter_t;
	NEMOSHOW_FILTER_CC(filter, filter) = NULL;

	one = &filter->base;
	one->type = NEMOSHOW_FILTER_TYPE;
	one->sub = type;
	one->update = nemoshow_filter_update;
	one->destroy = nemoshow_filter_destroy;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "r", &filter->r, sizeof(double));

	return one;
}

void nemoshow_filter_destroy(struct showone *one)
{
	struct showfilter *filter = NEMOSHOW_FILTER(one);

	nemoshow_one_finish(one);

	if (NEMOSHOW_FILTER_CC(filter, filter) != NULL)
		NEMOSHOW_FILTER_CC(filter, filter)->unref();

	delete static_cast<showfilter_t *>(filter->cc);

	free(filter);
}

int nemoshow_filter_arrange(struct showone *one)
{
	struct showfilter *filter = NEMOSHOW_FILTER(one);
	const char *flags = nemoobject_gets(&one->object, "flags");
	const char *style = nemoobject_gets(&one->object, "type");

	if (one->sub == NEMOSHOW_BLUR_FILTER)
		nemoshow_filter_set_blur(one, flags, style, filter->r);

	return 0;
}

int nemoshow_filter_update(struct showone *one)
{
	struct showfilter *filter = NEMOSHOW_FILTER(one);

	if (NEMOSHOW_FILTER_CC(filter, filter) != NULL)
		NEMOSHOW_FILTER_CC(filter, filter)->unref();

	NEMOSHOW_FILTER_CC(filter, filter) = SkBlurMaskFilter::Create(
			NEMOSHOW_FILTER_CC(filter, style),
			SkBlurMask::ConvertRadiusToSigma(filter->r),
			NEMOSHOW_FILTER_CC(filter, flags));

	return 0;
}

void nemoshow_filter_set_blur(struct showone *one, const char *flags, const char *style, double r)
{
	struct showfilter *filter = NEMOSHOW_FILTER(one);

	filter->r = r;

	if (flags == NULL)
		NEMOSHOW_FILTER_CC(filter, flags) = SkBlurMaskFilter::kNone_BlurFlag;
	else if (strcmp(flags, "ignore") == 0)
		NEMOSHOW_FILTER_CC(filter, flags) = SkBlurMaskFilter::kIgnoreTransform_BlurFlag;
	else if (strcmp(flags, "high") == 0)
		NEMOSHOW_FILTER_CC(filter, flags) = SkBlurMaskFilter::kHighQuality_BlurFlag;

	if (style == NULL)
		NEMOSHOW_FILTER_CC(filter, style) = kNormal_SkBlurStyle;
	else if (strcmp(style, "inner") == 0)
		NEMOSHOW_FILTER_CC(filter, style) = kInner_SkBlurStyle;
	else if (strcmp(style, "outer") == 0)
		NEMOSHOW_FILTER_CC(filter, style) = kOuter_SkBlurStyle;
	else if (strcmp(style, "solid") == 0)
		NEMOSHOW_FILTER_CC(filter, style) = kSolid_SkBlurStyle;

	nemoshow_one_dirty(one, NEMOSHOW_FILTER_DIRTY | NEMOSHOW_SHAPE_DIRTY);
}

void nemoshow_filter_set_radius(struct showone *one, double r)
{
	struct showfilter *filter = NEMOSHOW_FILTER(one);

	filter->r = r;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}
