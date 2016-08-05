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
#include <nemomisc.h>

struct showone *nemoblurs[NEMOBLUR_LAST_TYPE];

void __attribute__((constructor(102))) nemoshow_filter_initialize(void)
{
	nemoblurs[NEMOBLUR_INNER_SMALL_TYPE] = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_filter_set_blur(nemoblurs[NEMOBLUR_INNER_SMALL_TYPE], "inner", 5.0f);
	nemoblurs[NEMOBLUR_INNER_MEDIUM_TYPE] = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_filter_set_blur(nemoblurs[NEMOBLUR_INNER_MEDIUM_TYPE], "inner", 10.0f);
	nemoblurs[NEMOBLUR_INNER_LARGE_TYPE] = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_filter_set_blur(nemoblurs[NEMOBLUR_INNER_LARGE_TYPE], "inner", 15.0f);
	nemoblurs[NEMOBLUR_OUTER_SMALL_TYPE] = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_filter_set_blur(nemoblurs[NEMOBLUR_OUTER_SMALL_TYPE], "outer", 5.0f);
	nemoblurs[NEMOBLUR_OUTER_MEDIUM_TYPE] = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_filter_set_blur(nemoblurs[NEMOBLUR_OUTER_MEDIUM_TYPE], "outer", 10.0f);
	nemoblurs[NEMOBLUR_OUTER_LARGE_TYPE] = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_filter_set_blur(nemoblurs[NEMOBLUR_OUTER_LARGE_TYPE], "outer", 15.0f);
	nemoblurs[NEMOBLUR_SOLID_SMALL_TYPE] = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_filter_set_blur(nemoblurs[NEMOBLUR_SOLID_SMALL_TYPE], "solid", 5.0f);
	nemoblurs[NEMOBLUR_SOLID_MEDIUM_TYPE] = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_filter_set_blur(nemoblurs[NEMOBLUR_SOLID_MEDIUM_TYPE], "solid", 10.0f);
	nemoblurs[NEMOBLUR_SOLID_LARGE_TYPE] = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_filter_set_blur(nemoblurs[NEMOBLUR_SOLID_LARGE_TYPE], "solid", 15.0f);
}

void __attribute__((destructor(102))) nemoshow_filter_finalize(void)
{
	int i;

	for (i = 0; i < NEMOBLUR_LAST_TYPE; i++) {
		nemoshow_filter_destroy(nemoblurs[i]);
	}
}

struct showone *nemoshow_filter_create(int type)
{
	struct showfilter *filter;
	struct showone *one;

	filter = (struct showfilter *)malloc(sizeof(struct showfilter));
	if (filter == NULL)
		return NULL;
	memset(filter, 0, sizeof(struct showfilter));

	filter->cc = new showfilter_t;
	NEMOSHOW_FILTER_CC(filter, maskfilter) = NULL;
	NEMOSHOW_FILTER_CC(filter, imagefilter) = NULL;
	NEMOSHOW_FILTER_CC(filter, colorfilter) = NULL;

	one = &filter->base;
	one->type = NEMOSHOW_FILTER_TYPE;
	one->sub = type;
	one->update = nemoshow_filter_update;
	one->destroy = nemoshow_filter_destroy;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "r", &filter->r, sizeof(double));

	nemoobject_set_reserved(&one->object, "dx", &filter->dx, sizeof(double));
	nemoobject_set_reserved(&one->object, "dy", &filter->dy, sizeof(double));
	nemoobject_set_reserved(&one->object, "dz", &filter->dz, sizeof(double));
	nemoobject_set_reserved(&one->object, "ambient", &filter->ambient, sizeof(double));
	nemoobject_set_reserved(&one->object, "specular", &filter->specular, sizeof(double));
	nemoobject_set_reserved(&one->object, "fill", &filter->fills, sizeof(double[4]));
	nemoobject_set_reserved(&one->object, "sigma-x", &filter->sx, sizeof(double));
	nemoobject_set_reserved(&one->object, "sigma-y", &filter->sy, sizeof(double));

	if (type == NEMOSHOW_BLUR_FILTER || type == NEMOSHOW_EMBOSS_FILTER) {
		filter->type = NEMOSHOW_FILTER_MASK_TYPE;
	} else if (type == NEMOSHOW_SHADOW_FILTER) {
		filter->type = NEMOSHOW_FILTER_IMAGE_TYPE;
	}

	return one;
}

void nemoshow_filter_destroy(struct showone *one)
{
	struct showfilter *filter = NEMOSHOW_FILTER(one);

	nemoshow_one_finish(one);

	if (NEMOSHOW_FILTER_CC(filter, maskfilter) != NULL)
		NEMOSHOW_FILTER_CC(filter, maskfilter)->unref();
	if (NEMOSHOW_FILTER_CC(filter, imagefilter) != NULL)
		NEMOSHOW_FILTER_CC(filter, imagefilter)->unref();
	if (NEMOSHOW_FILTER_CC(filter, colorfilter) != NULL)
		NEMOSHOW_FILTER_CC(filter, colorfilter)->unref();

	delete static_cast<showfilter_t *>(filter->cc);

	free(filter);
}

int nemoshow_filter_update(struct showone *one)
{
	static SkBlurMaskFilter::BlurFlags flags[] = {
		SkBlurMaskFilter::kNone_BlurFlag,
		SkBlurMaskFilter::kHighQuality_BlurFlag
	};
	struct nemoshow *show = one->show;
	struct showfilter *filter = NEMOSHOW_FILTER(one);

	if (filter->type == NEMOSHOW_FILTER_MASK_TYPE) {
		if (NEMOSHOW_FILTER_CC(filter, maskfilter) != NULL)
			NEMOSHOW_FILTER_CC(filter, maskfilter)->unref();

		if (one->sub == NEMOSHOW_BLUR_FILTER) {
			NEMOSHOW_FILTER_CC(filter, maskfilter) = SkBlurMaskFilter::Create(
					NEMOSHOW_FILTER_CC(filter, style),
					SkBlurMask::ConvertRadiusToSigma(filter->r),
					flags[show->quality]);
		} else if (one->sub == NEMOSHOW_EMBOSS_FILTER) {
			SkEmbossMaskFilter::Light light;

			light.fDirection[0] = filter->dx;
			light.fDirection[1] = filter->dy;
			light.fDirection[2] = filter->dz;
			light.fAmbient = filter->ambient;
			light.fSpecular = filter->specular;

			NEMOSHOW_FILTER_CC(filter, maskfilter) = SkEmbossMaskFilter::Create(
					SkBlurMask::ConvertRadiusToSigma(filter->r), light);
		}
	} else if (filter->type == NEMOSHOW_FILTER_IMAGE_TYPE) {
		if (NEMOSHOW_FILTER_CC(filter, imagefilter) != NULL)
			NEMOSHOW_FILTER_CC(filter, imagefilter)->unref();

		if (one->sub == NEMOSHOW_SHADOW_FILTER) {
			static const SkDropShadowImageFilter::ShadowMode shadowmodes[] = {
				SkDropShadowImageFilter::kDrawShadowAndForeground_ShadowMode,
				SkDropShadowImageFilter::kDrawShadowOnly_ShadowMode
			};

			NEMOSHOW_FILTER_CC(filter, imagefilter) = SkDropShadowImageFilter::Create(
					SkDoubleToScalar(filter->dx),
					SkDoubleToScalar(filter->dy),
					SkDoubleToScalar(filter->sx),
					SkDoubleToScalar(filter->sy),
					SkColorSetARGB(
						filter->fills[NEMOSHOW_ALPHA_COLOR],
						filter->fills[NEMOSHOW_RED_COLOR],
						filter->fills[NEMOSHOW_GREEN_COLOR],
						filter->fills[NEMOSHOW_BLUE_COLOR]),
					shadowmodes[filter->mode]);
		}
	} else if (filter->type == NEMOSHOW_FILTER_COLOR_TYPE) {
		if (NEMOSHOW_FILTER_CC(filter, colorfilter) != NULL)
			NEMOSHOW_FILTER_CC(filter, colorfilter)->unref();
	}

	return 0;
}

void nemoshow_filter_set_blur(struct showone *one, const char *style, double r)
{
	struct showfilter *filter = NEMOSHOW_FILTER(one);

	filter->r = r;

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

void nemoshow_filter_set_light(struct showone *one, double dx, double dy, double dz, double ambient, double specular)
{
	struct showfilter *filter = NEMOSHOW_FILTER(one);

	filter->dx = dx;
	filter->dy = dy;
	filter->dz = dz;

	filter->ambient = ambient;
	filter->specular = specular;

	nemoshow_one_dirty(one, NEMOSHOW_FILTER_DIRTY);
}

void nemoshow_filter_set_radius(struct showone *one, double r)
{
	struct showfilter *filter = NEMOSHOW_FILTER(one);

	filter->r = r;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

void nemoshow_filter_set_direction(struct showone *one, double dx, double dy, double dz)
{
	struct showfilter *filter = NEMOSHOW_FILTER(one);

	filter->dx = dx;
	filter->dy = dy;
	filter->dz = dz;

	nemoshow_one_dirty(one, NEMOSHOW_FILTER_DIRTY);
}

void nemoshow_filter_set_ambient(struct showone *one, double ambient)
{
	struct showfilter *filter = NEMOSHOW_FILTER(one);

	filter->ambient = ambient;

	nemoshow_one_dirty(one, NEMOSHOW_FILTER_DIRTY);
}

void nemoshow_filter_set_specular(struct showone *one, double specular)
{
	struct showfilter *filter = NEMOSHOW_FILTER(one);

	filter->specular = specular;

	nemoshow_one_dirty(one, NEMOSHOW_FILTER_DIRTY);
}
