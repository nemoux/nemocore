#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <stdarg.h>

#include <nemotozz.h>
#include <tozzstyle.hpp>
#include <tozzmatrix.hpp>
#include <nemotozz.hpp>
#include <fonthelper.h>
#include <nemomisc.h>

struct tozzstyle *nemotozz_style_create(void)
{
	struct tozzstyle *style;

	style = new tozzstyle;
	style->paint = new SkPaint;

	style->paint->setStyle(SkPaint::kFill_Style);
	style->paint->setStrokeCap(SkPaint::kRound_Cap);
	style->paint->setStrokeJoin(SkPaint::kRound_Join);
	style->paint->setAntiAlias(true);

	return style;
}

void nemotozz_style_destroy(struct tozzstyle *style)
{
	delete style->paint;
	delete style;
}

void nemotozz_style_set_type(struct tozzstyle *style, int type)
{
	static const SkPaint::Style styles[] = {
		SkPaint::kFill_Style,
		SkPaint::kStroke_Style,
		SkPaint::kStrokeAndFill_Style
	};

	style->paint->setStyle(styles[type]);
}

void nemotozz_style_set_color(struct tozzstyle *style, float r, float g, float b, float a)
{
	style->paint->setColor(SkColorSetARGB(a, r, g, b));
}

void nemotozz_style_set_stroke_width(struct tozzstyle *style, float w)
{
	style->paint->setStrokeWidth(w);
}

void nemotozz_style_set_stroke_cap(struct tozzstyle *style, int cap)
{
	static const SkPaint::Cap caps[] = {
		SkPaint::kButt_Cap,
		SkPaint::kRound_Cap,
		SkPaint::kSquare_Cap
	};

	style->paint->setStrokeCap(caps[cap]);
}

void nemotozz_style_set_stroke_join(struct tozzstyle *style, int join)
{
	static const SkPaint::Join joins[] = {
		SkPaint::kMiter_Join,
		SkPaint::kRound_Join,
		SkPaint::kBevel_Join
	};

	style->paint->setStrokeJoin(joins[join]);
}

void nemotozz_style_set_anti_alias(struct tozzstyle *style, int use_antialias)
{
	if (use_antialias != 0)
		style->paint->setAntiAlias(true);
	else
		style->paint->setAntiAlias(false);
}

void nemotozz_style_set_blur_filter(struct tozzstyle *style, int type, int quality, float r)
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

void nemotozz_style_set_emboss_filter(struct tozzstyle *style, float x, float y, float z, float r, float ambient, float specular)
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

void nemotozz_style_set_shadow_filter(struct tozzstyle *style, int mode, float dx, float dy, float sx, float sy, float r, float g, float b, float a)
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

void nemotozz_style_set_path_effect(struct tozzstyle *style, float segment, float deviation, uint32_t seed)
{
	style->paint->setPathEffect(
			SkDiscretePathEffect::Make(segment, deviation, seed));
}

void nemotozz_style_set_dash_effect(struct tozzstyle *style, int *dashes, int count)
{
	SkScalar intervals[count];
	int i;

	for (i = 0; i < count; i++)
		intervals[i] = SkIntToScalar(dashes[i]);

	style->paint->setPathEffect(
			SkDashPathEffect::Make(intervals, count, 1));
}

void nemotozz_style_put_mask_filter(struct tozzstyle *style)
{
	style->paint->setMaskFilter(NULL);
}

void nemotozz_style_put_image_filter(struct tozzstyle *style)
{
	style->paint->setImageFilter(NULL);
}

void nemotozz_style_put_path_effect(struct tozzstyle *style)
{
	style->paint->setPathEffect(NULL);
}

void nemotozz_style_set_linear_gradient_shader(struct tozzstyle *style, float x0, float y0, float x1, float y1, int tilemode, int noffsets, ...)
{
	static const SkShader::TileMode tilemodes[] = {
		SkShader::kClamp_TileMode,
		SkShader::kRepeat_TileMode,
		SkShader::kMirror_TileMode
	};
	SkPoint points[] = {
		SkDoubleToScalar(x0),
		SkDoubleToScalar(y0),
		SkDoubleToScalar(x1),
		SkDoubleToScalar(y1)
	};
	SkColor colors[noffsets];
	SkScalar offsets[noffsets];
	va_list vargs;
	double r, g, b, a;
	double off;
	int i;

	va_start(vargs, noffsets);

	for (i = 0; i < noffsets; i++) {
		r = va_arg(vargs, double);
		g = va_arg(vargs, double);
		b = va_arg(vargs, double);
		a = va_arg(vargs, double);
		off = va_arg(vargs, double);

		colors[i] = SkColorSetARGB(a, r, g, b);
		offsets[i] = off;
	}

	va_end(vargs);

	style->paint->setShader(
			SkGradientShader::MakeLinear(
				points,
				colors,
				offsets,
				noffsets,
				tilemodes[tilemode]));
}

void nemotozz_style_set_radial_gradient_shader(struct tozzstyle *style, float x, float y, float radius, int tilemode, int noffsets, ...)
{
	static const SkShader::TileMode tilemodes[] = {
		SkShader::kClamp_TileMode,
		SkShader::kRepeat_TileMode,
		SkShader::kMirror_TileMode
	};
	SkColor colors[noffsets];
	SkScalar offsets[noffsets];
	va_list vargs;
	double r, g, b, a;
	double off;
	int i;

	va_start(vargs, noffsets);

	for (i = 0; i < noffsets; i++) {
		r = va_arg(vargs, double);
		g = va_arg(vargs, double);
		b = va_arg(vargs, double);
		a = va_arg(vargs, double);
		off = va_arg(vargs, double);

		colors[i] = SkColorSetARGB(a, r, g, b);
		offsets[i] = off;
	}

	va_end(vargs);

	style->paint->setShader(
			SkGradientShader::MakeRadial(
				SkPoint::Make(x, y),
				radius,
				colors,
				offsets,
				noffsets,
				tilemodes[tilemode]));
}

void nemotozz_style_set_bitmap_shader(struct tozzstyle *style, struct nemotozz *bitmap, int tilemodex, int tilemodey)
{
	static const SkShader::TileMode tilemodes[] = {
		SkShader::kClamp_TileMode,
		SkShader::kRepeat_TileMode,
		SkShader::kMirror_TileMode
	};

	style->paint->setShader(
			SkShader::MakeBitmapShader(
				*bitmap->bitmap,
				tilemodes[tilemodex],
				tilemodes[tilemodey]));
}

void nemotozz_style_put_shader(struct tozzstyle *style)
{
	style->paint->setShader(NULL);
}

void nemotozz_style_transform_shader(struct tozzstyle *style, struct tozzmatrix *matrix)
{
	SkShader *shader;

	shader = style->paint->getShader();
	if (shader != NULL)
		style->paint->setShader(
				shader->makeWithLocalMatrix(*matrix->matrix));
}

void nemotozz_style_load_font(struct tozzstyle *style, const char *path, int index)
{
	style->paint->setTypeface(
			SkTypeface::MakeFromFile(path, index));
}

void nemotozz_style_load_fontconfig(struct tozzstyle *style, const char *fontfamily, const char *fontstyle)
{
	style->paint->setTypeface(
			SkTypeface::MakeFromFile(
				fontconfig_get_path(
					fontfamily,
					fontstyle,
					FC_SLANT_ROMAN,
					FC_WEIGHT_NORMAL,
					FC_WIDTH_NORMAL,
					FC_MONO),
				0));
}

void nemotozz_style_set_font_size(struct tozzstyle *style, float fontsize)
{
	SkPaint::FontMetrics metrics;

	style->paint->setTextSize(fontsize);

	style->paint->getFontMetrics(&metrics, 0);
	style->fontascent = metrics.fAscent;
	style->fontdescent = metrics.fDescent;
}

float nemotozz_style_get_text_height(struct tozzstyle *style)
{
	return style->fontdescent - style->fontascent;
}

float nemotozz_style_get_text_width(struct tozzstyle *style, const char *text, int length)
{
	SkScalar widths[length];
	float width = 0.0f;
	int i, count;

	count = style->paint->getTextWidths(text, length, widths, NULL);

	for (i = 0; i < count; i++)
		width += ceil(widths[i]);

	return width;
}
