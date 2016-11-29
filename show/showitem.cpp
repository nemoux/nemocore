#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <skiaconfig.hpp>

#include <showitem.h>
#include <showitem.hpp>
#include <showcolor.h>
#include <showmatrix.h>
#include <showmatrix.hpp>
#include <showfilter.h>
#include <showfilter.hpp>
#include <showshader.h>
#include <showshader.hpp>
#include <showpath.h>
#include <showpath.hpp>
#include <showpathcmd.h>
#include <showfont.h>
#include <showfont.hpp>
#include <showmisc.h>
#include <nemoshow.h>
#include <skiahelper.hpp>
#include <fonthelper.h>
#include <svghelper.hpp>
#include <stringhelper.h>
#include <colorhelper.h>
#include <nemoxml.h>
#include <nemomisc.h>

#define	NEMOSHOW_ANTIALIAS_EPSILON			(5.0f)

struct showone *nemoshow_item_create(int type)
{
	struct showitem *item;
	struct showone *one;

	item = (struct showitem *)malloc(sizeof(struct showitem));
	if (item == NULL)
		return NULL;
	memset(item, 0, sizeof(struct showitem));

	item->cc = new showitem_t;
	NEMOSHOW_ITEM_CC(item, matrix) = new SkMatrix;
	NEMOSHOW_ITEM_CC(item, inverse) = new SkMatrix;
	NEMOSHOW_ITEM_CC(item, modelview) = new SkMatrix;
	NEMOSHOW_ITEM_CC(item, viewbox) = new SkMatrix;
	NEMOSHOW_ITEM_CC(item, path) = new SkPath;
	NEMOSHOW_ITEM_CC(item, fillpath) = NULL;
	NEMOSHOW_ITEM_CC(item, subpath) = NULL;
	NEMOSHOW_ITEM_CC(item, fill) = new SkPaint;
	NEMOSHOW_ITEM_CC(item, fill)->setStyle(SkPaint::kFill_Style);
	NEMOSHOW_ITEM_CC(item, fill)->setStrokeCap(SkPaint::kRound_Cap);
	NEMOSHOW_ITEM_CC(item, fill)->setStrokeJoin(SkPaint::kRound_Join);
	NEMOSHOW_ITEM_CC(item, fill)->setAntiAlias(true);
	NEMOSHOW_ITEM_CC(item, stroke) = new SkPaint;
	NEMOSHOW_ITEM_CC(item, stroke)->setStyle(SkPaint::kStroke_Style);
	NEMOSHOW_ITEM_CC(item, stroke)->setStrokeCap(SkPaint::kRound_Cap);
	NEMOSHOW_ITEM_CC(item, stroke)->setStrokeJoin(SkPaint::kRound_Join);
	NEMOSHOW_ITEM_CC(item, stroke)->setAntiAlias(true);
	NEMOSHOW_ITEM_CC(item, measure) = new SkPathMeasure;
	NEMOSHOW_ITEM_CC(item, points) = NULL;
	NEMOSHOW_ITEM_CC(item, textbox) = NULL;
	NEMOSHOW_ITEM_CC(item, bitmap) = NULL;
	NEMOSHOW_ITEM_CC(item, needs_free) = false;
	NEMOSHOW_ITEM_CC(item, width) = 0;
	NEMOSHOW_ITEM_CC(item, height) = 0;

	item->alpha = 1.0f;
	item->from = 0.0f;
	item->to = 1.0f;

	item->sx = 1.0f;
	item->sy = 1.0f;

	item->fills[NEMOSHOW_ALPHA_COLOR] = 255.0f;
	item->fills[NEMOSHOW_RED_COLOR] = 255.0f;
	item->fills[NEMOSHOW_GREEN_COLOR] = 255.0f;
	item->fills[NEMOSHOW_BLUE_COLOR] = 255.0f;
	item->strokes[NEMOSHOW_ALPHA_COLOR] = 255.0f;
	item->strokes[NEMOSHOW_RED_COLOR] = 255.0f;
	item->strokes[NEMOSHOW_GREEN_COLOR] = 255.0f;
	item->strokes[NEMOSHOW_BLUE_COLOR] = 255.0f;

	item->_alpha = 1.0f;

	item->_fills[NEMOSHOW_ALPHA_COLOR] = 255.0f;
	item->_fills[NEMOSHOW_RED_COLOR] = 255.0f;
	item->_fills[NEMOSHOW_GREEN_COLOR] = 255.0f;
	item->_fills[NEMOSHOW_BLUE_COLOR] = 255.0f;
	item->_strokes[NEMOSHOW_ALPHA_COLOR] = 255.0f;
	item->_strokes[NEMOSHOW_RED_COLOR] = 255.0f;
	item->_strokes[NEMOSHOW_GREEN_COLOR] = 255.0f;
	item->_strokes[NEMOSHOW_BLUE_COLOR] = 255.0f;

	one = &item->base;
	one->type = NEMOSHOW_ITEM_TYPE;
	one->sub = type;
	one->update = nemoshow_item_update;
	one->destroy = nemoshow_item_destroy;
	one->attach = nemoshow_item_attach_one;
	one->detach = nemoshow_item_detach_one;
	one->above = nemoshow_item_above_one;
	one->below = nemoshow_item_below_one;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "x", &item->x, sizeof(double));
	nemoobject_set_reserved(&one->object, "y", &item->y, sizeof(double));
	nemoobject_set_reserved(&one->object, "ox", &item->ox, sizeof(double));
	nemoobject_set_reserved(&one->object, "oy", &item->oy, sizeof(double));
	nemoobject_set_reserved(&one->object, "width", &item->width, sizeof(double));
	nemoobject_set_reserved(&one->object, "height", &item->height, sizeof(double));
	nemoobject_set_reserved(&one->object, "cx", &item->cx, sizeof(double));
	nemoobject_set_reserved(&one->object, "cy", &item->cy, sizeof(double));
	nemoobject_set_reserved(&one->object, "r", &item->r, sizeof(double));

	nemoobject_set_reserved(&one->object, "tx", &item->tx, sizeof(double));
	nemoobject_set_reserved(&one->object, "ty", &item->ty, sizeof(double));
	nemoobject_set_reserved(&one->object, "sx", &item->sx, sizeof(double));
	nemoobject_set_reserved(&one->object, "sy", &item->sy, sizeof(double));
	nemoobject_set_reserved(&one->object, "px", &item->px, sizeof(double));
	nemoobject_set_reserved(&one->object, "py", &item->py, sizeof(double));
	nemoobject_set_reserved(&one->object, "ro", &item->ro, sizeof(double));

	nemoobject_set_reserved(&one->object, "ax", &item->ax, sizeof(double));
	nemoobject_set_reserved(&one->object, "ay", &item->ay, sizeof(double));

	nemoobject_set_reserved(&one->object, "from", &item->from, sizeof(double));
	nemoobject_set_reserved(&one->object, "to", &item->to, sizeof(double));

	nemoobject_set_reserved(&one->object, "stroke", item->_strokes, sizeof(double[4]));
	nemoobject_set_reserved(&one->object, "stroke-width", &item->stroke_width, sizeof(double));
	nemoobject_set_reserved(&one->object, "fill", item->_fills, sizeof(double[4]));

	nemoobject_set_reserved(&one->object, "font-size", &item->fontsize, sizeof(double));

	nemoobject_set_reserved(&one->object, "pathsegment", &item->pathsegment, sizeof(double));
	nemoobject_set_reserved(&one->object, "pathdeviation", &item->pathdeviation, sizeof(double));
	nemoobject_set_reserved(&one->object, "pathseed", &item->pathseed, sizeof(uint32_t));

	nemoobject_set_reserved(&one->object, "alpha", &item->_alpha, sizeof(double));

	if (one->sub == NEMOSHOW_TEXT_ITEM) {
		item->points = (double *)malloc(sizeof(double) * 8);
		item->npoints = 0;
		item->spoints = 8;
	} else if (one->sub == NEMOSHOW_TEXTBOX_ITEM) {
		NEMOSHOW_ITEM_CC(item, textbox) = new SkTextBox;
		NEMOSHOW_ITEM_CC(item, textbox)->setMode(SkTextBox::kLineBreak_Mode);
	} else if (one->sub == NEMOSHOW_PATH_ITEM) {
		NEMOSHOW_ITEM_CC(item, subpath) = new SkPath;
	} else if (one->sub == NEMOSHOW_PATHTWICE_ITEM) {
		NEMOSHOW_ITEM_CC(item, fillpath) = new SkPath;
		NEMOSHOW_ITEM_CC(item, subpath) = new SkPath;
	} else if (one->sub == NEMOSHOW_PATHARRAY_ITEM) {
		NEMOSHOW_ITEM_CC(item, subpath) = new SkPath;

		item->cmds = (uint32_t *)malloc(sizeof(uint32_t) * 8);
		item->ncmds = 0;
		item->scmds = 8;

		item->points = (double *)malloc(sizeof(double) * 8);
		item->npoints = 0;
		item->spoints = 8;

		nemoobject_set_reserved(&one->object, "points", item->points, sizeof(double) * item->spoints);
	} else if (one->sub == NEMOSHOW_PATHLIST_ITEM) {
		NEMOSHOW_ITEM_CC(item, subpath) = new SkPath;
	} else if (one->sub == NEMOSHOW_PATHGROUP_ITEM) {
		nemoshow_one_set_state(one, NEMOSHOW_INHERIT_STATE);
		nemoshow_one_set_state(one, NEMOSHOW_FILL_STATE);
		nemoshow_one_set_state(one, NEMOSHOW_STROKE_STATE);
	} else if (one->sub == NEMOSHOW_POINTS_ITEM || one->sub == NEMOSHOW_POLYLINE_ITEM || one->sub == NEMOSHOW_POLYGON_ITEM) {
		item->points = (double *)malloc(sizeof(double) * 8);
		item->npoints = 0;
		item->spoints = 8;

		nemoobject_set_reserved(&one->object, "points", item->points, sizeof(double) * item->spoints);
	} else if (one->sub == NEMOSHOW_GROUP_ITEM) {
		nemoshow_one_set_state(one, NEMOSHOW_INHERIT_STATE);
		nemoshow_one_set_state(one, NEMOSHOW_FILL_STATE);
		nemoshow_one_set_state(one, NEMOSHOW_STROKE_STATE);
		nemoshow_one_set_state(one, NEMOSHOW_BOUNDS_STATE);
	} else if (one->sub == NEMOSHOW_IMAGE_ITEM) {
		nemoshow_one_set_state(one, NEMOSHOW_FILL_STATE);
	} else if (one->sub == NEMOSHOW_CONTAINER_ITEM) {
		nemoshow_one_set_state(one, NEMOSHOW_INHERIT_STATE);
		nemoshow_one_set_state(one, NEMOSHOW_FILL_STATE);
		nemoshow_one_set_state(one, NEMOSHOW_STROKE_STATE);
		nemoshow_one_set_state(one, NEMOSHOW_VIEWPORT_STATE);
		nemoshow_one_set_state(one, NEMOSHOW_TRANSFORM_STATE);
	}

	return one;
}

void nemoshow_item_destroy(struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	nemoshow_one_finish(one);

	if (NEMOSHOW_ITEM_CC(item, matrix) != NULL)
		delete NEMOSHOW_ITEM_CC(item, matrix);
	if (NEMOSHOW_ITEM_CC(item, inverse) != NULL)
		delete NEMOSHOW_ITEM_CC(item, inverse);
	if (NEMOSHOW_ITEM_CC(item, modelview) != NULL)
		delete NEMOSHOW_ITEM_CC(item, modelview);
	if (NEMOSHOW_ITEM_CC(item, viewbox) != NULL)
		delete NEMOSHOW_ITEM_CC(item, viewbox);
	if (NEMOSHOW_ITEM_CC(item, path) != NULL)
		delete NEMOSHOW_ITEM_CC(item, path);
	if (NEMOSHOW_ITEM_CC(item, fillpath) != NULL)
		delete NEMOSHOW_ITEM_CC(item, fillpath);
	if (NEMOSHOW_ITEM_CC(item, subpath) != NULL)
		delete NEMOSHOW_ITEM_CC(item, subpath);
	if (NEMOSHOW_ITEM_CC(item, fill) != NULL)
		delete NEMOSHOW_ITEM_CC(item, fill);
	if (NEMOSHOW_ITEM_CC(item, stroke) != NULL)
		delete NEMOSHOW_ITEM_CC(item, stroke);
	if (NEMOSHOW_ITEM_CC(item, measure) != NULL)
		delete NEMOSHOW_ITEM_CC(item, measure);
	if (NEMOSHOW_ITEM_CC(item, points) != NULL)
		delete[] NEMOSHOW_ITEM_CC(item, points);
	if (NEMOSHOW_ITEM_CC(item, textbox) != NULL)
		delete NEMOSHOW_ITEM_CC(item, textbox);
	if (NEMOSHOW_ITEM_CC(item, needs_free) == true && NEMOSHOW_ITEM_CC(item, bitmap) != NULL)
		delete NEMOSHOW_ITEM_CC(item, bitmap);

	delete static_cast<showitem_t *>(item->cc);

	if (item->cmds != NULL)
		free(item->cmds);
	if (item->points != NULL)
		free(item->points);
	if (item->pathdashes != NULL)
		free(item->pathdashes);
	if (item->text != NULL)
		free(item->text);
	if (item->uri != NULL)
		free(item->uri);

	free(item);
}

static inline void nemoshow_item_update_uri(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (item->uri != NULL) {
		if (NEMOSHOW_ITEM_CC(item, needs_free) == true && NEMOSHOW_ITEM_CC(item, bitmap) != NULL)
			delete NEMOSHOW_ITEM_CC(item, bitmap);

		if (one->sub == NEMOSHOW_IMAGE_ITEM) {
			NEMOSHOW_ITEM_CC(item, bitmap) = new SkBitmap;
			NEMOSHOW_ITEM_CC(item, needs_free) = true;

			if (skia_read_image(NEMOSHOW_ITEM_CC(item, bitmap), item->uri) < 0) {
				delete NEMOSHOW_ITEM_CC(item, bitmap);

				NEMOSHOW_ITEM_CC(item, bitmap) = NULL;
			}

			one->dirty |= NEMOSHOW_SHAPE_DIRTY;
		}
	}
}

static inline void nemoshow_item_update_style(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showitem *group;

	if (nemoshow_has_state(show, NEMOSHOW_ANTIALIAS_STATE)) {
		NEMOSHOW_ITEM_CC(item, fill)->setAntiAlias(true);
		NEMOSHOW_ITEM_CC(item, stroke)->setAntiAlias(true);
	} else {
		NEMOSHOW_ITEM_CC(item, fill)->setAntiAlias(false);
		NEMOSHOW_ITEM_CC(item, stroke)->setAntiAlias(false);
	}

	if (nemoshow_one_has_state(one, NEMOSHOW_FILL_STATE)) {
		if (one->parent != NULL && nemoshow_one_has_state(one->parent, NEMOSHOW_INHERIT_STATE)) {
			group = NEMOSHOW_ITEM(one->parent);

			item->alpha = item->_alpha * group->alpha;

			item->fills[NEMOSHOW_ALPHA_COLOR] = item->_fills[NEMOSHOW_ALPHA_COLOR] * (group->fills[NEMOSHOW_ALPHA_COLOR] / 255.0f);
			item->fills[NEMOSHOW_RED_COLOR] = item->_fills[NEMOSHOW_RED_COLOR] * (group->fills[NEMOSHOW_RED_COLOR] / 255.0f);
			item->fills[NEMOSHOW_GREEN_COLOR] = item->_fills[NEMOSHOW_GREEN_COLOR] * (group->fills[NEMOSHOW_GREEN_COLOR] / 255.0f);
			item->fills[NEMOSHOW_BLUE_COLOR] = item->_fills[NEMOSHOW_BLUE_COLOR] * (group->fills[NEMOSHOW_BLUE_COLOR] / 255.0f);
		} else {
			item->alpha = item->_alpha;

			item->fills[NEMOSHOW_ALPHA_COLOR] = item->_fills[NEMOSHOW_ALPHA_COLOR];
			item->fills[NEMOSHOW_RED_COLOR] = item->_fills[NEMOSHOW_RED_COLOR];
			item->fills[NEMOSHOW_GREEN_COLOR] = item->_fills[NEMOSHOW_GREEN_COLOR];
			item->fills[NEMOSHOW_BLUE_COLOR] = item->_fills[NEMOSHOW_BLUE_COLOR];
		}

		NEMOSHOW_ITEM_CC(item, fill)->setColor(
				SkColorSetARGB(
					item->fills[NEMOSHOW_ALPHA_COLOR] * item->alpha,
					item->fills[NEMOSHOW_RED_COLOR],
					item->fills[NEMOSHOW_GREEN_COLOR],
					item->fills[NEMOSHOW_BLUE_COLOR]));
	}
	if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE)) {
		if (one->parent != NULL && nemoshow_one_has_state(one->parent, NEMOSHOW_INHERIT_STATE)) {
			group = NEMOSHOW_ITEM(one->parent);

			item->alpha = item->_alpha * group->alpha;

			item->strokes[NEMOSHOW_ALPHA_COLOR] = item->_strokes[NEMOSHOW_ALPHA_COLOR] * (group->strokes[NEMOSHOW_ALPHA_COLOR] / 255.0f);
			item->strokes[NEMOSHOW_RED_COLOR] = item->_strokes[NEMOSHOW_RED_COLOR] * (group->strokes[NEMOSHOW_RED_COLOR] / 255.0f);
			item->strokes[NEMOSHOW_GREEN_COLOR] = item->_strokes[NEMOSHOW_GREEN_COLOR] * (group->strokes[NEMOSHOW_GREEN_COLOR] / 255.0f);
			item->strokes[NEMOSHOW_BLUE_COLOR] = item->_strokes[NEMOSHOW_BLUE_COLOR] * (group->strokes[NEMOSHOW_BLUE_COLOR] / 255.0f);
		} else {
			item->alpha = item->_alpha;

			item->strokes[NEMOSHOW_ALPHA_COLOR] = item->_strokes[NEMOSHOW_ALPHA_COLOR];
			item->strokes[NEMOSHOW_RED_COLOR] = item->_strokes[NEMOSHOW_RED_COLOR];
			item->strokes[NEMOSHOW_GREEN_COLOR] = item->_strokes[NEMOSHOW_GREEN_COLOR];
			item->strokes[NEMOSHOW_BLUE_COLOR] = item->_strokes[NEMOSHOW_BLUE_COLOR];
		}

		NEMOSHOW_ITEM_CC(item, stroke)->setStrokeWidth(item->stroke_width);
		NEMOSHOW_ITEM_CC(item, stroke)->setColor(
				SkColorSetARGB(
					item->strokes[NEMOSHOW_ALPHA_COLOR] * item->alpha * (item->stroke_width > 0.0f ? 1.0f : 0.0f),
					item->strokes[NEMOSHOW_RED_COLOR],
					item->strokes[NEMOSHOW_GREEN_COLOR],
					item->strokes[NEMOSHOW_BLUE_COLOR]));
	}
}

static inline void nemoshow_item_update_filter(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (NEMOSHOW_REF(one, NEMOSHOW_FILTER_REF) != NULL && nemoshow_has_state(show, NEMOSHOW_FILTER_STATE)) {
		struct showone *ref = NEMOSHOW_REF(one, NEMOSHOW_FILTER_REF);
		struct showfilter *filter = NEMOSHOW_FILTER(ref);

		if (filter->type == NEMOSHOW_FILTER_MASK_TYPE) {
			NEMOSHOW_ITEM_CC(item, fill)->setMaskFilter(NEMOSHOW_FILTER_CC(filter, maskfilter));
			NEMOSHOW_ITEM_CC(item, stroke)->setMaskFilter(NEMOSHOW_FILTER_CC(filter, maskfilter));

			if (one->sub == NEMOSHOW_PATHGROUP_ITEM) {
				struct showone *child;

				nemoshow_children_for_each(child, one) {
					NEMOSHOW_PATH_ATCC(child, fill)->setMaskFilter(NEMOSHOW_FILTER_CC(filter, maskfilter));
					NEMOSHOW_PATH_ATCC(child, stroke)->setMaskFilter(NEMOSHOW_FILTER_CC(filter, maskfilter));
				}
			}
		} else if (filter->type == NEMOSHOW_FILTER_IMAGE_TYPE) {
			NEMOSHOW_ITEM_CC(item, fill)->setImageFilter(NEMOSHOW_FILTER_CC(filter, imagefilter));
			NEMOSHOW_ITEM_CC(item, stroke)->setImageFilter(NEMOSHOW_FILTER_CC(filter, imagefilter));

			if (one->sub == NEMOSHOW_PATHGROUP_ITEM) {
				struct showone *child;

				nemoshow_children_for_each(child, one) {
					NEMOSHOW_PATH_ATCC(child, fill)->setImageFilter(NEMOSHOW_FILTER_CC(filter, imagefilter));
					NEMOSHOW_PATH_ATCC(child, stroke)->setImageFilter(NEMOSHOW_FILTER_CC(filter, imagefilter));
				}
			}
		} else if (filter->type == NEMOSHOW_FILTER_COLOR_TYPE) {
			NEMOSHOW_ITEM_CC(item, fill)->setColorFilter(NEMOSHOW_FILTER_CC(filter, colorfilter));
			NEMOSHOW_ITEM_CC(item, stroke)->setColorFilter(NEMOSHOW_FILTER_CC(filter, colorfilter));

			if (one->sub == NEMOSHOW_PATHGROUP_ITEM) {
				struct showone *child;

				nemoshow_children_for_each(child, one) {
					NEMOSHOW_PATH_ATCC(child, fill)->setColorFilter(NEMOSHOW_FILTER_CC(filter, colorfilter));
					NEMOSHOW_PATH_ATCC(child, stroke)->setColorFilter(NEMOSHOW_FILTER_CC(filter, colorfilter));
				}
			}
		}
	} else {
		NEMOSHOW_ITEM_CC(item, fill)->setMaskFilter(NULL);
		NEMOSHOW_ITEM_CC(item, stroke)->setMaskFilter(NULL);
		NEMOSHOW_ITEM_CC(item, fill)->setImageFilter(NULL);
		NEMOSHOW_ITEM_CC(item, stroke)->setImageFilter(NULL);
		NEMOSHOW_ITEM_CC(item, fill)->setColorFilter(NULL);
		NEMOSHOW_ITEM_CC(item, stroke)->setColorFilter(NULL);

		if (one->sub == NEMOSHOW_PATHGROUP_ITEM) {
			struct showone *child;

			nemoshow_children_for_each(child, one) {
				NEMOSHOW_PATH_ATCC(child, fill)->setMaskFilter(NULL);
				NEMOSHOW_PATH_ATCC(child, stroke)->setMaskFilter(NULL);
				NEMOSHOW_PATH_ATCC(child, fill)->setImageFilter(NULL);
				NEMOSHOW_PATH_ATCC(child, stroke)->setImageFilter(NULL);
				NEMOSHOW_PATH_ATCC(child, fill)->setColorFilter(NULL);
				NEMOSHOW_PATH_ATCC(child, stroke)->setColorFilter(NULL);
			}
		}
	}

	one->dirty |= NEMOSHOW_BOUNDS_DIRTY;
}

static inline void nemoshow_item_update_shader(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (NEMOSHOW_REF(one, NEMOSHOW_SHADER_REF) != NULL) {
		NEMOSHOW_ITEM_CC(item, fill)->setShader(NEMOSHOW_SHADER_ATCC(NEMOSHOW_REF(one, NEMOSHOW_SHADER_REF), shader));
		NEMOSHOW_ITEM_CC(item, stroke)->setShader(NEMOSHOW_SHADER_ATCC(NEMOSHOW_REF(one, NEMOSHOW_SHADER_REF), shader));
	} else {
		NEMOSHOW_ITEM_CC(item, fill)->setShader(NULL);
		NEMOSHOW_ITEM_CC(item, stroke)->setShader(NULL);
	}

	one->dirty |= NEMOSHOW_BOUNDS_DIRTY;
}

static inline void nemoshow_item_update_font(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (NEMOSHOW_REF(one, NEMOSHOW_FONT_REF) != NULL) {
		NEMOSHOW_ITEM_CC(item, fill)->setTypeface(
				NEMOSHOW_FONT_CC(NEMOSHOW_FONT(NEMOSHOW_REF(one, NEMOSHOW_FONT_REF)), face));
		NEMOSHOW_ITEM_CC(item, stroke)->setTypeface(
				NEMOSHOW_FONT_CC(NEMOSHOW_FONT(NEMOSHOW_REF(one, NEMOSHOW_FONT_REF)), face));

		one->dirty |= NEMOSHOW_TEXT_DIRTY;
	}
}

static inline void nemoshow_item_update_text(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (one->sub == NEMOSHOW_TEXT_ITEM) {
		if (item->text != NULL) {
			if (NEMOSHOW_FONT_AT(NEMOSHOW_REF(one, NEMOSHOW_FONT_REF), layout) == NEMOSHOW_NORMAL_LAYOUT) {
				SkPaint::FontMetrics metrics;

				NEMOSHOW_ITEM_CC(item, fill)->setTextSize(item->fontsize);
				NEMOSHOW_ITEM_CC(item, stroke)->setTextSize(item->fontsize);

				NEMOSHOW_ITEM_CC(item, stroke)->getFontMetrics(&metrics, 0);
				item->fontascent = metrics.fAscent;
				item->fontdescent = metrics.fDescent;
			} else {
				SkPaint::FontMetrics metrics;
				hb_buffer_t *hbbuffer;
				hb_glyph_info_t *hbglyphs;
				hb_glyph_position_t *hbglyphspos;
				double fontscale;
				unsigned int nhbglyphinfos;
				unsigned int nhbglyphs;
				int i;

				NEMOSHOW_ITEM_CC(item, fill)->setTextSize((NEMOSHOW_FONT_AT(NEMOSHOW_REF(one, NEMOSHOW_FONT_REF), upem) / NEMOSHOW_FONT_AT(NEMOSHOW_REF(one, NEMOSHOW_FONT_REF), max_advance_height)) * item->fontsize);
				NEMOSHOW_ITEM_CC(item, stroke)->setTextSize((NEMOSHOW_FONT_AT(NEMOSHOW_REF(one, NEMOSHOW_FONT_REF), upem) / NEMOSHOW_FONT_AT(NEMOSHOW_REF(one, NEMOSHOW_FONT_REF), max_advance_height)) * item->fontsize);

				NEMOSHOW_ITEM_CC(item, stroke)->getFontMetrics(&metrics, 0);
				item->fontascent = metrics.fAscent;
				item->fontdescent = metrics.fDescent;

				hbbuffer = hb_buffer_create();
				hb_buffer_add_utf8(hbbuffer, item->text, strlen(item->text), 0, strlen(item->text));
				hb_buffer_set_direction(hbbuffer, HB_DIRECTION_LTR);
				hb_buffer_set_script(hbbuffer, HB_SCRIPT_LATIN);
				hb_buffer_set_language(hbbuffer, HB_LANGUAGE_INVALID);
				hb_shape_full(NEMOSHOW_FONT_AT(NEMOSHOW_REF(one, NEMOSHOW_FONT_REF), hbfont), hbbuffer, NULL, 0, NULL);

				hbglyphs = hb_buffer_get_glyph_infos(hbbuffer, &nhbglyphinfos);
				hbglyphspos = hb_buffer_get_glyph_positions(hbbuffer, &nhbglyphs);

				fontscale = item->fontsize / NEMOSHOW_FONT_AT(NEMOSHOW_REF(one, NEMOSHOW_FONT_REF), max_advance_height);

				item->npoints = 0;
				item->textwidth = 0.0f;

				for (i = 0; i < nhbglyphs; i++) {
					NEMOSHOW_ARRAY_APPEND(item->points, item->spoints, item->npoints, hbglyphspos[i].x_offset * fontscale + item->textwidth + item->x);
					NEMOSHOW_ARRAY_APPEND(item->points, item->spoints, item->npoints, item->y - item->fontascent);

					item->textwidth += hbglyphspos[i].x_advance * fontscale;
				}

				item->textheight = item->fontdescent - item->fontascent;

				hb_buffer_destroy(hbbuffer);

				nemoobject_set_reserved(&one->object, "points", item->points, sizeof(double) * item->npoints);

				one->dirty |= NEMOSHOW_POINTS_DIRTY;
			}
		} else {
			item->textwidth = 0.0f;
			item->textheight = 0.0f;
		}

		one->dirty |= NEMOSHOW_SHAPE_DIRTY;
	} else if (one->sub == NEMOSHOW_TEXTBOX_ITEM) {
		if (item->text != NULL) {
			NEMOSHOW_ITEM_CC(item, fill)->setTextSize(item->fontsize);
			NEMOSHOW_ITEM_CC(item, stroke)->setTextSize(item->fontsize);
			NEMOSHOW_ITEM_CC(item, textbox)->setSpacing(item->spacingmul, item->spacingadd);
		}

		one->dirty |= NEMOSHOW_SHAPE_DIRTY;
	}
}

static inline void nemoshow_item_update_points(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (one->sub == NEMOSHOW_TEXT_ITEM) {
		if (NEMOSHOW_FONT_AT(NEMOSHOW_REF(one, NEMOSHOW_FONT_REF), layout) == NEMOSHOW_HARFBUZZ_LAYOUT) {
			int i;

			if (NEMOSHOW_ITEM_CC(item, points) != NULL)
				delete[] NEMOSHOW_ITEM_CC(item, points);

			NEMOSHOW_ITEM_CC(item, points) = new SkPoint[item->npoints / 2];

			for (i = 0; i < item->npoints / 2; i++) {
				NEMOSHOW_ITEM_CC(item, points)[i].set(
						item->points[i * 2 + 0],
						item->points[i * 2 + 1]);
			}

			one->dirty |= NEMOSHOW_SHAPE_DIRTY;
		}
	} else if (one->sub == NEMOSHOW_POINTS_ITEM || one->sub == NEMOSHOW_POLYLINE_ITEM || one->sub == NEMOSHOW_POLYGON_ITEM) {
		int i;

		if (NEMOSHOW_ITEM_CC(item, points) != NULL)
			delete[] NEMOSHOW_ITEM_CC(item, points);

		NEMOSHOW_ITEM_CC(item, points) = new SkPoint[item->npoints / 2];

		for (i = 0; i < item->npoints / 2; i++) {
			NEMOSHOW_ITEM_CC(item, points)[i].set(
					item->points[i * 2 + 0],
					item->points[i * 2 + 1]);
		}

		one->dirty |= NEMOSHOW_SHAPE_DIRTY;
	} else if (one->sub == NEMOSHOW_PATHARRAY_ITEM) {
		one->dirty |= NEMOSHOW_PATH_DIRTY;
	}
}

static inline void nemoshow_item_update_path(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (one->sub == NEMOSHOW_PATH_ITEM || one->sub == NEMOSHOW_PATHTWICE_ITEM) {
		NEMOSHOW_ITEM_CC(item, measure)->setPath(NEMOSHOW_ITEM_CC(item, path), false);

		one->dirty |= NEMOSHOW_SHAPE_DIRTY;
	} else if (one->sub == NEMOSHOW_PATHARRAY_ITEM) {
		int i;

		NEMOSHOW_ITEM_CC(item, path)->reset();

		for (i = 0; i < item->ncmds; i++) {
			if (item->cmds[i] == NEMOSHOW_PATH_MOVETO_CMD)
				NEMOSHOW_ITEM_CC(item, path)->moveTo(
						item->points[NEMOSHOW_PATHARRAY_OFFSET_X0(i)],
						item->points[NEMOSHOW_PATHARRAY_OFFSET_Y0(i)]);
			else if (item->cmds[i] == NEMOSHOW_PATH_LINETO_CMD)
				NEMOSHOW_ITEM_CC(item, path)->lineTo(
						item->points[NEMOSHOW_PATHARRAY_OFFSET_X0(i)],
						item->points[NEMOSHOW_PATHARRAY_OFFSET_Y0(i)]);
			else if (item->cmds[i] == NEMOSHOW_PATH_CURVETO_CMD)
				NEMOSHOW_ITEM_CC(item, path)->cubicTo(
						item->points[NEMOSHOW_PATHARRAY_OFFSET_X0(i)],
						item->points[NEMOSHOW_PATHARRAY_OFFSET_Y0(i)],
						item->points[NEMOSHOW_PATHARRAY_OFFSET_X1(i)],
						item->points[NEMOSHOW_PATHARRAY_OFFSET_Y1(i)],
						item->points[NEMOSHOW_PATHARRAY_OFFSET_X2(i)],
						item->points[NEMOSHOW_PATHARRAY_OFFSET_Y2(i)]);
			else if (item->cmds[i] == NEMOSHOW_PATH_CLOSE_CMD)
				NEMOSHOW_ITEM_CC(item, path)->close();
		}

		NEMOSHOW_ITEM_CC(item, measure)->setPath(NEMOSHOW_ITEM_CC(item, path), false);

		one->dirty |= NEMOSHOW_SHAPE_DIRTY;
	} else if (one->sub == NEMOSHOW_PATHLIST_ITEM) {
		struct showone *child;
		struct showpathcmd *pcmd;

		NEMOSHOW_ITEM_CC(item, path)->reset();

		nemoshow_children_for_each(child, one) {
			pcmd = NEMOSHOW_PATHCMD(child);

			if (child->sub == NEMOSHOW_PATH_MOVETO_CMD) {
				NEMOSHOW_ITEM_CC(item, path)->moveTo(pcmd->x0, pcmd->y0);
			} else if (child->sub == NEMOSHOW_PATH_LINETO_CMD) {
				NEMOSHOW_ITEM_CC(item, path)->lineTo(pcmd->x0, pcmd->y0);
			} else if (child->sub == NEMOSHOW_PATH_CURVETO_CMD) {
				NEMOSHOW_ITEM_CC(item, path)->cubicTo(
						pcmd->x0, pcmd->y0,
						pcmd->x1, pcmd->y1,
						pcmd->x2, pcmd->y2);
			} else if (child->sub == NEMOSHOW_PATH_CLOSE_CMD) {
				NEMOSHOW_ITEM_CC(item, path)->close();
			}
		}

		NEMOSHOW_ITEM_CC(item, measure)->setPath(NEMOSHOW_ITEM_CC(item, path), false);

		one->dirty |= NEMOSHOW_SHAPE_DIRTY;
	}
}

static inline void nemoshow_item_update_patheffect(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (one->sub == NEMOSHOW_PATH_ITEM || one->sub == NEMOSHOW_PATHTWICE_ITEM || one->sub == NEMOSHOW_PATHARRAY_ITEM || one->sub == NEMOSHOW_PATHLIST_ITEM) {
		if (item->pathsegment >= 1.0f) {
			sk_sp<SkPathEffect> effect;

			effect = SkDiscretePathEffect::Make(item->pathsegment, item->pathdeviation, item->pathseed);
			if (effect != NULL) {
				NEMOSHOW_ITEM_CC(item, stroke)->setPathEffect(effect);
				NEMOSHOW_ITEM_CC(item, fill)->setPathEffect(effect);
			}
		}

		if (item->pathdashcount > 0) {
			sk_sp<SkPathEffect> effect;
			SkScalar dashes[NEMOSHOW_ITEM_PATH_DASH_MAX];
			int i;

			for (i = 0; i < item->pathdashcount; i++)
				dashes[i] = item->pathdashes[i];

			effect = SkDashPathEffect::Make(dashes, item->pathdashcount, 0);
			if (effect != NULL) {
				NEMOSHOW_ITEM_CC(item, stroke)->setPathEffect(effect);
				NEMOSHOW_ITEM_CC(item, fill)->setPathEffect(effect);
			}
		}

		one->dirty |= NEMOSHOW_SHAPE_DIRTY;
	}
}

static inline void nemoshow_item_update_shape(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (one->sub == NEMOSHOW_CIRCLE_ITEM) {
		item->x = item->cx - item->r;
		item->y = item->cy - item->r;
		item->width = item->r * 2;
		item->height = item->r * 2;

		one->dirty |= NEMOSHOW_SIZE_DIRTY;
	} else if (one->sub == NEMOSHOW_PATH_ITEM || one->sub == NEMOSHOW_PATHTWICE_ITEM || one->sub == NEMOSHOW_PATHARRAY_ITEM || one->sub == NEMOSHOW_PATHLIST_ITEM) {
		if (item->from != 0.0f || item->to != 1.0f) {
			NEMOSHOW_ITEM_CC(item, subpath)->reset();

			NEMOSHOW_ITEM_CC(item, measure)->getSegment(
					NEMOSHOW_ITEM_CC(item, measure)->getLength() * item->from,
					NEMOSHOW_ITEM_CC(item, measure)->getLength() * item->to,
					NEMOSHOW_ITEM_CC(item, subpath), true);
		}

		if (nemoshow_one_has_state(one, NEMOSHOW_SIZE_STATE) == 0) {
			SkRect box;

			if (item->from == 0.0f && item->to == 1.0f)
				box = NEMOSHOW_ITEM_CC(item, path)->getBounds();
			else
				box = NEMOSHOW_ITEM_CC(item, subpath)->getBounds();

			if (item->pathsegment >= 1.0f)
				box.outset(fabs(item->pathdeviation), fabs(item->pathdeviation));

			item->x = box.x();
			item->y = box.y();
			item->width = box.width();
			item->height = box.height();

			one->dirty |= NEMOSHOW_SIZE_DIRTY;
		}
	} else if (one->sub == NEMOSHOW_POINTS_ITEM || one->sub == NEMOSHOW_POLYLINE_ITEM || one->sub == NEMOSHOW_POLYGON_ITEM) {
		int32_t x0 = INT_MAX, y0 = INT_MAX, x1 = INT_MIN, y1 = INT_MIN;
		int i;

		for (i = 0; i < item->npoints / 2; i++) {
			if (item->points[i * 2 + 0] < x0)
				x0 = item->points[i * 2 + 0];
			if (item->points[i * 2 + 0] > x1)
				x1 = item->points[i * 2 + 0];
			if (item->points[i * 2 + 1] < y0)
				y0 = item->points[i * 2 + 1];
			if (item->points[i * 2 + 1] > y1)
				y1 = item->points[i * 2 + 1];
		}

		item->x = x0;
		item->y = y0;
		item->width = x1 - x0;
		item->height = y1 - y0;

		one->dirty |= NEMOSHOW_SIZE_DIRTY;
	} else if (one->sub == NEMOSHOW_TEXT_ITEM) {
		SkRect box;

		if (NEMOSHOW_REF(one, NEMOSHOW_PATH_REF) == NULL) {
			if (NEMOSHOW_FONT_AT(NEMOSHOW_REF(one, NEMOSHOW_FONT_REF), layout) == NEMOSHOW_NORMAL_LAYOUT) {
				SkScalar widths[strlen(item->text)];
				double width = 0.0f;
				int i, count;

				count = NEMOSHOW_ITEM_CC(item, stroke)->getTextWidths(item->text, strlen(item->text), widths, NULL);

				for (i = 0; i < count; i++) {
					width += ceil(widths[i]);
				}

				box = SkRect::MakeXYWH(0, 0, width, item->fontdescent - item->fontascent);
			} else if (NEMOSHOW_FONT_AT(NEMOSHOW_REF(one, NEMOSHOW_FONT_REF), layout) == NEMOSHOW_HARFBUZZ_LAYOUT) {
				box = SkRect::MakeXYWH(item->x, item->y, item->textwidth, item->textheight);
			}
		} else {
			box = NEMOSHOW_ITEM_ATCC(NEMOSHOW_REF(one, NEMOSHOW_PATH_REF), path)->getBounds();

			box.outset(item->fontsize, item->fontsize);
		}

		item->x = box.x();
		item->y = box.y();
		item->width = box.width();
		item->height = box.height();

		one->dirty |= NEMOSHOW_SIZE_DIRTY;
	} else if (one->sub == NEMOSHOW_TEXTBOX_ITEM) {
		NEMOSHOW_ITEM_CC(item, textbox)->setBox(item->x, item->y, item->width, item->height);
	}

	one->dirty |= NEMOSHOW_BOUNDS_DIRTY;
}

static inline void nemoshow_item_update_matrix(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	NEMOSHOW_ITEM_CC(item, matrix)->setIdentity();

	if (nemoshow_one_has_state(one, NEMOSHOW_TRANSFORM_STATE)) {
		if (item->transform == 0)
			item->transform = NEMOSHOW_TSR_TRANSFORM;

		NEMOSHOW_ITEM_CC(item, modelview)->setIdentity();

		if (nemoshow_one_has_state(one, NEMOSHOW_VIEWPORT_STATE))
			NEMOSHOW_ITEM_CC(item, modelview)->postScale(item->width / item->width0, item->height / item->height0);

		if (nemoshow_one_has_state(one, NEMOSHOW_ANCHOR_STATE))
			NEMOSHOW_ITEM_CC(item, modelview)->postTranslate(-item->width * item->ax, -item->height * item->ay);

		if (item->transform == NEMOSHOW_TSR_TRANSFORM) {
			if (item->px != 0.0f || item->py != 0.0f) {
				NEMOSHOW_ITEM_CC(item, modelview)->postTranslate(-item->px, -item->py);

				if (item->ro != 0.0f) {
					NEMOSHOW_ITEM_CC(item, modelview)->postRotate(item->ro);
				}
				if (item->sx != 1.0f || item->sy != 1.0f) {
					NEMOSHOW_ITEM_CC(item, modelview)->postScale(item->sx, item->sy);
				}

				NEMOSHOW_ITEM_CC(item, modelview)->postTranslate(item->px, item->py);
			} else {
				if (item->ro != 0.0f) {
					NEMOSHOW_ITEM_CC(item, modelview)->postRotate(item->ro);
				}
				if (item->sx != 1.0f || item->sy != 1.0f) {
					NEMOSHOW_ITEM_CC(item, modelview)->postScale(item->sx, item->sy);
				}
			}

			NEMOSHOW_ITEM_CC(item, modelview)->postTranslate(item->tx, item->ty);
		} else if (item->transform == NEMOSHOW_EXTERN_TRANSFORM) {
			NEMOSHOW_ITEM_CC(item, modelview)->postConcat(
					*NEMOSHOW_MATRIX_CC(
						NEMOSHOW_MATRIX(
							NEMOSHOW_REF(one, NEMOSHOW_MATRIX_REF)),
						matrix));
		} else if (item->transform == NEMOSHOW_CHILDREN_TRANSFORM) {
			struct showone *child;

			nemoshow_children_for_each(child, one) {
				if (child->type == NEMOSHOW_MATRIX_TYPE) {
					NEMOSHOW_ITEM_CC(item, modelview)->postConcat(
							*NEMOSHOW_MATRIX_CC(
								NEMOSHOW_MATRIX(child),
								matrix));
				}
			}
		} else if (item->transform == NEMOSHOW_DIRECT_TRANSFORM) {
			SkScalar args[9] = {
				SkDoubleToScalar(item->matrix[0]),
				SkDoubleToScalar(item->matrix[1]),
				SkDoubleToScalar(item->matrix[2]),
				SkDoubleToScalar(item->matrix[3]),
				SkDoubleToScalar(item->matrix[4]),
				SkDoubleToScalar(item->matrix[5]),
				SkDoubleToScalar(item->matrix[6]),
				SkDoubleToScalar(item->matrix[7]),
				SkDoubleToScalar(item->matrix[8])
			};

			NEMOSHOW_ITEM_CC(item, modelview)->set9(args);
		}

		NEMOSHOW_ITEM_CC(item, matrix)->postConcat(
				*NEMOSHOW_ITEM_CC(item, modelview));
	}

	if (one->parent != NULL && nemoshow_one_has_state(one->parent, NEMOSHOW_INHERIT_STATE)) {
		struct showitem *group = NEMOSHOW_ITEM(one->parent);

		NEMOSHOW_ITEM_CC(item, matrix)->postConcat(
				*NEMOSHOW_ITEM_CC(group, matrix));
	}

	NEMOSHOW_ITEM_CC(item, has_inverse) =
		NEMOSHOW_ITEM_CC(item, matrix)->invert(
				NEMOSHOW_ITEM_CC(item, inverse));

	one->dirty |= NEMOSHOW_BOUNDS_DIRTY;
}

static inline void nemoshow_item_update_bounds(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one->canvas);
	SkRect box = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);
	double outer = NEMOSHOW_ANTIALIAS_EPSILON;

	if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE))
		box.outset(item->stroke_width, item->stroke_width);

	one->x0 = box.x();
	one->y0 = box.y();
	one->x1 = box.x() + box.width();
	one->y1 = box.y() + box.height();

	NEMOSHOW_ITEM_CC(item, matrix)->mapRect(&box);

	if (NEMOSHOW_REF(one, NEMOSHOW_FILTER_REF) != NULL)
		outer += NEMOSHOW_FILTER_AT(NEMOSHOW_REF(one, NEMOSHOW_FILTER_REF), r) * 2.0f;
	box.outset(outer, outer);

	one->x = MAX(floor(box.x()), 0);
	one->y = MAX(floor(box.y()), 0);
	one->w = ceil(box.width());
	one->h = ceil(box.height());
	one->sx = floor(one->x * canvas->viewport.sx);
	one->sy = floor(one->y * canvas->viewport.sy);
	one->sw = ceil(one->w * canvas->viewport.sx);
	one->sh = ceil(one->h * canvas->viewport.sy);
}

int nemoshow_item_update(struct showone *one)
{
	struct nemoshow *show = one->show;
	struct showitem *item = NEMOSHOW_ITEM(one);

	if ((one->dirty & NEMOSHOW_URI_DIRTY) != 0)
		nemoshow_item_update_uri(show, one);
	if ((one->dirty & NEMOSHOW_STYLE_DIRTY) != 0)
		nemoshow_item_update_style(show, one);
	if ((one->dirty & NEMOSHOW_FILTER_DIRTY) != 0)
		nemoshow_item_update_filter(show, one);
	if ((one->dirty & NEMOSHOW_SHADER_DIRTY) != 0)
		nemoshow_item_update_shader(show, one);
	if ((one->dirty & NEMOSHOW_FONT_DIRTY) != 0)
		nemoshow_item_update_font(show, one);
	if ((one->dirty & NEMOSHOW_TEXT_DIRTY) != 0)
		nemoshow_item_update_text(show, one);
	if ((one->dirty & NEMOSHOW_POINTS_DIRTY) != 0)
		nemoshow_item_update_points(show, one);
	if ((one->dirty & NEMOSHOW_PATH_DIRTY) != 0)
		nemoshow_item_update_path(show, one);
	if ((one->dirty & NEMOSHOW_PATHEFFECT_DIRTY) != 0)
		nemoshow_item_update_patheffect(show, one);
	if ((one->dirty & NEMOSHOW_SHAPE_DIRTY) != 0)
		nemoshow_item_update_shape(show, one);
	if ((one->dirty & NEMOSHOW_MATRIX_DIRTY) != 0)
		nemoshow_item_update_matrix(show, one);
	else if ((one->dirty & NEMOSHOW_SIZE_DIRTY) != 0 && nemoshow_one_has_state(one, NEMOSHOW_VIEWPORT_STATE | NEMOSHOW_ANCHOR_STATE))
		nemoshow_item_update_matrix(show, one);

	if (one->canvas != NULL) {
		if ((one->dirty & NEMOSHOW_BOUNDS_DIRTY) != 0) {
			nemoshow_canvas_damage_one(one->canvas, one);

			nemoshow_item_update_bounds(show, one);

			nemoshow_one_bounds(one);
		}

		nemoshow_canvas_damage_one(one->canvas, one);
	}

	return 0;
}

void nemoshow_item_set_stroke_cap(struct showone *one, int cap)
{
	const SkPaint::Cap caps[] = {
		SkPaint::kButt_Cap,
		SkPaint::kRound_Cap,
		SkPaint::kSquare_Cap
	};
	struct showitem *item = NEMOSHOW_ITEM(one);

	NEMOSHOW_ITEM_CC(item, stroke)->setStrokeCap(caps[cap]);
	NEMOSHOW_ITEM_CC(item, fill)->setStrokeCap(caps[cap]);
}

void nemoshow_item_set_stroke_join(struct showone *one, int join)
{
	const SkPaint::Join joins[] = {
		SkPaint::kMiter_Join,
		SkPaint::kRound_Join,
		SkPaint::kBevel_Join
	};
	struct showitem *item = NEMOSHOW_ITEM(one);

	NEMOSHOW_ITEM_CC(item, stroke)->setStrokeJoin(joins[join]);
	NEMOSHOW_ITEM_CC(item, fill)->setStrokeJoin(joins[join]);
}

void nemoshow_item_set_matrix(struct showone *one, double m[9])
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	int i;

	for (i = 0; i < 9; i++)
		item->matrix[i] = m[i];

	item->transform = NEMOSHOW_DIRECT_TRANSFORM;

	nemoshow_one_set_state(one, NEMOSHOW_TRANSFORM_STATE);

	nemoshow_one_dirty(one, NEMOSHOW_MATRIX_DIRTY);
}

void nemoshow_item_set_shader(struct showone *one, struct showone *shader)
{
	nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_SHADER_REF));

	if (shader != NULL)
		nemoshow_one_reference_one(one, shader, NEMOSHOW_SHADER_DIRTY, 0x0, NEMOSHOW_SHADER_REF);
}

void nemoshow_item_set_filter(struct showone *one, struct showone *filter)
{
	nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_FILTER_REF));

	if (filter != NULL)
		nemoshow_one_reference_one(one, filter, NEMOSHOW_FILTER_DIRTY, 0x0, NEMOSHOW_FILTER_REF);
}

void nemoshow_item_set_clip(struct showone *one, struct showone *clip)
{
	nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_CLIP_REF));
	nemoshow_one_reference_one(one, clip, NEMOSHOW_REDRAW_DIRTY, NEMOSHOW_CLIP_STATE, NEMOSHOW_CLIP_REF);
}

void nemoshow_item_set_font(struct showone *one, struct showone *font)
{
	nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_FONT_REF));
	nemoshow_one_reference_one(one, font, NEMOSHOW_FONT_DIRTY, 0x0, NEMOSHOW_FONT_REF);
}

void nemoshow_item_set_path(struct showone *one, struct showone *path)
{
	nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_PATH_REF));
	nemoshow_one_reference_one(one, path, NEMOSHOW_SHAPE_DIRTY, 0x0, NEMOSHOW_PATH_REF);
}

void nemoshow_item_set_uri(struct showone *one, const char *uri)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (item->uri != NULL)
		free(item->uri);

	item->uri = strdup(uri);

	nemoshow_one_dirty(one, NEMOSHOW_URI_DIRTY);
}

void nemoshow_item_set_text(struct showone *one, const char *text)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (item->text != NULL)
		free(item->text);

	item->text = strdup(text);

	nemoshow_one_dirty(one, NEMOSHOW_TEXT_DIRTY);
}

void nemoshow_item_attach_one(struct showone *parent, struct showone *one)
{
	struct showone *canvas;

	nemoshow_one_attach_one(parent, one);

	canvas = nemoshow_one_get_parent(one, NEMOSHOW_CANVAS_TYPE, 0);
	if (canvas != NULL)
		nemoshow_canvas_set_ones(canvas, one);
}

void nemoshow_item_detach_one(struct showone *one)
{
	nemoshow_one_detach_one(one);

	nemoshow_canvas_put_ones(one);
}

int nemoshow_item_above_one(struct showone *one, struct showone *above)
{
	if (above != NULL && one->parent != above->parent)
		return -1;

	nemoshow_one_above_one(one, above);

	nemoshow_one_dirty(one, NEMOSHOW_REDRAW_DIRTY);

	return 0;
}

int nemoshow_item_below_one(struct showone *one, struct showone *below)
{
	if (below != NULL && one->parent != below->parent)
		return -1;

	nemoshow_one_below_one(one, below);

	nemoshow_one_dirty(one, NEMOSHOW_REDRAW_DIRTY);

	return 0;
}

void nemoshow_item_path_clear(struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (one->sub == NEMOSHOW_PATH_ITEM) {
		NEMOSHOW_ITEM_CC(item, path)->reset();

		nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
	} else if (one->sub == NEMOSHOW_PATHTWICE_ITEM) {
		NEMOSHOW_ITEM_CC(item, path)->reset();
		NEMOSHOW_ITEM_CC(item, fillpath)->reset();

		nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
	} else if (one->sub == NEMOSHOW_PATHARRAY_ITEM) {
		item->ncmds = 0;
		item->npoints = 0;

		nemoshow_one_dirty(one, NEMOSHOW_POINTS_DIRTY | NEMOSHOW_PATH_DIRTY);
	} else if (one->sub == NEMOSHOW_PATHLIST_ITEM) {
		struct showone *child, *nchild;

		nemoshow_children_for_each_safe(child, nchild, one) {
			if (child->type == NEMOSHOW_PATHCMD_TYPE)
				nemoshow_one_destroy(child);
		}

		nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
	}
}

int nemoshow_item_path_moveto(struct showone *one, double x, double y)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (one->sub == NEMOSHOW_PATHARRAY_ITEM) {
		NEMOSHOW_ARRAY_APPEND(item->cmds, item->scmds, item->ncmds, NEMOSHOW_PATH_MOVETO_CMD);

		NEMOSHOW_ARRAY_APPEND(item->points, item->spoints, item->npoints, x);
		NEMOSHOW_ARRAY_APPEND(item->points, item->spoints, item->npoints, y);
		NEMOSHOW_ARRAY_APPEND(item->points, item->spoints, item->npoints, 0.0f);
		NEMOSHOW_ARRAY_APPEND(item->points, item->spoints, item->npoints, 0.0f);
		NEMOSHOW_ARRAY_APPEND(item->points, item->spoints, item->npoints, 0.0f);
		NEMOSHOW_ARRAY_APPEND(item->points, item->spoints, item->npoints, 0.0f);

		nemoobject_set_reserved(&one->object, "points", item->points, sizeof(double) * item->npoints);

		nemoshow_one_dirty(one, NEMOSHOW_POINTS_DIRTY | NEMOSHOW_PATH_DIRTY);

		return item->ncmds - 1;
	} else if (one->sub == NEMOSHOW_PATHLIST_ITEM) {
		struct showone *child;

		child = nemoshow_pathcmd_create(NEMOSHOW_PATH_MOVETO_CMD);
		nemoshow_one_attach(one, child);

		nemoshow_pathcmd_set_x0(child, x);
		nemoshow_pathcmd_set_y0(child, y);

		nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);

		return nemoshow_children_count(one) - 1;
	} else if (one->sub == NEMOSHOW_PATHTWICE_ITEM) {
		if (item->pathselect & NEMOSHOW_ITEM_STROKE_PATH)
			NEMOSHOW_ITEM_CC(item, path)->moveTo(x, y);
		if (item->pathselect & NEMOSHOW_ITEM_FILL_PATH)
			NEMOSHOW_ITEM_CC(item, fillpath)->moveTo(x, y);
	} else {
		NEMOSHOW_ITEM_CC(item, path)->moveTo(x, y);
	}

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);

	return 0;
}

int nemoshow_item_path_lineto(struct showone *one, double x, double y)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (one->sub == NEMOSHOW_PATHARRAY_ITEM) {
		NEMOSHOW_ARRAY_APPEND(item->cmds, item->scmds, item->ncmds, NEMOSHOW_PATH_LINETO_CMD);

		NEMOSHOW_ARRAY_APPEND(item->points, item->spoints, item->npoints, x);
		NEMOSHOW_ARRAY_APPEND(item->points, item->spoints, item->npoints, y);
		NEMOSHOW_ARRAY_APPEND(item->points, item->spoints, item->npoints, 0.0f);
		NEMOSHOW_ARRAY_APPEND(item->points, item->spoints, item->npoints, 0.0f);
		NEMOSHOW_ARRAY_APPEND(item->points, item->spoints, item->npoints, 0.0f);
		NEMOSHOW_ARRAY_APPEND(item->points, item->spoints, item->npoints, 0.0f);

		nemoobject_set_reserved(&one->object, "points", item->points, sizeof(double) * item->npoints);

		nemoshow_one_dirty(one, NEMOSHOW_POINTS_DIRTY | NEMOSHOW_PATH_DIRTY);

		return item->ncmds - 1;
	} else if (one->sub == NEMOSHOW_PATHLIST_ITEM) {
		struct showone *child;

		child = nemoshow_pathcmd_create(NEMOSHOW_PATH_LINETO_CMD);
		nemoshow_one_attach(one, child);

		nemoshow_pathcmd_set_x0(child, x);
		nemoshow_pathcmd_set_y0(child, y);

		nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);

		return nemoshow_children_count(one) - 1;
	} else if (one->sub == NEMOSHOW_PATHTWICE_ITEM) {
		if (item->pathselect & NEMOSHOW_ITEM_STROKE_PATH)
			NEMOSHOW_ITEM_CC(item, path)->lineTo(x, y);
		if (item->pathselect & NEMOSHOW_ITEM_FILL_PATH)
			NEMOSHOW_ITEM_CC(item, fillpath)->lineTo(x, y);
	} else {
		NEMOSHOW_ITEM_CC(item, path)->lineTo(x, y);
	}

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);

	return 0;
}

int nemoshow_item_path_cubicto(struct showone *one, double x0, double y0, double x1, double y1, double x2, double y2)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (one->sub == NEMOSHOW_PATHARRAY_ITEM) {
		NEMOSHOW_ARRAY_APPEND(item->cmds, item->scmds, item->ncmds, NEMOSHOW_PATH_CURVETO_CMD);

		NEMOSHOW_ARRAY_APPEND(item->points, item->spoints, item->npoints, x0);
		NEMOSHOW_ARRAY_APPEND(item->points, item->spoints, item->npoints, y0);
		NEMOSHOW_ARRAY_APPEND(item->points, item->spoints, item->npoints, x1);
		NEMOSHOW_ARRAY_APPEND(item->points, item->spoints, item->npoints, y1);
		NEMOSHOW_ARRAY_APPEND(item->points, item->spoints, item->npoints, x2);
		NEMOSHOW_ARRAY_APPEND(item->points, item->spoints, item->npoints, y2);

		nemoobject_set_reserved(&one->object, "points", item->points, sizeof(double) * item->npoints);

		nemoshow_one_dirty(one, NEMOSHOW_POINTS_DIRTY | NEMOSHOW_PATH_DIRTY);

		return item->ncmds - 1;
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
	} else if (one->sub == NEMOSHOW_PATHTWICE_ITEM) {
		if (item->pathselect & NEMOSHOW_ITEM_STROKE_PATH)
			NEMOSHOW_ITEM_CC(item, path)->cubicTo(x0, y0, x1, y1, x2, y2);
		if (item->pathselect & NEMOSHOW_ITEM_FILL_PATH)
			NEMOSHOW_ITEM_CC(item, fillpath)->cubicTo(x0, y0, x1, y1, x2, y2);
	} else {
		NEMOSHOW_ITEM_CC(item, path)->cubicTo(x0, y0, x1, y1, x2, y2);
	}

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);

	return 0;
}

void nemoshow_item_path_arcto(struct showone *one, double x, double y, double width, double height, double from, double to, int needs_moveto)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	SkRect rect = SkRect::MakeXYWH(x, y, width, height);

	if (one->sub == NEMOSHOW_PATHTWICE_ITEM) {
		if (item->pathselect & NEMOSHOW_ITEM_STROKE_PATH)
			NEMOSHOW_ITEM_CC(item, path)->arcTo(rect, from, to, needs_moveto == 0 ? false : true);
		if (item->pathselect & NEMOSHOW_ITEM_FILL_PATH)
			NEMOSHOW_ITEM_CC(item, fillpath)->arcTo(rect, from, to, needs_moveto == 0 ? false : true);
	} else {
		NEMOSHOW_ITEM_CC(item, path)->arcTo(rect, from, to, needs_moveto == 0 ? false : true);
	}

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

int nemoshow_item_path_close(struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (one->sub == NEMOSHOW_PATHARRAY_ITEM) {
		NEMOSHOW_ARRAY_APPEND(item->cmds, item->scmds, item->ncmds, NEMOSHOW_PATH_CLOSE_CMD);

		NEMOSHOW_ARRAY_APPEND(item->points, item->spoints, item->npoints, 0.0f);
		NEMOSHOW_ARRAY_APPEND(item->points, item->spoints, item->npoints, 0.0f);
		NEMOSHOW_ARRAY_APPEND(item->points, item->spoints, item->npoints, 0.0f);
		NEMOSHOW_ARRAY_APPEND(item->points, item->spoints, item->npoints, 0.0f);
		NEMOSHOW_ARRAY_APPEND(item->points, item->spoints, item->npoints, 0.0f);
		NEMOSHOW_ARRAY_APPEND(item->points, item->spoints, item->npoints, 0.0f);

		nemoobject_set_reserved(&one->object, "points", item->points, sizeof(double) * item->npoints);

		nemoshow_one_dirty(one, NEMOSHOW_POINTS_DIRTY | NEMOSHOW_PATH_DIRTY);

		return item->ncmds - 1;
	} else if (one->sub == NEMOSHOW_PATHLIST_ITEM) {
		struct showone *child;

		child = nemoshow_pathcmd_create(NEMOSHOW_PATH_CLOSE_CMD);
		nemoshow_one_attach(one, child);

		nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);

		return nemoshow_children_count(one) - 1;
	} else if (one->sub == NEMOSHOW_PATHTWICE_ITEM) {
		if (item->pathselect & NEMOSHOW_ITEM_STROKE_PATH)
			NEMOSHOW_ITEM_CC(item, path)->close();
		if (item->pathselect & NEMOSHOW_ITEM_FILL_PATH)
			NEMOSHOW_ITEM_CC(item, fillpath)->close();
	} else {
		NEMOSHOW_ITEM_CC(item, path)->close();
	}

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);

	return 0;
}

void nemoshow_item_path_cmd(struct showone *one, const char *cmd)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	SkPath path;

	SkParsePath::FromSVGString(cmd, &path);

	if (one->sub == NEMOSHOW_PATHTWICE_ITEM) {
		if (item->pathselect & NEMOSHOW_ITEM_STROKE_PATH)
			NEMOSHOW_ITEM_CC(item, path)->addPath(path);
		if (item->pathselect & NEMOSHOW_ITEM_FILL_PATH)
			NEMOSHOW_ITEM_CC(item, fillpath)->addPath(path);
	} else {
		NEMOSHOW_ITEM_CC(item, path)->addPath(path);
	}

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

void nemoshow_item_path_arc(struct showone *one, double x, double y, double width, double height, double from, double to)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	SkRect rect = SkRect::MakeXYWH(x, y, width, height);

	if (one->sub == NEMOSHOW_PATHTWICE_ITEM) {
		if (item->pathselect & NEMOSHOW_ITEM_STROKE_PATH)
			NEMOSHOW_ITEM_CC(item, path)->addArc(rect, from, to);
		if (item->pathselect & NEMOSHOW_ITEM_FILL_PATH)
			NEMOSHOW_ITEM_CC(item, fillpath)->addArc(rect, from, to);
	} else {
		NEMOSHOW_ITEM_CC(item, path)->addArc(rect, from, to);
	}

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

void nemoshow_item_path_text(struct showone *one, const char *font, int fontsize, const char *text, int textlength, double x, double y)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	SkPaint paint;
	SkPath path;
	sk_sp<SkTypeface> face;
	SkPaint::FontMetrics metrics;

	face = SkTypeface::MakeFromFile(
			fontconfig_get_path(
				font,
				NULL,
				FC_SLANT_ROMAN,
				FC_WEIGHT_NORMAL,
				FC_WIDTH_NORMAL,
				FC_MONO),
			0);

	paint.setTypeface(face);
	paint.setAntiAlias(true);
	paint.setTextSize(fontsize);
	paint.getFontMetrics(&metrics, 0);
	paint.getTextPath(text, textlength, x, y - metrics.fAscent, &path);

	if (one->sub == NEMOSHOW_PATHTWICE_ITEM) {
		if (item->pathselect & NEMOSHOW_ITEM_STROKE_PATH)
			NEMOSHOW_ITEM_CC(item, path)->addPath(path);
		if (item->pathselect & NEMOSHOW_ITEM_FILL_PATH)
			NEMOSHOW_ITEM_CC(item, fillpath)->addPath(path);
	} else {
		NEMOSHOW_ITEM_CC(item, path)->addPath(path);
	}

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

void nemoshow_item_path_append(struct showone *one, struct showone *src)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showitem *other = NEMOSHOW_ITEM(src);

	if (one->sub == NEMOSHOW_PATHTWICE_ITEM) {
		if (src->sub == NEMOSHOW_PATHGROUP_ITEM) {
			struct showone *child;

			if (item->pathselect & NEMOSHOW_ITEM_STROKE_PATH) {
				nemoshow_children_for_each(child, src) {
					NEMOSHOW_ITEM_CC(item, path)->addPath(*NEMOSHOW_PATH_ATCC(child, path));
				}
			}
			if (item->pathselect & NEMOSHOW_ITEM_FILL_PATH) {
				nemoshow_children_for_each(child, src) {
					NEMOSHOW_ITEM_CC(item, fillpath)->addPath(*NEMOSHOW_PATH_ATCC(child, path));
				}
			}
		} else if (src->sub == NEMOSHOW_PATHTWICE_ITEM) {
			NEMOSHOW_ITEM_CC(item, path)->addPath(*NEMOSHOW_ITEM_CC(other, path));
			NEMOSHOW_ITEM_CC(item, fillpath)->addPath(*NEMOSHOW_ITEM_CC(other, fillpath));
		} else if (src->sub == NEMOSHOW_PATH_ITEM) {
			if (item->pathselect & NEMOSHOW_ITEM_STROKE_PATH)
				NEMOSHOW_ITEM_CC(item, path)->addPath(*NEMOSHOW_ITEM_CC(other, path));
			if (item->pathselect & NEMOSHOW_ITEM_FILL_PATH)
				NEMOSHOW_ITEM_CC(item, fillpath)->addPath(*NEMOSHOW_ITEM_CC(other, path));
		}
	} else if (one->sub == NEMOSHOW_PATH_ITEM) {
		if (src->sub == NEMOSHOW_PATHGROUP_ITEM) {
			struct showone *child;

			nemoshow_children_for_each(child, src) {
				NEMOSHOW_ITEM_CC(item, path)->addPath(*NEMOSHOW_PATH_ATCC(child, path));
			}
		} else if (src->sub == NEMOSHOW_PATHTWICE_ITEM) {
			NEMOSHOW_ITEM_CC(item, path)->addPath(*NEMOSHOW_ITEM_CC(other, path));
			NEMOSHOW_ITEM_CC(item, path)->addPath(*NEMOSHOW_ITEM_CC(other, fillpath));
		} else if (src->sub == NEMOSHOW_PATH_ITEM) {
			NEMOSHOW_ITEM_CC(item, path)->addPath(*NEMOSHOW_ITEM_CC(other, path));
		}
	}

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

static inline void nemoshow_item_path_load_style(struct showone *one, struct xmlnode *node)
{
	const char *attr;
	uint32_t c;
	double w;

	attr = nemoxml_node_get_attr(node, "fill");
	if (attr != NULL && strcmp(attr, "none") != 0) {
		c = color_parse(attr);

		nemoshow_path_set_fill_color(one,
				COLOR_UINT32_R(c),
				COLOR_UINT32_G(c),
				COLOR_UINT32_B(c),
				COLOR_UINT32_A(c));
	}

	attr = nemoxml_node_get_attr(node, "stroke");
	if (attr != NULL && strcmp(attr, "none") != 0) {
		c = color_parse(attr);

		attr = nemoxml_node_get_attr(node, "stroke-width");
		if (attr != NULL)
			w = strtod(attr, NULL);
		else
			w = 1.0f;

		nemoshow_path_set_stroke_color(one,
				COLOR_UINT32_R(c),
				COLOR_UINT32_G(c),
				COLOR_UINT32_B(c),
				COLOR_UINT32_A(c));
		nemoshow_path_set_stroke_width(one, w);
	}
}

int nemoshow_item_path_load_svg(struct showone *one, const char *uri, double x, double y, double width, double height)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showone *child;
	struct nemoxml *xml;
	struct xmlnode *node;
	double sw, sh;
	double pw, ph;
	const char *attr0, *attr1;
	const char *units;
	int has_fill;
	int has_stroke;
	SkPath path;
	SkMatrix matrix;

	if (uri == NULL)
		return -1;

	xml = nemoxml_create();
	nemoxml_load_file(xml, uri);
	nemoxml_update(xml);

	nemolist_for_each(node, &xml->nodes, nodelink) {
		has_stroke = 0;
		has_fill = 0;

		if ((attr0 = nemoxml_node_get_attr(node, "display")) != NULL && strcmp(attr0, "none") == 0)
			continue;

		if ((attr0 = nemoxml_node_get_attr(node, "stroke")) != NULL && strcmp(attr0, "none") != 0)
			has_stroke = 1;

		if ((attr0 = nemoxml_node_get_attr(node, "fill")) != NULL && strcmp(attr0, "none") != 0)
			has_fill = 1;

		if ((attr0 = nemoxml_node_get_attr(node, "style")) != NULL) {
			if (strstr(attr0, "fill") != NULL)
				has_fill = 1;
			if (strstr(attr0, "stroke") != NULL)
				has_stroke = 1;
		}

		if (has_stroke == 0 && has_fill == 0)
			has_stroke = 1;

		path.reset();

		if (strcmp(node->name, "path") == 0) {
			const char *d;

			d = nemoxml_node_get_attr(node, "d");
			if (d != NULL) {
				SkParsePath::FromSVGString(d, &path);
			}
		} else if (strcmp(node->name, "line") == 0) {
			double x1 = nemoxml_node_get_dattr(node, "x1", 0.0f);
			double y1 = nemoxml_node_get_dattr(node, "y1", 0.0f);
			double x2 = nemoxml_node_get_dattr(node, "x2", 0.0f);
			double y2 = nemoxml_node_get_dattr(node, "y2", 0.0f);

			path.moveTo(x1, y1);
			path.lineTo(x2, y2);
		} else if (strcmp(node->name, "rect") == 0) {
			double x = nemoxml_node_get_dattr(node, "x", 0.0f);
			double y = nemoxml_node_get_dattr(node, "y", 0.0f);
			double width = nemoxml_node_get_dattr(node, "width", 0.0f);
			double height = nemoxml_node_get_dattr(node, "height", 0.0f);

			path.addRect(x, y, x + width, y + height);
		} else if (strcmp(node->name, "circle") == 0) {
			double x = nemoxml_node_get_dattr(node, "cx", 0.0f);
			double y = nemoxml_node_get_dattr(node, "cy", 0.0f);
			double r = nemoxml_node_get_dattr(node, "r", 0.0f);

			path.addCircle(x, y, r);
		} else if (strcmp(node->name, "ellipse") == 0) {
			double x = nemoxml_node_get_dattr(node, "cx", 0.0f);
			double y = nemoxml_node_get_dattr(node, "cy", 0.0f);
			double rx = nemoxml_node_get_dattr(node, "rx", 0.0f);
			double ry = nemoxml_node_get_dattr(node, "ry", 0.0f);
			SkRect oval = SkRect::MakeXYWH(x - rx, y - ry, rx * 2, ry * 2);

			path.addArc(oval, 0, 360);
		} else if (strcmp(node->name, "polygon") == 0) {
			const char *points = nemoxml_node_get_attr(node, "points");
			struct nemotoken *token;
			int i, count;

			token = nemotoken_create(points, strlen(points));
			nemotoken_divide(token, ' ');
			nemotoken_divide(token, '\t');
			nemotoken_divide(token, ',');
			nemotoken_update(token);

			count = nemotoken_get_token_count(token);

			path.moveTo(
					nemotoken_get_double(token, 0, 0.0f),
					nemotoken_get_double(token, 1, 0.0f));

			for (i = 2; i < count; i += 2) {
				path.lineTo(
						nemotoken_get_double(token, i + 0, 0.0f),
						nemotoken_get_double(token, i + 1, 0.0f));
			}

			path.close();

			nemotoken_destroy(token);
		} else if (strcmp(node->name, "polyline") == 0) {
			const char *points = nemoxml_node_get_attr(node, "points");
			struct nemotoken *token;
			int i, count;

			token = nemotoken_create(points, strlen(points));
			nemotoken_divide(token, ' ');
			nemotoken_divide(token, '\t');
			nemotoken_divide(token, ',');
			nemotoken_update(token);

			count = nemotoken_get_token_count(token);

			path.moveTo(
					nemotoken_get_double(token, 0, 0.0f),
					nemotoken_get_double(token, 1, 0.0f));

			for (i = 2; i < count; i += 2) {
				path.lineTo(
						nemotoken_get_double(token, i + 0, 0.0f),
						nemotoken_get_double(token, i + 1, 0.0f));
			}

			nemotoken_destroy(token);
		}

		if ((attr0 = nemoxml_node_get_attr(node, "transform")) != NULL) {
			nemoshow_svg_get_transform(&matrix, attr0);

			path.transform(matrix);
		}

		if (one->sub == NEMOSHOW_PATHGROUP_ITEM) {
			child = nemoshow_path_create(NEMOSHOW_NORMAL_PATH);
			nemoshow_one_attach(one, child);

			NEMOSHOW_PATH_ATCC(child, path)->addPath(path);

			nemoshow_item_path_load_style(child, node);
		} else if (one->sub == NEMOSHOW_PATHTWICE_ITEM) {
			if (has_fill != 0)
				NEMOSHOW_ITEM_CC(item, fillpath)->addPath(path);
			if (has_stroke != 0)
				NEMOSHOW_ITEM_CC(item, path)->addPath(path);
		} else {
			NEMOSHOW_ITEM_CC(item, path)->addPath(path);
		}
	}

	matrix.setIdentity();

	nemolist_for_each(node, &xml->nodes, nodelink) {
		if (strcmp(node->name, "svg") == 0) {
			attr0 = nemoxml_node_get_attr(node, "width");
			attr1 = nemoxml_node_get_attr(node, "height");

			if (attr0 != NULL && attr1 != NULL) {
				sw = width;
				sh = height;

				pw = string_parse_float_with_endptr(attr0, 0, strlen(attr0), &units);
				ph = string_parse_float_with_endptr(attr1, 0, strlen(attr1), &units);

				if (sw != pw || sh != ph) {
					matrix.postScale(sw / pw, sh / ph);
				}
			}

			break;
		}
	}

	matrix.postTranslate(x, y);

	if (one->sub == NEMOSHOW_PATHGROUP_ITEM) {
		nemoshow_children_for_each(child, one) {
			NEMOSHOW_PATH_ATCC(child, path)->transform(matrix);
		}
	} else if (one->sub == NEMOSHOW_PATHTWICE_ITEM) {
		NEMOSHOW_ITEM_CC(item, path)->transform(matrix);
		NEMOSHOW_ITEM_CC(item, fillpath)->transform(matrix);
	} else {
		NEMOSHOW_ITEM_CC(item, path)->transform(matrix);
	}

	nemoxml_destroy(xml);

	return 0;
}

void nemoshow_item_path_use(struct showone *one, uint32_t paths)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->pathselect = paths;
}

void nemoshow_item_path_translate(struct showone *one, double x, double y)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showone *child;
	SkMatrix matrix;

	matrix.setIdentity();
	matrix.postTranslate(x, y);

	if (one->sub == NEMOSHOW_PATHTWICE_ITEM) {
		NEMOSHOW_ITEM_CC(item, path)->transform(matrix);
		NEMOSHOW_ITEM_CC(item, fillpath)->transform(matrix);
	} else {
		NEMOSHOW_ITEM_CC(item, path)->transform(matrix);
	}

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

void nemoshow_item_path_scale(struct showone *one, double sx, double sy)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showone *child;
	SkMatrix matrix;

	matrix.setIdentity();
	matrix.postScale(sx, sy);

	if (one->sub == NEMOSHOW_PATHTWICE_ITEM) {
		NEMOSHOW_ITEM_CC(item, path)->transform(matrix);
		NEMOSHOW_ITEM_CC(item, fillpath)->transform(matrix);
	} else {
		NEMOSHOW_ITEM_CC(item, path)->transform(matrix);
	}

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

void nemoshow_item_path_rotate(struct showone *one, double ro)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showone *child;
	SkMatrix matrix;

	matrix.setIdentity();
	matrix.postRotate(ro);

	if (one->sub == NEMOSHOW_PATHTWICE_ITEM) {
		NEMOSHOW_ITEM_CC(item, path)->transform(matrix);
		NEMOSHOW_ITEM_CC(item, fillpath)->transform(matrix);
	} else {
		NEMOSHOW_ITEM_CC(item, path)->transform(matrix);
	}

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

void nemoshow_item_path_set_discrete_effect(struct showone *one, double segment, double deviation, uint32_t seed)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->pathsegment = segment;
	item->pathdeviation = deviation;
	item->pathseed = seed;

	nemoshow_one_dirty(one, NEMOSHOW_PATHEFFECT_DIRTY);
}

void nemoshow_item_path_set_dash_effect(struct showone *one, double *dashes, int dashcount)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	int i;

	if (item->pathdashes != NULL)
		free(item->pathdashes);

	item->pathdashes = (double *)malloc(sizeof(double) * dashcount);
	item->pathdashcount = dashcount;

	for (i = 0; i < dashcount; i++)
		item->pathdashes[i] = dashes[i];

	nemoobject_set_reserved(&one->object, "pathdashes", item->pathdashes, sizeof(double) * dashcount);

	nemoshow_one_dirty(one, NEMOSHOW_PATHEFFECT_DIRTY);
}

int nemoshow_item_path_get_position(struct showone *one, double t, double *px, double *py, double *tx, double *ty)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	SkPoint point;
	SkVector tangent;
	bool r;

	r = NEMOSHOW_ITEM_CC(item, measure)->getPosTan(
			NEMOSHOW_ITEM_CC(item, measure)->getLength() * t,
			&point,
			&tangent);
	if (r == false)
		return -1;

	if (px != NULL && py != NULL) {
		NEMOSHOW_ITEM_CC(item, matrix)->mapPoints(&point, 1);

		*px = point.x();
		*py = point.y();
	}

	if (tx != NULL && ty != NULL) {
		NEMOSHOW_ITEM_CC(item, matrix)->mapVectors(&tangent, 1);

		*tx = tangent.x();
		*ty = tangent.y();
	}

	return 0;
}

int nemoshow_item_path_contain_point(struct showone *one, double x, double y)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (NEMOSHOW_ITEM_CC(item, has_inverse)) {
		SkPoint p = NEMOSHOW_ITEM_CC(item, inverse)->mapXY(x, y);

		if (one->x0 < p.x() && p.x() < one->x1 &&
				one->y0 < p.y() && p.y() < one->y1) {
			SkRegion region;
			SkRegion clip;

			clip.setRect(p.x(), p.y(), p.x() + 1, p.y() + 1);

			return region.setPath(*NEMOSHOW_ITEM_CC(item, path), clip) == true;
		}
	}

	return 0;
}

int nemoshow_item_set_bitmap(struct showone *one, SkBitmap *bitmap)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (NEMOSHOW_ITEM_CC(item, needs_free) == true && NEMOSHOW_ITEM_CC(item, bitmap) != NULL)
		delete NEMOSHOW_ITEM_CC(item, bitmap);

	NEMOSHOW_ITEM_CC(item, bitmap) = bitmap;
	NEMOSHOW_ITEM_CC(item, needs_free) = false;

	nemoshow_one_dirty(one, NEMOSHOW_REDRAW_DIRTY);

	return 0;
}

int nemoshow_item_contain_one(struct showone *one, float x, float y)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (NEMOSHOW_ITEM_CC(item, has_inverse)) {
		SkPoint p = NEMOSHOW_ITEM_CC(item, inverse)->mapXY(x, y);

		if (one->x0 < p.x() && p.x() < one->x1 &&
				one->y0 < p.y() && p.y() < one->y1) {
			if (item->pick == NEMOSHOW_ITEM_NORMAL_PICK) {
				return 1;
			} else if (item->pick == NEMOSHOW_ITEM_PATH_PICK) {
				SkRegion region;
				SkRegion clip;

				clip.setRect(p.x(), p.y(), p.x() + 1, p.y() + 1);

				if (region.setPath(*NEMOSHOW_ITEM_CC(item, path), clip) == true)
					return 1;
			}
		}
	}

	return 0;
}

struct showone *nemoshow_item_pick_one(struct showone *one, float x, float y)
{
	struct showone *child;
	struct showone *pick;

	nemoshow_children_for_each_reverse(child, one) {
		if (child->sub == NEMOSHOW_GROUP_ITEM) {
			if (nemoshow_one_has_state(child, NEMOSHOW_PICK_STATE)) {
				if (nemoshow_item_contain_one(child, x, y) != 0)
					return child;
			} else {
				pick = nemoshow_item_pick_one(child, x, y);
				if (pick != NULL)
					return pick;
			}
		} else {
			if (nemoshow_one_has_state(child, NEMOSHOW_PICK_STATE)) {
				if (nemoshow_item_contain_one(child, x, y) != 0)
					return child;
			}
		}
	}

	return NULL;
}

void nemoshow_item_transform_to_global(struct showone *one, float sx, float sy, float *x, float *y)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	SkPoint p = NEMOSHOW_ITEM_CC(item, matrix)->mapXY(sx, sy);

	*x = p.x();
	*y = p.y();
}

void nemoshow_item_transform_from_global(struct showone *one, float x, float y, float *sx, float *sy)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	SkPoint p = NEMOSHOW_ITEM_CC(item, inverse)->mapXY(x, y);

	*sx = p.x();
	*sy = p.y();
}
