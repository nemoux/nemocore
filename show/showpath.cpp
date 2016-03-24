#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <skiaconfig.hpp>

#include <showpath.h>
#include <showpath.hpp>
#include <showpathcmd.h>
#include <showitem.h>
#include <showitem.hpp>
#include <showfilter.h>
#include <showfilter.hpp>
#include <showshader.h>
#include <showshader.hpp>
#include <nemoshow.h>
#include <svghelper.h>
#include <fonthelper.h>
#include <nemomisc.h>

struct showone *nemoshow_path_create(int type)
{
	struct showpath *path;
	struct showone *one;

	path = (struct showpath *)malloc(sizeof(struct showpath));
	if (path == NULL)
		return NULL;
	memset(path, 0, sizeof(struct showpath));

	path->cc = new showpath_t;
	NEMOSHOW_PATH_CC(path, fill) = new SkPaint;
	NEMOSHOW_PATH_CC(path, fill)->setStyle(SkPaint::kFill_Style);
	NEMOSHOW_PATH_CC(path, fill)->setStrokeCap(SkPaint::kRound_Cap);
	NEMOSHOW_PATH_CC(path, fill)->setStrokeJoin(SkPaint::kRound_Join);
	NEMOSHOW_PATH_CC(path, fill)->setAntiAlias(true);
	NEMOSHOW_PATH_CC(path, stroke) = new SkPaint;
	NEMOSHOW_PATH_CC(path, stroke)->setStyle(SkPaint::kStroke_Style);
	NEMOSHOW_PATH_CC(path, stroke)->setStrokeCap(SkPaint::kRound_Cap);
	NEMOSHOW_PATH_CC(path, stroke)->setStrokeJoin(SkPaint::kRound_Join);
	NEMOSHOW_PATH_CC(path, stroke)->setAntiAlias(true);
	NEMOSHOW_PATH_CC(path, path) = new SkPath;

	path->alpha = 1.0f;

	path->fills[NEMOSHOW_ALPHA_COLOR] = 255.0f;
	path->fills[NEMOSHOW_RED_COLOR] = 255.0f;
	path->fills[NEMOSHOW_GREEN_COLOR] = 255.0f;
	path->fills[NEMOSHOW_BLUE_COLOR] = 255.0f;
	path->strokes[NEMOSHOW_ALPHA_COLOR] = 255.0f;
	path->strokes[NEMOSHOW_RED_COLOR] = 255.0f;
	path->strokes[NEMOSHOW_GREEN_COLOR] = 255.0f;
	path->strokes[NEMOSHOW_BLUE_COLOR] = 255.0f;

	path->_alpha = 1.0f;

	path->_fills[NEMOSHOW_ALPHA_COLOR] = 255.0f;
	path->_fills[NEMOSHOW_RED_COLOR] = 255.0f;
	path->_fills[NEMOSHOW_GREEN_COLOR] = 255.0f;
	path->_fills[NEMOSHOW_BLUE_COLOR] = 255.0f;
	path->_strokes[NEMOSHOW_ALPHA_COLOR] = 255.0f;
	path->_strokes[NEMOSHOW_RED_COLOR] = 255.0f;
	path->_strokes[NEMOSHOW_GREEN_COLOR] = 255.0f;
	path->_strokes[NEMOSHOW_BLUE_COLOR] = 255.0f;

	one = &path->base;
	one->type = NEMOSHOW_PATH_TYPE;
	one->sub = type;
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

	if (one->sub == NEMOSHOW_ARRAY_PATH) {
		path->cmds = (uint32_t *)malloc(sizeof(uint32_t) * 8);
		path->ncmds = 0;
		path->scmds = 8;

		path->points = (double *)malloc(sizeof(double) * 8);
		path->npoints = 0;
		path->spoints = 8;

		nemoobject_set_reserved(&one->object, "points", path->points, sizeof(double) * path->spoints);
	}

	return one;
}

void nemoshow_path_destroy(struct showone *one)
{
	struct showpath *path = NEMOSHOW_PATH(one);

	nemoshow_one_finish(one);

	if (NEMOSHOW_PATH_CC(path, fill) != NULL)
		delete NEMOSHOW_PATH_CC(path, fill);
	if (NEMOSHOW_PATH_CC(path, stroke) != NULL)
		delete NEMOSHOW_PATH_CC(path, stroke);
	if (NEMOSHOW_PATH_CC(path, path) != NULL)
		delete NEMOSHOW_PATH_CC(path, path);

	delete static_cast<showpath_t *>(path->cc);

	if (path->cmds != NULL)
		free(path->cmds);
	if (path->points != NULL)
		free(path->points);

	free(path);
}

static inline void nemoshow_path_update_style(struct nemoshow *show, struct showone *one)
{
	struct showpath *path = NEMOSHOW_PATH(one);
	struct showitem *group;

	if (nemoshow_one_has_state(one, NEMOSHOW_FILL_STATE)) {
		if (nemoshow_one_has_state(one->parent, NEMOSHOW_INHERIT_STATE)) {
			group = NEMOSHOW_ITEM(one->parent);

			path->alpha = path->_alpha * group->alpha;

			path->fills[NEMOSHOW_ALPHA_COLOR] = path->_fills[NEMOSHOW_ALPHA_COLOR] * (group->fills[NEMOSHOW_ALPHA_COLOR] / 255.0f);
			path->fills[NEMOSHOW_RED_COLOR] = path->_fills[NEMOSHOW_RED_COLOR] * (group->fills[NEMOSHOW_RED_COLOR] / 255.0f);
			path->fills[NEMOSHOW_GREEN_COLOR] = path->_fills[NEMOSHOW_GREEN_COLOR] * (group->fills[NEMOSHOW_GREEN_COLOR] / 255.0f);
			path->fills[NEMOSHOW_BLUE_COLOR] = path->_fills[NEMOSHOW_BLUE_COLOR] * (group->fills[NEMOSHOW_BLUE_COLOR] / 255.0f);
		} else {
			path->alpha = path->_alpha;

			path->fills[NEMOSHOW_ALPHA_COLOR] = path->_fills[NEMOSHOW_ALPHA_COLOR];
			path->fills[NEMOSHOW_RED_COLOR] = path->_fills[NEMOSHOW_RED_COLOR];
			path->fills[NEMOSHOW_GREEN_COLOR] = path->_fills[NEMOSHOW_GREEN_COLOR];
			path->fills[NEMOSHOW_BLUE_COLOR] = path->_fills[NEMOSHOW_BLUE_COLOR];
		}

		NEMOSHOW_PATH_CC(path, fill)->setColor(
				SkColorSetARGB(
					path->fills[NEMOSHOW_ALPHA_COLOR] * path->alpha,
					path->fills[NEMOSHOW_RED_COLOR],
					path->fills[NEMOSHOW_GREEN_COLOR],
					path->fills[NEMOSHOW_BLUE_COLOR]));
	}
	if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE)) {
		if (nemoshow_one_has_state(one->parent, NEMOSHOW_INHERIT_STATE)) {
			group = NEMOSHOW_ITEM(one->parent);

			path->alpha = path->_alpha * group->alpha;

			path->strokes[NEMOSHOW_ALPHA_COLOR] = path->_strokes[NEMOSHOW_ALPHA_COLOR] * (group->strokes[NEMOSHOW_ALPHA_COLOR] / 255.0f);
			path->strokes[NEMOSHOW_RED_COLOR] = path->_strokes[NEMOSHOW_RED_COLOR] * (group->strokes[NEMOSHOW_RED_COLOR] / 255.0f);
			path->strokes[NEMOSHOW_GREEN_COLOR] = path->_strokes[NEMOSHOW_GREEN_COLOR] * (group->strokes[NEMOSHOW_GREEN_COLOR] / 255.0f);
			path->strokes[NEMOSHOW_BLUE_COLOR] = path->_strokes[NEMOSHOW_BLUE_COLOR] * (group->strokes[NEMOSHOW_BLUE_COLOR] / 255.0f);
		} else {
			path->alpha = path->_alpha;

			path->strokes[NEMOSHOW_ALPHA_COLOR] = path->_strokes[NEMOSHOW_ALPHA_COLOR];
			path->strokes[NEMOSHOW_RED_COLOR] = path->_strokes[NEMOSHOW_RED_COLOR];
			path->strokes[NEMOSHOW_GREEN_COLOR] = path->_strokes[NEMOSHOW_GREEN_COLOR];
			path->strokes[NEMOSHOW_BLUE_COLOR] = path->_strokes[NEMOSHOW_BLUE_COLOR];
		}

		NEMOSHOW_PATH_CC(path, stroke)->setStrokeWidth(path->stroke_width);
		NEMOSHOW_PATH_CC(path, stroke)->setColor(
				SkColorSetARGB(
					path->strokes[NEMOSHOW_ALPHA_COLOR] * path->alpha,
					path->strokes[NEMOSHOW_RED_COLOR],
					path->strokes[NEMOSHOW_GREEN_COLOR],
					path->strokes[NEMOSHOW_BLUE_COLOR]));
	}
}

static inline void nemoshow_path_update_filter(struct nemoshow *show, struct showone *one)
{
	struct showpath *path = NEMOSHOW_PATH(one);

	if (NEMOSHOW_REF(one, NEMOSHOW_FILTER_REF) != NULL) {
		struct showone *ref = NEMOSHOW_REF(one, NEMOSHOW_FILTER_REF);
		struct showfilter *filter = NEMOSHOW_FILTER(ref);

		if (filter->type == NEMOSHOW_FILTER_MASK_TYPE) {
			NEMOSHOW_PATH_CC(path, fill)->setMaskFilter(NEMOSHOW_FILTER_CC(filter, maskfilter));
			NEMOSHOW_PATH_CC(path, stroke)->setMaskFilter(NEMOSHOW_FILTER_CC(filter, maskfilter));
		} else if (filter->type == NEMOSHOW_FILTER_IMAGE_TYPE) {
			NEMOSHOW_PATH_CC(path, fill)->setImageFilter(NEMOSHOW_FILTER_CC(filter, imagefilter));
			NEMOSHOW_PATH_CC(path, stroke)->setImageFilter(NEMOSHOW_FILTER_CC(filter, imagefilter));
		} else if (filter->type == NEMOSHOW_FILTER_COLOR_TYPE) {
			NEMOSHOW_PATH_CC(path, fill)->setColorFilter(NEMOSHOW_FILTER_CC(filter, colorfilter));
			NEMOSHOW_PATH_CC(path, stroke)->setColorFilter(NEMOSHOW_FILTER_CC(filter, colorfilter));
		}
	} else {
		NEMOSHOW_PATH_CC(path, fill)->setMaskFilter(NULL);
		NEMOSHOW_PATH_CC(path, stroke)->setMaskFilter(NULL);
		NEMOSHOW_PATH_CC(path, fill)->setImageFilter(NULL);
		NEMOSHOW_PATH_CC(path, stroke)->setImageFilter(NULL);
		NEMOSHOW_PATH_CC(path, fill)->setColorFilter(NULL);
		NEMOSHOW_PATH_CC(path, stroke)->setColorFilter(NULL);
	}
}

static inline void nemoshow_path_update_shader(struct nemoshow *show, struct showone *one)
{
	struct showpath *path = NEMOSHOW_PATH(one);

	if (NEMOSHOW_REF(one, NEMOSHOW_SHADER_REF) != NULL) {
		NEMOSHOW_PATH_CC(path, fill)->setShader(NEMOSHOW_SHADER_ATCC(NEMOSHOW_REF(one, NEMOSHOW_SHADER_REF), shader));
		NEMOSHOW_PATH_CC(path, stroke)->setShader(NEMOSHOW_SHADER_ATCC(NEMOSHOW_REF(one, NEMOSHOW_SHADER_REF), shader));
	} else {
		NEMOSHOW_PATH_CC(path, fill)->setShader(NULL);
		NEMOSHOW_PATH_CC(path, stroke)->setShader(NULL);
	}
}

static inline void nemoshow_path_update_points(struct nemoshow *show, struct showone *one)
{
	struct showpath *path = NEMOSHOW_PATH(one);

	one->dirty |= NEMOSHOW_PATH_DIRTY;
}

static inline void nemoshow_path_update_path(struct nemoshow *show, struct showone *one)
{
	struct showpath *path = NEMOSHOW_PATH(one);
	int i;

	if (one->sub == NEMOSHOW_ARRAY_PATH) {
		NEMOSHOW_PATH_CC(path, path)->reset();

		for (i = 0; i < path->ncmds; i++) {
			if (path->cmds[i] == NEMOSHOW_PATH_MOVETO_CMD)
				NEMOSHOW_PATH_CC(path, path)->moveTo(
						path->points[NEMOSHOW_PATHARRAY_OFFSET_X0(i)],
						path->points[NEMOSHOW_PATHARRAY_OFFSET_Y0(i)]);
			else if (path->cmds[i] == NEMOSHOW_PATH_LINETO_CMD)
				NEMOSHOW_PATH_CC(path, path)->lineTo(
						path->points[NEMOSHOW_PATHARRAY_OFFSET_X0(i)],
						path->points[NEMOSHOW_PATHARRAY_OFFSET_Y0(i)]);
			else if (path->cmds[i] == NEMOSHOW_PATH_CURVETO_CMD)
				NEMOSHOW_PATH_CC(path, path)->cubicTo(
						path->points[NEMOSHOW_PATHARRAY_OFFSET_X0(i)],
						path->points[NEMOSHOW_PATHARRAY_OFFSET_Y0(i)],
						path->points[NEMOSHOW_PATHARRAY_OFFSET_X1(i)],
						path->points[NEMOSHOW_PATHARRAY_OFFSET_Y1(i)],
						path->points[NEMOSHOW_PATHARRAY_OFFSET_X2(i)],
						path->points[NEMOSHOW_PATHARRAY_OFFSET_Y2(i)]);
			else if (path->cmds[i] == NEMOSHOW_PATH_CLOSE_CMD)
				NEMOSHOW_PATH_CC(path, path)->close();
		}
	} else if (one->sub == NEMOSHOW_LIST_PATH) {
		struct showone *child;
		struct showpathcmd *pcmd;

		NEMOSHOW_PATH_CC(path, path)->reset();

		nemoshow_children_for_each(child, one) {
			pcmd = NEMOSHOW_PATHCMD(child);

			if (child->sub == NEMOSHOW_PATH_MOVETO_CMD) {
				NEMOSHOW_PATH_CC(path, path)->moveTo(pcmd->x0, pcmd->y0);
			} else if (child->sub == NEMOSHOW_PATH_LINETO_CMD) {
				NEMOSHOW_PATH_CC(path, path)->lineTo(pcmd->x0, pcmd->y0);
			} else if (child->sub == NEMOSHOW_PATH_CURVETO_CMD) {
				NEMOSHOW_PATH_CC(path, path)->cubicTo(
						pcmd->x0, pcmd->y0,
						pcmd->x1, pcmd->y1,
						pcmd->x2, pcmd->y2);
			} else if (child->sub == NEMOSHOW_PATH_CLOSE_CMD) {
				NEMOSHOW_PATH_CC(path, path)->close();
			}
		}
	}
}

static inline void nemoshow_path_update_patheffect(struct nemoshow *show, struct showone *one)
{
	struct showpath *path = NEMOSHOW_PATH(one);
	int i;

	if (path->pathsegment >= 1.0f) {
		SkPathEffect *effect;

		effect = SkDiscretePathEffect::Create(path->pathsegment, path->pathdeviation, path->pathseed);
		if (effect != NULL) {
			NEMOSHOW_PATH_CC(path, fill)->setPathEffect(effect);
			NEMOSHOW_PATH_CC(path, stroke)->setPathEffect(effect);
			effect->unref();
		}
	}

	if (path->pathdashcount > 0) {
		SkPathEffect *effect;
		SkScalar dashes[NEMOSHOW_PATH_DASH_MAX];

		for (i = 0; i < path->pathdashcount; i++)
			dashes[i] = path->pathdashes[i];

		effect = SkDashPathEffect::Create(dashes, path->pathdashcount, 0);
		if (effect != NULL) {
			NEMOSHOW_PATH_CC(path, fill)->setPathEffect(effect);
			NEMOSHOW_PATH_CC(path, stroke)->setPathEffect(effect);
			effect->unref();
		}
	}
}

int nemoshow_path_update(struct showone *one)
{
	struct nemoshow *show = one->show;
	struct showone *parent = one->parent;

	if ((one->dirty & NEMOSHOW_STYLE_DIRTY) != 0)
		nemoshow_path_update_style(show, one);
	if ((one->dirty & NEMOSHOW_FILTER_DIRTY) != 0)
		nemoshow_path_update_filter(show, one);
	if ((one->dirty & NEMOSHOW_SHADER_DIRTY) != 0)
		nemoshow_path_update_shader(show, one);
	if ((one->dirty & NEMOSHOW_POINTS_DIRTY) != 0)
		nemoshow_path_update_points(show, one);
	if ((one->dirty & NEMOSHOW_PATH_DIRTY) != 0)
		nemoshow_path_update_path(show, one);
	if ((one->dirty & NEMOSHOW_PATHEFFECT_DIRTY) != 0)
		nemoshow_path_update_patheffect(show, one);

	if (parent->canvas != NULL)
		nemoshow_canvas_damage_one(parent->canvas, parent);

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

	NEMOSHOW_PATH_CC(path, stroke)->setStrokeCap(caps[cap]);
	NEMOSHOW_PATH_CC(path, fill)->setStrokeCap(caps[cap]);
}

void nemoshow_path_set_stroke_join(struct showone *one, int join)
{
	const SkPaint::Join joins[] = {
		SkPaint::kMiter_Join,
		SkPaint::kRound_Join,
		SkPaint::kBevel_Join
	};
	struct showpath *path = NEMOSHOW_PATH(one);

	NEMOSHOW_PATH_CC(path, stroke)->setStrokeJoin(joins[join]);
	NEMOSHOW_PATH_CC(path, fill)->setStrokeJoin(joins[join]);
}

void nemoshow_path_clear(struct showone *one)
{
	struct showpath *path = NEMOSHOW_PATH(one);

	if (one->sub == NEMOSHOW_ARRAY_PATH) {
		path->ncmds = 0;
		path->npoints = 0;

		nemoshow_one_dirty(one, NEMOSHOW_POINTS_DIRTY | NEMOSHOW_PATH_DIRTY);
	} else if (one->sub == NEMOSHOW_LIST_PATH) {
		struct showone *child, *nchild;

		nemoshow_children_for_each_safe(child, nchild, one) {
			nemoshow_one_destroy(child);
		}

		nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
	} else {
		NEMOSHOW_PATH_CC(path, path)->reset();

		nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
	}
}

int nemoshow_path_moveto(struct showone *one, double x, double y)
{
	struct showpath *path = NEMOSHOW_PATH(one);

	if (one->sub == NEMOSHOW_ARRAY_PATH) {
		NEMOBOX_APPEND(path->cmds, path->scmds, path->ncmds, NEMOSHOW_PATH_MOVETO_CMD);

		NEMOBOX_APPEND(path->points, path->spoints, path->npoints, x);
		NEMOBOX_APPEND(path->points, path->spoints, path->npoints, y);
		NEMOBOX_APPEND(path->points, path->spoints, path->npoints, 0.0f);
		NEMOBOX_APPEND(path->points, path->spoints, path->npoints, 0.0f);
		NEMOBOX_APPEND(path->points, path->spoints, path->npoints, 0.0f);
		NEMOBOX_APPEND(path->points, path->spoints, path->npoints, 0.0f);

		nemoobject_set_reserved(&one->object, "points", path->points, sizeof(double) * path->npoints);

		nemoshow_one_dirty(one, NEMOSHOW_POINTS_DIRTY | NEMOSHOW_PATH_DIRTY);

		return path->ncmds - 1;
	} else if (one->sub == NEMOSHOW_PATHLIST_ITEM) {
		struct showone *child;

		child = nemoshow_pathcmd_create(NEMOSHOW_PATH_MOVETO_CMD);
		nemoshow_one_attach(one, child);

		nemoshow_pathcmd_set_x0(child, x);
		nemoshow_pathcmd_set_y0(child, y);

		nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);

		return nemoshow_children_count(one) - 1;
	} else {
		NEMOSHOW_PATH_CC(path, path)->moveTo(x, y);

		nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
	}

	return 0;
}

int nemoshow_path_lineto(struct showone *one, double x, double y)
{
	struct showpath *path = NEMOSHOW_PATH(one);

	if (one->sub == NEMOSHOW_ARRAY_PATH) {
		NEMOBOX_APPEND(path->cmds, path->scmds, path->ncmds, NEMOSHOW_PATH_LINETO_CMD);

		NEMOBOX_APPEND(path->points, path->spoints, path->npoints, x);
		NEMOBOX_APPEND(path->points, path->spoints, path->npoints, y);
		NEMOBOX_APPEND(path->points, path->spoints, path->npoints, 0.0f);
		NEMOBOX_APPEND(path->points, path->spoints, path->npoints, 0.0f);
		NEMOBOX_APPEND(path->points, path->spoints, path->npoints, 0.0f);
		NEMOBOX_APPEND(path->points, path->spoints, path->npoints, 0.0f);

		nemoobject_set_reserved(&one->object, "points", path->points, sizeof(double) * path->npoints);

		nemoshow_one_dirty(one, NEMOSHOW_POINTS_DIRTY | NEMOSHOW_PATH_DIRTY);

		return path->ncmds - 1;
	} else if (one->sub == NEMOSHOW_PATHLIST_ITEM) {
		struct showone *child;

		child = nemoshow_pathcmd_create(NEMOSHOW_PATH_LINETO_CMD);
		nemoshow_one_attach(one, child);

		nemoshow_pathcmd_set_x0(child, x);
		nemoshow_pathcmd_set_y0(child, y);

		nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);

		return nemoshow_children_count(one) - 1;
	} else {
		NEMOSHOW_PATH_CC(path, path)->lineTo(x, y);

		nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
	}

	return 0;
}

int nemoshow_path_cubicto(struct showone *one, double x0, double y0, double x1, double y1, double x2, double y2)
{
	struct showpath *path = NEMOSHOW_PATH(one);

	if (one->sub == NEMOSHOW_ARRAY_PATH) {
		NEMOBOX_APPEND(path->cmds, path->scmds, path->ncmds, NEMOSHOW_PATH_CURVETO_CMD);

		NEMOBOX_APPEND(path->points, path->spoints, path->npoints, x0);
		NEMOBOX_APPEND(path->points, path->spoints, path->npoints, y0);
		NEMOBOX_APPEND(path->points, path->spoints, path->npoints, x1);
		NEMOBOX_APPEND(path->points, path->spoints, path->npoints, y1);
		NEMOBOX_APPEND(path->points, path->spoints, path->npoints, x2);
		NEMOBOX_APPEND(path->points, path->spoints, path->npoints, y2);

		nemoobject_set_reserved(&one->object, "points", path->points, sizeof(double) * path->npoints);

		nemoshow_one_dirty(one, NEMOSHOW_POINTS_DIRTY | NEMOSHOW_PATH_DIRTY);

		return path->ncmds - 1;
	} else if (one->sub == NEMOSHOW_PATHLIST_ITEM) {
		struct showone *child;

		child = nemoshow_pathcmd_create(NEMOSHOW_PATH_CURVETO_CMD);
		nemoshow_one_attach(one, child);

		nemoshow_pathcmd_set_x0(child, x0);
		nemoshow_pathcmd_set_y0(child, y0);
		nemoshow_pathcmd_set_x1(child, x1);
		nemoshow_pathcmd_set_y1(child, y1);
		nemoshow_pathcmd_set_x2(child, x2);
		nemoshow_pathcmd_set_y2(child, y2);

		nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);

		return nemoshow_children_count(one) - 1;
	} else {
		NEMOSHOW_PATH_CC(path, path)->cubicTo(x0, y0, x1, y1, x2, y2);

		nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
	}

	return 0;
}

void nemoshow_path_arcto(struct showone *one, double x, double y, double width, double height, double from, double to, int needs_moveto)
{
	struct showpath *path = NEMOSHOW_PATH(one);
	SkRect rect = SkRect::MakeXYWH(x, y, width, height);

	NEMOSHOW_PATH_CC(path, path)->arcTo(rect, from, to, needs_moveto == 0 ? false : true);

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

int nemoshow_path_close(struct showone *one)
{
	struct showpath *path = NEMOSHOW_PATH(one);

	if (one->sub == NEMOSHOW_ARRAY_PATH) {
		NEMOBOX_APPEND(path->cmds, path->scmds, path->ncmds, NEMOSHOW_PATH_CLOSE_CMD);

		NEMOBOX_APPEND(path->points, path->spoints, path->npoints, 0.0f);
		NEMOBOX_APPEND(path->points, path->spoints, path->npoints, 0.0f);
		NEMOBOX_APPEND(path->points, path->spoints, path->npoints, 0.0f);
		NEMOBOX_APPEND(path->points, path->spoints, path->npoints, 0.0f);
		NEMOBOX_APPEND(path->points, path->spoints, path->npoints, 0.0f);
		NEMOBOX_APPEND(path->points, path->spoints, path->npoints, 0.0f);

		nemoobject_set_reserved(&one->object, "points", path->points, sizeof(double) * path->npoints);

		nemoshow_one_dirty(one, NEMOSHOW_POINTS_DIRTY | NEMOSHOW_PATH_DIRTY);

		return path->ncmds - 1;
	} else if (one->sub == NEMOSHOW_PATHLIST_ITEM) {
		struct showone *child;

		child = nemoshow_pathcmd_create(NEMOSHOW_PATH_CLOSE_CMD);
		nemoshow_one_attach(one, child);

		nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);

		return nemoshow_children_count(one) - 1;
	} else {
		NEMOSHOW_PATH_CC(path, path)->close();

		nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
	}

	return 0;
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

	nemoshow_one_dirty(one, NEMOSHOW_PATHEFFECT_DIRTY);
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

	nemoobject_set_reserved(&one->object, "pathdashes", path->pathdashes, sizeof(double) * dashcount);

	nemoshow_one_dirty(one, NEMOSHOW_PATHEFFECT_DIRTY);
}

void nemoshow_path_set_shader(struct showone *one, struct showone *shader)
{
	nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_SHADER_REF));

	if (shader != NULL)
		nemoshow_one_reference_one(one, shader, NEMOSHOW_SHADER_DIRTY, 0x0, NEMOSHOW_SHADER_REF);
}

void nemoshow_path_set_filter(struct showone *one, struct showone *filter)
{
	nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_FILTER_REF));

	if (filter != NULL)
		nemoshow_one_reference_one(one, filter, NEMOSHOW_FILTER_DIRTY, 0x0, NEMOSHOW_FILTER_REF);
}
