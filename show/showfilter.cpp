#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

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

	delete static_cast<showfilter_t *>(filter->cc);

	free(filter);
}

int nemoshow_filter_arrange(struct nemoshow *show, struct showone *one)
{
	struct showfilter *filter = NEMOSHOW_FILTER(one);
	const char *flags = nemoobject_gets(&one->object, "flags");
	const char *style = nemoobject_gets(&one->object, "type");

	if (one->sub == NEMOSHOW_BLUR_FILTER)
		nemoshow_filter_set_blur(one, flags, style, filter->r);

	return 0;
}

int nemoshow_filter_update(struct nemoshow *show, struct showone *one)
{
	struct showfilter *filter = NEMOSHOW_FILTER(one);

	return 0;
}

void nemoshow_filter_set_blur(struct showone *one, const char *flags, const char *style, double r)
{
	struct showfilter *filter = NEMOSHOW_FILTER(one);
	SkBlurMaskFilter::BlurFlags f;
	SkBlurStyle s;

	filter->r = r;

	if (NEMOSHOW_FILTER_CC(filter, filter) != NULL)
		NEMOSHOW_FILTER_CC(filter, filter)->unref();

	if (flags == NULL)
		f = SkBlurMaskFilter::kNone_BlurFlag;
	else if (strcmp(flags, "ignore") == 0)
		f = SkBlurMaskFilter::kIgnoreTransform_BlurFlag;
	else if (strcmp(flags, "high") == 0)
		f = SkBlurMaskFilter::kHighQuality_BlurFlag;

	if (style == NULL)
		s = kNormal_SkBlurStyle;
	else if (strcmp(style, "inner") == 0)
		s = kInner_SkBlurStyle;
	else if (strcmp(style, "outer") == 0)
		s = kOuter_SkBlurStyle;
	else if (strcmp(style, "solid") == 0)
		s = kSolid_SkBlurStyle;

	NEMOSHOW_FILTER_CC(filter, filter) = SkBlurMaskFilter::Create(s, SkBlurMask::ConvertRadiusToSigma(filter->r), f);
	
	nemoshow_one_dirty(one, NEMOSHOW_FILTER_DIRTY);
}
