#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showblur.h>
#include <showblur.hpp>
#include <nemoshow.h>
#include <nemoxml.h>
#include <nemomisc.h>

struct showone *nemoshow_blur_create(void)
{
	struct showblur *blur;
	struct showone *one;

	blur = (struct showblur *)malloc(sizeof(struct showblur));
	if (blur == NULL)
		return NULL;
	memset(blur, 0, sizeof(struct showblur));

	blur->cc = new showblur_t;
	NEMOSHOW_BLUR_CC(blur, filter) = NULL;

	one = &blur->base;
	one->type = NEMOSHOW_BLUR_TYPE;
	one->update = nemoshow_blur_update;
	one->destroy = nemoshow_blur_destroy;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "r", &blur->r, sizeof(double));

	return one;
}

void nemoshow_blur_destroy(struct showone *one)
{
	struct showblur *blur = NEMOSHOW_BLUR(one);

	nemoshow_one_finish(one);

	delete static_cast<showblur_t *>(blur->cc);

	free(blur);
}

int nemoshow_blur_arrange(struct nemoshow *show, struct showone *one)
{
	struct showblur *blur = NEMOSHOW_BLUR(one);
	const char *flags = nemoobject_gets(&one->object, "flags");
	const char *style = nemoobject_gets(&one->object, "type");
	SkBlurMaskFilter::BlurFlags f;
	SkBlurStyle s;

	if (NEMOSHOW_BLUR_CC(blur, filter) != NULL)
		NEMOSHOW_BLUR_CC(blur, filter)->unref();

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

	NEMOSHOW_BLUR_CC(blur, filter) = SkBlurMaskFilter::Create(s, SkBlurMask::ConvertRadiusToSigma(blur->r), f);

	return 0;
}

int nemoshow_blur_update(struct nemoshow *show, struct showone *one)
{
	struct showblur *blur = NEMOSHOW_BLUR(one);

	return 0;
}

void nemoshow_blur_set_filter(struct showone *one, const char *flags, const char *style, double r)
{
	struct showblur *blur = NEMOSHOW_BLUR(one);
	SkBlurMaskFilter::BlurFlags f;
	SkBlurStyle s;

	blur->r = r;

	if (NEMOSHOW_BLUR_CC(blur, filter) != NULL)
		NEMOSHOW_BLUR_CC(blur, filter)->unref();

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

	NEMOSHOW_BLUR_CC(blur, filter) = SkBlurMaskFilter::Create(s, SkBlurMask::ConvertRadiusToSigma(blur->r), f);
}
