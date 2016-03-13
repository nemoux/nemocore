#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <skiaconfig.hpp>

#include <showpath.h>
#include <showpath.hpp>
#include <showitem.h>
#include <showitem.hpp>
#include <showfilter.h>
#include <showfilter.hpp>
#include <nemoshow.h>
#include <svghelper.h>
#include <fonthelper.h>
#include <nemomisc.h>

struct showone *nemoshow_path_create(void)
{
	struct showpath *path;
	struct showone *one;

	path = (struct showpath *)malloc(sizeof(struct showpath));
	if (path == NULL)
		return NULL;
	memset(path, 0, sizeof(struct showpath));

	path->cc = new showpath_t;
	NEMOSHOW_PATH_CC(path, paint) = new SkPaint;
	NEMOSHOW_PATH_CC(path, paint)->setStyle(SkPaint::kFill_Style);
	NEMOSHOW_PATH_CC(path, paint)->setStrokeCap(SkPaint::kRound_Cap);
	NEMOSHOW_PATH_CC(path, paint)->setStrokeJoin(SkPaint::kRound_Join);
	NEMOSHOW_PATH_CC(path, paint)->setAntiAlias(true);
	NEMOSHOW_PATH_CC(path, path) = new SkPath;

	path->alpha = 1.0f;

	path->fills[NEMOSHOW_PATH_ALPHA_COLOR] = 255.0f;
	path->fills[NEMOSHOW_PATH_RED_COLOR] = 255.0f;
	path->fills[NEMOSHOW_PATH_GREEN_COLOR] = 255.0f;
	path->fills[NEMOSHOW_PATH_BLUE_COLOR] = 255.0f;
	path->strokes[NEMOSHOW_PATH_ALPHA_COLOR] = 255.0f;
	path->strokes[NEMOSHOW_PATH_RED_COLOR] = 255.0f;
	path->strokes[NEMOSHOW_PATH_GREEN_COLOR] = 255.0f;
	path->strokes[NEMOSHOW_PATH_BLUE_COLOR] = 255.0f;

	path->_alpha = 1.0f;

	path->_fills[NEMOSHOW_PATH_ALPHA_COLOR] = 255.0f;
	path->_fills[NEMOSHOW_PATH_RED_COLOR] = 255.0f;
	path->_fills[NEMOSHOW_PATH_GREEN_COLOR] = 255.0f;
	path->_fills[NEMOSHOW_PATH_BLUE_COLOR] = 255.0f;
	path->_strokes[NEMOSHOW_PATH_ALPHA_COLOR] = 255.0f;
	path->_strokes[NEMOSHOW_PATH_RED_COLOR] = 255.0f;
	path->_strokes[NEMOSHOW_PATH_GREEN_COLOR] = 255.0f;
	path->_strokes[NEMOSHOW_PATH_BLUE_COLOR] = 255.0f;

	one = &path->base;
	one->type = NEMOSHOW_PATH_TYPE;
	one->update = nemoshow_path_update;
	one->destroy = nemoshow_path_destroy;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "stroke", path->_strokes, sizeof(double[4]));
	nemoobject_set_reserved(&one->object, "stroke-width", &path->stroke_width, sizeof(double));
	nemoobject_set_reserved(&one->object, "fill", path->_fills, sizeof(double[4]));

	nemoobject_set_reserved(&one->object, "pathsegment", &path->pathsegment, sizeof(double));
	nemoobject_set_reserved(&one->object, "pathdeviation", &path->pathdeviation, sizeof(double));
	nemoobject_set_reserved(&one->object, "pathseed", &path->pathseed, sizeof(uint32_t));

	nemoobject_set_reserved(&one->object, "alpha", &path->_alpha, sizeof(double));

	return one;
}

void nemoshow_path_destroy(struct showone *one)
{
	struct showpath *path = NEMOSHOW_PATH(one);

	nemoshow_one_finish(one);

	if (NEMOSHOW_PATH_CC(path, paint) != NULL)
		delete NEMOSHOW_PATH_CC(path, paint);
	if (NEMOSHOW_PATH_CC(path, path) != NULL)
		delete NEMOSHOW_PATH_CC(path, path);

	free(path);
}

int nemoshow_path_arrange(struct showone *one)
{
	return 0;
}

int nemoshow_path_update(struct showone *one)
{
	struct showpath *path = NEMOSHOW_PATH(one);

	if ((one->dirty & NEMOSHOW_STYLE_DIRTY) != 0) {
		struct showpath *group;

		if (nemoshow_one_has_state(one->parent, NEMOSHOW_INHERIT_STATE)) {
			group = NEMOSHOW_PATH(one->parent);

			path->alpha = path->_alpha * group->alpha;

			path->fills[NEMOSHOW_PATH_ALPHA_COLOR] = path->_fills[NEMOSHOW_PATH_ALPHA_COLOR] * (group->fills[NEMOSHOW_ITEM_ALPHA_COLOR] / 255.0f);
			path->fills[NEMOSHOW_PATH_RED_COLOR] = path->_fills[NEMOSHOW_PATH_RED_COLOR] * (group->fills[NEMOSHOW_ITEM_RED_COLOR] / 255.0f);
			path->fills[NEMOSHOW_PATH_GREEN_COLOR] = path->_fills[NEMOSHOW_PATH_GREEN_COLOR] * (group->fills[NEMOSHOW_ITEM_GREEN_COLOR] / 255.0f);
			path->fills[NEMOSHOW_PATH_BLUE_COLOR] = path->_fills[NEMOSHOW_PATH_BLUE_COLOR] * (group->fills[NEMOSHOW_ITEM_BLUE_COLOR] / 255.0f);

			path->strokes[NEMOSHOW_PATH_ALPHA_COLOR] = path->_strokes[NEMOSHOW_PATH_ALPHA_COLOR] * (group->strokes[NEMOSHOW_ITEM_ALPHA_COLOR] / 255.0f);
			path->strokes[NEMOSHOW_PATH_RED_COLOR] = path->_strokes[NEMOSHOW_PATH_RED_COLOR] * (group->strokes[NEMOSHOW_ITEM_RED_COLOR] / 255.0f);
			path->strokes[NEMOSHOW_PATH_GREEN_COLOR] = path->_strokes[NEMOSHOW_PATH_GREEN_COLOR] * (group->strokes[NEMOSHOW_ITEM_GREEN_COLOR] / 255.0f);
			path->strokes[NEMOSHOW_PATH_BLUE_COLOR] = path->_strokes[NEMOSHOW_PATH_BLUE_COLOR] * (group->strokes[NEMOSHOW_ITEM_BLUE_COLOR] / 255.0f);
		} else {
			path->alpha = path->_alpha;

			path->fills[NEMOSHOW_PATH_ALPHA_COLOR] = path->_fills[NEMOSHOW_ITEM_ALPHA_COLOR];
			path->fills[NEMOSHOW_PATH_RED_COLOR] = path->_fills[NEMOSHOW_ITEM_RED_COLOR];
			path->fills[NEMOSHOW_PATH_GREEN_COLOR] = path->_fills[NEMOSHOW_ITEM_GREEN_COLOR];
			path->fills[NEMOSHOW_PATH_BLUE_COLOR] = path->_fills[NEMOSHOW_ITEM_BLUE_COLOR];

			path->strokes[NEMOSHOW_PATH_ALPHA_COLOR] = path->_strokes[NEMOSHOW_ITEM_ALPHA_COLOR];
			path->strokes[NEMOSHOW_PATH_RED_COLOR] = path->_strokes[NEMOSHOW_ITEM_RED_COLOR];
			path->strokes[NEMOSHOW_PATH_GREEN_COLOR] = path->_strokes[NEMOSHOW_ITEM_GREEN_COLOR];
			path->strokes[NEMOSHOW_PATH_BLUE_COLOR] = path->_strokes[NEMOSHOW_ITEM_BLUE_COLOR];
		}

		NEMOSHOW_PATH_CC(path, paint)->setStrokeWidth(path->stroke_width);

		if (nemoshow_one_has_state(one, NEMOSHOW_FILL_STATE))
			NEMOSHOW_PATH_CC(path, paint)->setStyle(SkPaint::kFill_Style);
		else
			NEMOSHOW_PATH_CC(path, paint)->setStyle(SkPaint::kStroke_Style);

		NEMOSHOW_PATH_CC(path, paint)->setColor(
				SkColorSetARGB(
					path->fills[NEMOSHOW_PATH_ALPHA_COLOR] * path->alpha,
					path->fills[NEMOSHOW_PATH_RED_COLOR],
					path->fills[NEMOSHOW_PATH_GREEN_COLOR],
					path->fills[NEMOSHOW_PATH_BLUE_COLOR]));
	}

	if ((one->dirty & NEMOSHOW_PATH_DIRTY) != 0) {
		if (path->pathsegment >= 1.0f) {
			SkPathEffect *effect;

			effect = SkDiscretePathEffect::Create(path->pathsegment, path->pathdeviation, path->pathseed);
			if (effect != NULL) {
				NEMOSHOW_PATH_CC(path, paint)->setPathEffect(effect);
				effect->unref();
			}
		}

		if (path->pathdashcount > 0) {
			SkPathEffect *effect;
			SkScalar dashes[NEMOSHOW_PATH_DASH_MAX];
			int i;

			for (i = 0; i < path->pathdashcount; i++)
				dashes[i] = path->pathdashes[i];

			effect = SkDashPathEffect::Create(dashes, path->pathdashcount, 0);
			if (effect != NULL) {
				NEMOSHOW_PATH_CC(path, paint)->setPathEffect(effect);
				effect->unref();
			}
		}
	}

	if ((one->dirty & NEMOSHOW_FILTER_DIRTY) != 0) {
		if (NEMOSHOW_REF(one, NEMOSHOW_FILTER_REF) != NULL) {
			NEMOSHOW_PATH_CC(path, paint)->setMaskFilter(NEMOSHOW_FILTER_CC(NEMOSHOW_FILTER(NEMOSHOW_REF(one, NEMOSHOW_FILTER_REF)), filter));
		}
	}

	return 0;
}

void nemoshow_path_set_stroke_cap(struct showone *one, int cap)
{
	const SkPaint::Cap caps[] = {
		SkPaint::kButt_Cap,
		SkPaint::kRound_Cap,
		SkPaint::kSquare_Cap
	};
	struct showpath *path = NEMOSHOW_PATH(one);

	NEMOSHOW_PATH_CC(path, paint)->setStrokeCap(caps[cap]);
}

void nemoshow_path_set_stroke_join(struct showone *one, int join)
{
	const SkPaint::Join joins[] = {
		SkPaint::kMiter_Join,
		SkPaint::kRound_Join,
		SkPaint::kBevel_Join
	};
	struct showpath *path = NEMOSHOW_PATH(one);

	NEMOSHOW_PATH_CC(path, paint)->setStrokeJoin(joins[join]);
}

void nemoshow_path_clear(struct showone *one)
{
	struct showpath *path = NEMOSHOW_PATH(one);

	NEMOSHOW_PATH_CC(path, path)->reset();

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

void nemoshow_path_moveto(struct showone *one, double x, double y)
{
	struct showpath *path = NEMOSHOW_PATH(one);

	NEMOSHOW_PATH_CC(path, path)->moveTo(x, y);

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

void nemoshow_path_lineto(struct showone *one, double x, double y)
{
	struct showpath *path = NEMOSHOW_PATH(one);

	NEMOSHOW_PATH_CC(path, path)->lineTo(x, y);

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

void nemoshow_path_cubicto(struct showone *one, double x0, double y0, double x1, double y1, double x2, double y2)
{
	struct showpath *path = NEMOSHOW_PATH(one);

	NEMOSHOW_PATH_CC(path, path)->cubicTo(x0, y0, x1, y1, x2, y2);

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

void nemoshow_path_close(struct showone *one)
{
	struct showpath *path = NEMOSHOW_PATH(one);

	NEMOSHOW_PATH_CC(path, path)->close();

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

void nemoshow_path_cmd(struct showone *one, const char *cmd)
{
	struct showpath *path = NEMOSHOW_PATH(one);
	SkPath rpath;

	SkParsePath::FromSVGString(cmd, &rpath);

	NEMOSHOW_PATH_CC(path, path)->addPath(rpath);

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

void nemoshow_path_arc(struct showone *one, double x, double y, double width, double height, double from, double to)
{
	struct showpath *path = NEMOSHOW_PATH(one);
	SkRect rect = SkRect::MakeXYWH(x, y, width, height);

	NEMOSHOW_PATH_CC(path, path)->addArc(rect, from, to);

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

void nemoshow_path_text(struct showone *one, const char *font, int fontsize, const char *text, int textlength, double x, double y)
{
	struct showpath *path = NEMOSHOW_PATH(one);
	SkPaint paint;
	SkPath rpath;
	SkTypeface *face;

	SkSafeUnref(
			paint.setTypeface(
				SkTypeface::CreateFromFile(
					fontconfig_get_path(
						font,
						NULL,
						FC_SLANT_ROMAN,
						FC_WEIGHT_NORMAL,
						FC_WIDTH_NORMAL,
						FC_MONO), 0)));

	paint.setAntiAlias(true);
	paint.setTextSize(fontsize);
	paint.getTextPath(text, textlength, x, y, &rpath);

	NEMOSHOW_PATH_CC(path, path)->addPath(rpath);

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

void nemoshow_path_append(struct showone *one, struct showone *src)
{
	struct showpath *path = NEMOSHOW_PATH(one);
	struct showpath *other = NEMOSHOW_PATH(src);

	NEMOSHOW_PATH_CC(path, path)->addPath(*NEMOSHOW_PATH_CC(other, path));

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

void nemoshow_path_translate(struct showone *one, double x, double y)
{
	struct showpath *path = NEMOSHOW_PATH(one);
	SkMatrix matrix;

	matrix.setIdentity();
	matrix.postTranslate(x, y);

	NEMOSHOW_PATH_CC(path, path)->transform(matrix);

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

void nemoshow_path_scale(struct showone *one, double sx, double sy)
{
	struct showpath *path = NEMOSHOW_PATH(one);
	SkMatrix matrix;

	matrix.setIdentity();
	matrix.postScale(sx, sy);

	NEMOSHOW_PATH_CC(path, path)->transform(matrix);

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

void nemoshow_path_rotate(struct showone *one, double ro)
{
	struct showpath *path = NEMOSHOW_PATH(one);
	SkMatrix matrix;

	matrix.setIdentity();
	matrix.postRotate(ro);

	NEMOSHOW_PATH_CC(path, path)->transform(matrix);

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

void nemoshow_path_set_discrete_effect(struct showone *one, double segment, double deviation, uint32_t seed)
{
	struct showpath *path = NEMOSHOW_PATH(one);

	path->pathsegment = segment;
	path->pathdeviation = deviation;
	path->pathseed = seed;

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

void nemoshow_path_set_dash_effect(struct showone *one, double *dashes, int dashcount)
{
	struct showpath *path = NEMOSHOW_PATH(one);
	int i;

	if (path->pathdashes != NULL)
		free(path->pathdashes);

	path->pathdashes = (double *)malloc(sizeof(double) * dashcount);
	path->pathdashcount = dashcount;

	for (i = 0; i < dashcount; i++)
		path->pathdashes[i] = dashes[i];

	nemoobject_set_reserved(&one->object, "pathdash", path->pathdashes, sizeof(double) * dashcount);

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

void nemoshow_path_set_filter(struct showone *one, struct showone *filter)
{
	nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_FILTER_REF));
	nemoshow_one_reference_one(one, filter, NEMOSHOW_FILTER_DIRTY, NEMOSHOW_FILTER_REF);
}
