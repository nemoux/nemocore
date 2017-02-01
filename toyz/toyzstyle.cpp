#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <toyzstyle.hpp>
#include <nemomisc.h>

struct toyzstyle *nemotoyz_style_create(void)
{
	struct toyzstyle *style;

	style = new toyzstyle;
	style->paint = new SkPaint;

	style->paint->setStyle(SkPaint::kFill_Style);
	style->paint->setStrokeCap(SkPaint::kRound_Cap);
	style->paint->setStrokeJoin(SkPaint::kRound_Join);
	style->paint->setAntiAlias(true);

	return style;
}

void nemotoyz_style_destroy(struct toyzstyle *style)
{
	delete style->paint;
	delete style;
}

void nemotoyz_style_set_type(struct toyzstyle *style, int type)
{
	static const SkPaint::Style styles[] = {
		SkPaint::kFill_Style,
		SkPaint::kStroke_Style,
		SkPaint::kStrokeAndFill_Style
	};

	style->paint->setStyle(styles[type]);
}

void nemotoyz_style_set_color(struct toyzstyle *style, float r, float g, float b, float a)
{
	style->paint->setColor(SkColorSetARGB(a, r, g, b));
}

void nemotoyz_style_set_stroke_width(struct toyzstyle *style, float w)
{
	style->paint->setStrokeWidth(w);
}

void nemotoyz_style_set_stroke_cap(struct toyzstyle *style, int cap)
{
	static const SkPaint::Cap caps[] = {
		SkPaint::kButt_Cap,
		SkPaint::kRound_Cap,
		SkPaint::kSquare_Cap
	};

	style->paint->setStrokeCap(caps[cap]);
}

void nemotoyz_style_set_stroke_join(struct toyzstyle *style, int join)
{
	static const SkPaint::Join joins[] = {
		SkPaint::kMiter_Join,
		SkPaint::kRound_Join,
		SkPaint::kBevel_Join
	};

	style->paint->setStrokeJoin(joins[join]);
}

void nemotoyz_style_set_anti_alias(struct toyzstyle *style, int use_antialias)
{
	if (use_antialias != 0)
		style->paint->setAntiAlias(true);
	else
		style->paint->setAntiAlias(false);
}

void nemotoyz_style_set_blur_filter(struct toyzstyle *style, int type, int quality, float r)
{
	static const SkBlurStyle styles[] = {
		kNormal_SkBlurStyle,
		kInner_SkBlurStyle,
		kOuter_SkBlurStyle,
		kSolid_SkBlurStyle
	};
	static const SkBlurMaskFilter::BlurFlags flags[] = {
		SkBlurMaskFilter::kNone_BlurFlag,
		SkBlurMaskFilter::kIgnoreTransform_BlurFlag,
		SkBlurMaskFilter::kHighQuality_BlurFlag
	};

	style->paint->setMaskFilter(
			SkBlurMaskFilter::Make(
				styles[type],
				SkBlurMaskFilter::ConvertRadiusToSigma(r),
				flags[quality]));
}

void nemotoyz_style_set_emboss_filter(struct toyzstyle *style, float x, float y, float z, float r, float ambient, float specular)
{
	SkEmbossMaskFilter::Light light;

	light.fDirection[0] = x;
	light.fDirection[1] = y;
	light.fDirection[2] = z;
	light.fAmbient = ambient;
	light.fSpecular = specular;

	style->paint->setMaskFilter(
			SkEmbossMaskFilter::Make(
				SkBlurMaskFilter::ConvertRadiusToSigma(r),
				light));
}

void nemotoyz_style_set_shadow_filter(struct toyzstyle *style, int mode, float dx, float dy, float sx, float sy, float r, float g, float b, float a)
{
	static const SkDropShadowImageFilter::ShadowMode modes[] = {
		SkDropShadowImageFilter::kDrawShadowAndForeground_ShadowMode,
		SkDropShadowImageFilter::kDrawShadowOnly_ShadowMode
	};

	style->paint->setImageFilter(
			SkDropShadowImageFilter::Make(
				SkDoubleToScalar(dx),
				SkDoubleToScalar(dy),
				SkDoubleToScalar(sx),
				SkDoubleToScalar(sy),
				SkColorSetARGB(a, r, g, b),
				modes[mode],
				NULL,
				NULL));
}

void nemotoyz_style_set_path_effect(struct toyzstyle *style, float segment, float deviation, uint32_t seed)
{
	style->paint->setPathEffect(
			SkDiscretePathEffect::Make(segment, deviation, seed));
}

void nemotoyz_style_put_mask_filter(struct toyzstyle *style)
{
	style->paint->setMaskFilter(NULL);
}

void nemotoyz_style_put_image_filter(struct toyzstyle *style)
{
	style->paint->setImageFilter(NULL);
}

void nemotoyz_style_put_path_effect(struct toyzstyle *style)
{
	style->paint->setPathEffect(NULL);
}
