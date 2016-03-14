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
#include <showfont.h>
#include <showfont.hpp>
#include <nemoshow.h>
#include <fonthelper.h>
#include <svghelper.h>
#include <stringhelper.h>
#include <colorhelper.h>
#include <nemoxml.h>
#include <nemobox.h>
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
	NEMOSHOW_ITEM_CC(item, width) = 0;
	NEMOSHOW_ITEM_CC(item, height) = 0;

	item->alpha = 1.0f;
	item->from = 0.0f;
	item->to = 1.0f;

	item->sx = 1.0f;
	item->sy = 1.0f;

	item->fills[NEMOSHOW_ITEM_ALPHA_COLOR] = 255.0f;
	item->fills[NEMOSHOW_ITEM_RED_COLOR] = 255.0f;
	item->fills[NEMOSHOW_ITEM_GREEN_COLOR] = 255.0f;
	item->fills[NEMOSHOW_ITEM_BLUE_COLOR] = 255.0f;
	item->strokes[NEMOSHOW_ITEM_ALPHA_COLOR] = 255.0f;
	item->strokes[NEMOSHOW_ITEM_RED_COLOR] = 255.0f;
	item->strokes[NEMOSHOW_ITEM_GREEN_COLOR] = 255.0f;
	item->strokes[NEMOSHOW_ITEM_BLUE_COLOR] = 255.0f;

	item->_alpha = 1.0f;

	item->_fills[NEMOSHOW_ITEM_ALPHA_COLOR] = 255.0f;
	item->_fills[NEMOSHOW_ITEM_RED_COLOR] = 255.0f;
	item->_fills[NEMOSHOW_ITEM_GREEN_COLOR] = 255.0f;
	item->_fills[NEMOSHOW_ITEM_BLUE_COLOR] = 255.0f;
	item->_strokes[NEMOSHOW_ITEM_ALPHA_COLOR] = 255.0f;
	item->_strokes[NEMOSHOW_ITEM_RED_COLOR] = 255.0f;
	item->_strokes[NEMOSHOW_ITEM_GREEN_COLOR] = 255.0f;
	item->_strokes[NEMOSHOW_ITEM_BLUE_COLOR] = 255.0f;

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
	} else if (one->sub == NEMOSHOW_PATHTWICE_ITEM) {
		NEMOSHOW_ITEM_CC(item, fillpath) = new SkPath;
	} else if (one->sub == NEMOSHOW_PATHARRAY_ITEM) {
		item->cmds = (uint32_t *)malloc(sizeof(uint32_t) * 8);
		item->ncmds = 0;
		item->scmds = 8;

		item->points = (double *)malloc(sizeof(double) * 8);
		item->npoints = 0;
		item->spoints = 8;
	} else if (one->sub == NEMOSHOW_PATHGROUP_ITEM) {
		nemoshow_one_set_state(one, NEMOSHOW_INHERIT_STATE);
	} else if (one->sub == NEMOSHOW_POINTS_ITEM || one->sub == NEMOSHOW_POLYLINE_ITEM || one->sub == NEMOSHOW_POLYGON_ITEM) {
		item->points = (double *)malloc(sizeof(double) * 8);
		item->npoints = 0;
		item->spoints = 8;
	} else if (one->sub == NEMOSHOW_GROUP_ITEM) {
		nemoshow_one_set_state(one, NEMOSHOW_INHERIT_STATE);
		nemoshow_one_set_state(one, NEMOSHOW_BOUNDS_STATE);
	} else if (one->sub == NEMOSHOW_SVG_ITEM) {
		nemoshow_one_set_state(one, NEMOSHOW_INHERIT_STATE);
	} else if (one->sub == NEMOSHOW_CONTAINER_ITEM) {
		nemoshow_one_set_state(one, NEMOSHOW_INHERIT_STATE);
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
	if (NEMOSHOW_ITEM_CC(item, bitmap) != NULL)
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

int nemoshow_item_arrange(struct showone *one)
{
	struct nemoshow *show = one->show;
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showone *filter;
	struct showone *shader;
	struct showone *matrix;
	struct showone *path;
	struct showone *clip;
	struct showone *font;
	struct showone *child;
	const char *v;
	int i;

	v = nemoobject_gets(&one->object, "filter");
	if (v != NULL && (filter = nemoshow_search_one(show, v)) != NULL) {
		nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_FILTER_REF));
		nemoshow_one_reference_one(one, filter, NEMOSHOW_FILTER_DIRTY, 0x0, NEMOSHOW_FILTER_REF);
	}

	v = nemoobject_gets(&one->object, "shader");
	if (v != NULL && (shader = nemoshow_search_one(show, v)) != NULL) {
		nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_SHADER_REF));
		nemoshow_one_reference_one(one, shader, NEMOSHOW_SHADER_DIRTY, 0x0, NEMOSHOW_SHADER_REF);
	}

	v = nemoobject_gets(&one->object, "matrix");
	if (v != NULL) {
		if (strcmp(v, "tsr") == 0) {
			item->transform = NEMOSHOW_ITEM_TSR_TRANSFORM;
		} else if ((matrix = nemoshow_search_one(show, v)) != NULL) {
			item->transform = NEMOSHOW_ITEM_EXTERN_TRANSFORM;

			nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_MATRIX_REF));
			nemoshow_one_reference_one(one, matrix, NEMOSHOW_MATRIX_DIRTY, NEMOSHOW_TRANSFORM_STATE, NEMOSHOW_MATRIX_REF);
		}
	} else {
		nemoshow_children_for_each(child, one) {
			if (child->type == NEMOSHOW_MATRIX_TYPE) {
				item->transform = NEMOSHOW_ITEM_CHILDREN_TRANSFORM;

				break;
			}
		}
	}

	v = nemoobject_gets(&one->object, "clip");
	if (v != NULL && (clip = nemoshow_search_one(show, v)) != NULL) {
		nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_CLIP_REF));
		nemoshow_one_reference_one(one, clip, NEMOSHOW_REDRAW_DIRTY, NEMOSHOW_CLIP_STATE, NEMOSHOW_CLIP_REF);
	}

	v = nemoobject_gets(&one->object, "path");
	if (v != NULL && (path = nemoshow_search_one(show, v)) != NULL) {
		nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_PATH_REF));
		nemoshow_one_reference_one(one, path, NEMOSHOW_SHAPE_DIRTY, 0x0, NEMOSHOW_PATH_REF);
	}

	v = nemoobject_gets(&one->object, "font");
	if (v != NULL && (font = nemoshow_search_one(show, v)) != NULL) {
		nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_FONT_REF));
		nemoshow_one_reference_one(one, font, NEMOSHOW_FONT_DIRTY, 0x0, NEMOSHOW_FONT_REF);
	}

	v = nemoobject_gets(&one->object, "uri");
	if (v != NULL)
		item->uri = strdup(v);

	v = nemoobject_gets(&one->object, "text");
	if (v != NULL)
		item->text = strdup(v);

	return 0;
}

static inline void nemoshow_item_update_uri(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	bool r;

	if (item->uri != NULL) {
		if (NEMOSHOW_ITEM_CC(item, bitmap) != NULL)
			delete NEMOSHOW_ITEM_CC(item, bitmap);

		if (one->sub == NEMOSHOW_IMAGE_ITEM) {
			NEMOSHOW_ITEM_CC(item, bitmap) = new SkBitmap;

			r = SkImageDecoder::DecodeFile(item->uri, NEMOSHOW_ITEM_CC(item, bitmap));
			if (r == false) {
				delete NEMOSHOW_ITEM_CC(item, bitmap);

				NEMOSHOW_ITEM_CC(item, bitmap) = NULL;
			}

			one->dirty |= NEMOSHOW_SHAPE_DIRTY;
		} else if (one->sub == NEMOSHOW_SVG_ITEM) {
			nemoshow_svg_load_uri(show, one, item->uri);

			one->dirty |= NEMOSHOW_SHAPE_DIRTY;
		}
	}
}

static inline void nemoshow_item_update_style(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showitem *group;

	if (nemoshow_one_has_state(one, NEMOSHOW_FILL_STATE)) {
		if (nemoshow_one_has_state(one->parent, NEMOSHOW_INHERIT_STATE)) {
			group = NEMOSHOW_ITEM(one->parent);

			item->alpha = item->_alpha * group->alpha;

			item->fills[NEMOSHOW_ITEM_ALPHA_COLOR] = item->_fills[NEMOSHOW_ITEM_ALPHA_COLOR] * (group->fills[NEMOSHOW_ITEM_ALPHA_COLOR] / 255.0f);
			item->fills[NEMOSHOW_ITEM_RED_COLOR] = item->_fills[NEMOSHOW_ITEM_RED_COLOR] * (group->fills[NEMOSHOW_ITEM_RED_COLOR] / 255.0f);
			item->fills[NEMOSHOW_ITEM_GREEN_COLOR] = item->_fills[NEMOSHOW_ITEM_GREEN_COLOR] * (group->fills[NEMOSHOW_ITEM_GREEN_COLOR] / 255.0f);
			item->fills[NEMOSHOW_ITEM_BLUE_COLOR] = item->_fills[NEMOSHOW_ITEM_BLUE_COLOR] * (group->fills[NEMOSHOW_ITEM_BLUE_COLOR] / 255.0f);
		} else {
			item->alpha = item->_alpha;

			item->fills[NEMOSHOW_ITEM_ALPHA_COLOR] = item->_fills[NEMOSHOW_ITEM_ALPHA_COLOR];
			item->fills[NEMOSHOW_ITEM_RED_COLOR] = item->_fills[NEMOSHOW_ITEM_RED_COLOR];
			item->fills[NEMOSHOW_ITEM_GREEN_COLOR] = item->_fills[NEMOSHOW_ITEM_GREEN_COLOR];
			item->fills[NEMOSHOW_ITEM_BLUE_COLOR] = item->_fills[NEMOSHOW_ITEM_BLUE_COLOR];
		}

		NEMOSHOW_ITEM_CC(item, fill)->setColor(
				SkColorSetARGB(
					item->fills[NEMOSHOW_ITEM_ALPHA_COLOR] * item->alpha,
					item->fills[NEMOSHOW_ITEM_RED_COLOR],
					item->fills[NEMOSHOW_ITEM_GREEN_COLOR],
					item->fills[NEMOSHOW_ITEM_BLUE_COLOR]));
	}
	if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE)) {
		if (nemoshow_one_has_state(one->parent, NEMOSHOW_INHERIT_STATE)) {
			group = NEMOSHOW_ITEM(one->parent);

			item->alpha = item->_alpha * group->alpha;

			item->strokes[NEMOSHOW_ITEM_ALPHA_COLOR] = item->_strokes[NEMOSHOW_ITEM_ALPHA_COLOR] * (group->strokes[NEMOSHOW_ITEM_ALPHA_COLOR] / 255.0f);
			item->strokes[NEMOSHOW_ITEM_RED_COLOR] = item->_strokes[NEMOSHOW_ITEM_RED_COLOR] * (group->strokes[NEMOSHOW_ITEM_RED_COLOR] / 255.0f);
			item->strokes[NEMOSHOW_ITEM_GREEN_COLOR] = item->_strokes[NEMOSHOW_ITEM_GREEN_COLOR] * (group->strokes[NEMOSHOW_ITEM_GREEN_COLOR] / 255.0f);
			item->strokes[NEMOSHOW_ITEM_BLUE_COLOR] = item->_strokes[NEMOSHOW_ITEM_BLUE_COLOR] * (group->strokes[NEMOSHOW_ITEM_BLUE_COLOR] / 255.0f);
		} else {
			item->alpha = item->_alpha;

			item->strokes[NEMOSHOW_ITEM_ALPHA_COLOR] = item->_strokes[NEMOSHOW_ITEM_ALPHA_COLOR];
			item->strokes[NEMOSHOW_ITEM_RED_COLOR] = item->_strokes[NEMOSHOW_ITEM_RED_COLOR];
			item->strokes[NEMOSHOW_ITEM_GREEN_COLOR] = item->_strokes[NEMOSHOW_ITEM_GREEN_COLOR];
			item->strokes[NEMOSHOW_ITEM_BLUE_COLOR] = item->_strokes[NEMOSHOW_ITEM_BLUE_COLOR];
		}

		NEMOSHOW_ITEM_CC(item, stroke)->setStrokeWidth(item->stroke_width);
		NEMOSHOW_ITEM_CC(item, stroke)->setColor(
				SkColorSetARGB(
					item->strokes[NEMOSHOW_ITEM_ALPHA_COLOR] * item->alpha * (item->stroke_width > 0.0f ? 1.0f : 0.0f),
					item->strokes[NEMOSHOW_ITEM_RED_COLOR],
					item->strokes[NEMOSHOW_ITEM_GREEN_COLOR],
					item->strokes[NEMOSHOW_ITEM_BLUE_COLOR]));
	}
}

static inline void nemoshow_item_update_filter(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (NEMOSHOW_REF(one, NEMOSHOW_FILTER_REF) != NULL) {
		struct showone *ref = NEMOSHOW_REF(one, NEMOSHOW_FILTER_REF);
		struct showfilter *filter = NEMOSHOW_FILTER(ref);

		NEMOSHOW_ITEM_CC(item, fill)->setMaskFilter(NEMOSHOW_FILTER_CC(filter, filter));
		NEMOSHOW_ITEM_CC(item, stroke)->setMaskFilter(NEMOSHOW_FILTER_CC(filter, filter));

		if (one->sub == NEMOSHOW_PATHGROUP_ITEM) {
			struct showone *child;

			nemoshow_children_for_each(child, one) {
				NEMOSHOW_PATH_ATCC(child, fill)->setMaskFilter(NEMOSHOW_FILTER_CC(filter, filter));
				NEMOSHOW_PATH_ATCC(child, stroke)->setMaskFilter(NEMOSHOW_FILTER_CC(filter, filter));
			}
		}

		one->dirty |= NEMOSHOW_BOUNDS_DIRTY;
	}
}

static inline void nemoshow_item_update_shader(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (NEMOSHOW_REF(one, NEMOSHOW_SHADER_REF) != NULL) {
		NEMOSHOW_ITEM_CC(item, fill)->setShader(NEMOSHOW_SHADER_CC(NEMOSHOW_SHADER(NEMOSHOW_REF(one, NEMOSHOW_SHADER_REF)), shader));

		one->dirty |= NEMOSHOW_BOUNDS_DIRTY;
	}
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
					NEMOBOX_APPEND(item->points, item->spoints, item->npoints, hbglyphspos[i].x_offset * fontscale + item->textwidth + item->x);
					NEMOBOX_APPEND(item->points, item->spoints, item->npoints, item->y - item->fontascent);

					item->textwidth += hbglyphspos[i].x_advance * fontscale;
				}

				item->textheight = item->fontdescent - item->fontascent;

				hb_buffer_destroy(hbbuffer);

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

		nemoobject_set_reserved(&one->object, "points", item->points, sizeof(double) * item->npoints);

		one->dirty |= NEMOSHOW_SHAPE_DIRTY;
	} else if (one->sub == NEMOSHOW_PATHARRAY_ITEM) {
		nemoobject_set_reserved(&one->object, "points", item->points, sizeof(double) * item->npoints);

		one->dirty |= NEMOSHOW_PATH_DIRTY;
	}
}

static inline void nemoshow_item_update_path(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (one->sub == NEMOSHOW_PATHARRAY_ITEM) {
		int i;

		NEMOSHOW_ITEM_CC(item, path)->reset();

		for (i = 0; i < item->ncmds; i++) {
			if (item->cmds[i] == NEMOSHOW_ITEM_PATH_MOVETO_CMD)
				NEMOSHOW_ITEM_CC(item, path)->moveTo(
						item->points[NEMOSHOW_ITEM_PATHARRAY_OFFSET_X0(i)],
						item->points[NEMOSHOW_ITEM_PATHARRAY_OFFSET_Y0(i)]);
			else if (item->cmds[i] == NEMOSHOW_ITEM_PATH_LINETO_CMD)
				NEMOSHOW_ITEM_CC(item, path)->lineTo(
						item->points[NEMOSHOW_ITEM_PATHARRAY_OFFSET_X0(i)],
						item->points[NEMOSHOW_ITEM_PATHARRAY_OFFSET_Y0(i)]);
			else if (item->cmds[i] == NEMOSHOW_ITEM_PATH_CURVETO_CMD)
				NEMOSHOW_ITEM_CC(item, path)->cubicTo(
						item->points[NEMOSHOW_ITEM_PATHARRAY_OFFSET_X0(i)],
						item->points[NEMOSHOW_ITEM_PATHARRAY_OFFSET_Y0(i)],
						item->points[NEMOSHOW_ITEM_PATHARRAY_OFFSET_X1(i)],
						item->points[NEMOSHOW_ITEM_PATHARRAY_OFFSET_Y1(i)],
						item->points[NEMOSHOW_ITEM_PATHARRAY_OFFSET_X2(i)],
						item->points[NEMOSHOW_ITEM_PATHARRAY_OFFSET_Y2(i)]);
			else if (item->cmds[i] == NEMOSHOW_ITEM_PATH_CLOSE_CMD)
				NEMOSHOW_ITEM_CC(item, path)->close();
		}
	}

	if (one->sub == NEMOSHOW_PATH_ITEM || one->sub == NEMOSHOW_PATHTWICE_ITEM || one->sub == NEMOSHOW_PATHARRAY_ITEM) {
		if (item->pathsegment >= 1.0f) {
			SkPathEffect *effect;

			effect = SkDiscretePathEffect::Create(item->pathsegment, item->pathdeviation, item->pathseed);
			if (effect != NULL) {
				NEMOSHOW_ITEM_CC(item, stroke)->setPathEffect(effect);
				NEMOSHOW_ITEM_CC(item, fill)->setPathEffect(effect);
				effect->unref();
			}
		}

		if (item->pathdashcount > 0) {
			SkPathEffect *effect;
			SkScalar dashes[NEMOSHOW_ITEM_PATH_DASH_MAX];
			int i;

			for (i = 0; i < item->pathdashcount; i++)
				dashes[i] = item->pathdashes[i];

			effect = SkDashPathEffect::Create(dashes, item->pathdashcount, 0);
			if (effect != NULL) {
				NEMOSHOW_ITEM_CC(item, stroke)->setPathEffect(effect);
				NEMOSHOW_ITEM_CC(item, fill)->setPathEffect(effect);
				effect->unref();
			}
		}

		NEMOSHOW_ITEM_CC(item, measure)->setPath(NEMOSHOW_ITEM_CC(item, path), false);

		one->dirty |= NEMOSHOW_SHAPE_DIRTY;
	}
}

static inline void nemoshow_item_update_matrix(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	NEMOSHOW_ITEM_CC(item, matrix)->setIdentity();

	if (nemoshow_one_has_state(one, NEMOSHOW_TRANSFORM_STATE)) {
		if (item->transform == 0)
			item->transform = NEMOSHOW_ITEM_TSR_TRANSFORM;

		NEMOSHOW_ITEM_CC(item, modelview)->setIdentity();

		if (nemoshow_one_has_state(one, NEMOSHOW_VIEWPORT_STATE))
			NEMOSHOW_ITEM_CC(item, modelview)->postScale(item->width / item->width0, item->height / item->height0);

		if (item->transform == NEMOSHOW_ITEM_TSR_TRANSFORM) {
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
		} else if (item->transform == NEMOSHOW_ITEM_EXTERN_TRANSFORM) {
			NEMOSHOW_ITEM_CC(item, modelview)->postConcat(
					*NEMOSHOW_MATRIX_CC(
						NEMOSHOW_MATRIX(
							NEMOSHOW_REF(one, NEMOSHOW_MATRIX_REF)),
						matrix));
		} else if (item->transform == NEMOSHOW_ITEM_CHILDREN_TRANSFORM) {
			struct showone *child;

			nemoshow_children_for_each(child, one) {
				if (child->type == NEMOSHOW_MATRIX_TYPE) {
					NEMOSHOW_ITEM_CC(item, modelview)->postConcat(
							*NEMOSHOW_MATRIX_CC(
								NEMOSHOW_MATRIX(child),
								matrix));
				}
			}
		} else if (item->transform == NEMOSHOW_ITEM_DIRECT_TRANSFORM) {
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

		if (nemoshow_one_has_state(one, NEMOSHOW_ANCHOR_STATE))
			NEMOSHOW_ITEM_CC(item, modelview)->postTranslate(-item->width * item->ax, -item->height * item->ay);

		NEMOSHOW_ITEM_CC(item, matrix)->postConcat(
				*NEMOSHOW_ITEM_CC(item, modelview));
	}

	if (nemoshow_one_has_state(one->parent, NEMOSHOW_INHERIT_STATE)) {
		struct showitem *group = NEMOSHOW_ITEM(one->parent);

		NEMOSHOW_ITEM_CC(item, matrix)->postConcat(
				*NEMOSHOW_ITEM_CC(group, matrix));
	}

	NEMOSHOW_ITEM_CC(item, has_inverse) =
		NEMOSHOW_ITEM_CC(item, matrix)->invert(
				NEMOSHOW_ITEM_CC(item, inverse));

	one->dirty |= NEMOSHOW_BOUNDS_DIRTY;
}

static inline void nemoshow_item_update_shape(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (one->sub == NEMOSHOW_CIRCLE_ITEM) {
		item->width = item->r * 2;
		item->height = item->r * 2;

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

		item->width = box.width();
		item->height = box.height();

		one->dirty |= NEMOSHOW_SIZE_DIRTY;
	} else if (one->sub == NEMOSHOW_TEXTBOX_ITEM) {
		NEMOSHOW_ITEM_CC(item, textbox)->setBox(item->x, item->y, item->width, item->height);
	}

	one->dirty |= NEMOSHOW_BOUNDS_DIRTY;
}

void nemoshow_item_update_bounds(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one->canvas);
	SkRect box;
	double outer;

	if (one->sub == NEMOSHOW_CIRCLE_ITEM) {
		box = SkRect::MakeXYWH(item->x - item->r, item->y - item->r, item->r * 2, item->r * 2);
	} else if (one->sub == NEMOSHOW_PATH_ITEM || one->sub == NEMOSHOW_PATHTWICE_ITEM || one->sub == NEMOSHOW_PATHARRAY_ITEM) {
		if (nemoshow_one_has_state(one, NEMOSHOW_SIZE_STATE)) {
			box = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);
		} else {
			box = NEMOSHOW_ITEM_CC(item, path)->getBounds();

			if (item->pathsegment >= 1.0f)
				box.outset(fabs(item->pathdeviation), fabs(item->pathdeviation));
		}
	} else if (one->sub == NEMOSHOW_PATHGROUP_ITEM) {
		if (nemoshow_one_has_state(one, NEMOSHOW_SIZE_STATE)) {
			box = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);
		} else {
			struct showone *child;
			struct showpath *path;
			SkRegion region;
			SkRect cbox;
			SkIRect bbox;

			nemoshow_children_for_each(child, one) {
				path = NEMOSHOW_PATH(child);

				cbox = NEMOSHOW_PATH_CC(path, path)->getBounds();

				if (path->pathsegment >= 1.0f)
					cbox.outset(fabs(path->pathdeviation), fabs(path->pathdeviation));

				region.op(
						SkIRect::MakeXYWH(cbox.x(), cbox.y(), cbox.width(), cbox.height()),
						SkRegion::kUnion_Op);
			}

			bbox = region.getBounds();
			box = SkRect::MakeXYWH(bbox.x(), bbox.y(), bbox.width(), bbox.height());
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

		box = SkRect::MakeXYWH(x0, y0, x1 - x0, y1 - y0);
	} else {
		box = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);
	}

	if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE))
		box.outset(item->stroke_width, item->stroke_width);

	one->x0 = box.x();
	one->y0 = box.y();
	one->x1 = box.x() + box.width();
	one->y1 = box.y() + box.height();

	NEMOSHOW_ITEM_CC(item, matrix)->mapRect(&box);

	outer = NEMOSHOW_ANTIALIAS_EPSILON;
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
	one->outer = outer;
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

	item->transform = NEMOSHOW_ITEM_DIRECT_TRANSFORM;

	nemoshow_one_set_state(one, NEMOSHOW_TRANSFORM_STATE);

	nemoshow_one_dirty(one, NEMOSHOW_MATRIX_DIRTY);
}

void nemoshow_item_set_shader(struct showone *one, struct showone *shader)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	nemoshow_one_set_state(one, NEMOSHOW_FILL_STATE);

	nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_SHADER_REF));
	nemoshow_one_reference_one(one, shader, NEMOSHOW_SHADER_DIRTY, 0x0, NEMOSHOW_SHADER_REF);
}

void nemoshow_item_set_filter(struct showone *one, struct showone *filter)
{
	nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_FILTER_REF));
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
		nemoshow_canvas_attach_ones(canvas, one);
}

void nemoshow_item_detach_one(struct showone *one)
{
	nemoshow_one_detach_one(one);

	nemoshow_canvas_detach_ones(one);
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
	}
}

int nemoshow_item_path_moveto(struct showone *one, double x, double y)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (one->sub == NEMOSHOW_PATHARRAY_ITEM) {
		NEMOBOX_APPEND(item->cmds, item->scmds, item->ncmds, NEMOSHOW_ITEM_PATH_MOVETO_CMD);

		NEMOBOX_APPEND(item->points, item->spoints, item->npoints, x);
		NEMOBOX_APPEND(item->points, item->spoints, item->npoints, y);
		NEMOBOX_APPEND(item->points, item->spoints, item->npoints, 0.0f);
		NEMOBOX_APPEND(item->points, item->spoints, item->npoints, 0.0f);
		NEMOBOX_APPEND(item->points, item->spoints, item->npoints, 0.0f);
		NEMOBOX_APPEND(item->points, item->spoints, item->npoints, 0.0f);

		nemoshow_one_dirty(one, NEMOSHOW_POINTS_DIRTY | NEMOSHOW_PATH_DIRTY);

		return item->ncmds - 1;
	} else if (one->sub == NEMOSHOW_PATHTWICE_ITEM) {
		if (item->pathindex == NEMOSHOW_ITEM_PATH_STROKE_INDEX)
			NEMOSHOW_ITEM_CC(item, path)->moveTo(x, y);
		else if (item->pathindex == NEMOSHOW_ITEM_PATH_FILL_INDEX)
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
		NEMOBOX_APPEND(item->cmds, item->scmds, item->ncmds, NEMOSHOW_ITEM_PATH_LINETO_CMD);

		NEMOBOX_APPEND(item->points, item->spoints, item->npoints, x);
		NEMOBOX_APPEND(item->points, item->spoints, item->npoints, y);
		NEMOBOX_APPEND(item->points, item->spoints, item->npoints, 0.0f);
		NEMOBOX_APPEND(item->points, item->spoints, item->npoints, 0.0f);
		NEMOBOX_APPEND(item->points, item->spoints, item->npoints, 0.0f);
		NEMOBOX_APPEND(item->points, item->spoints, item->npoints, 0.0f);

		nemoshow_one_dirty(one, NEMOSHOW_POINTS_DIRTY | NEMOSHOW_PATH_DIRTY);

		return item->ncmds - 1;
	} else if (one->sub == NEMOSHOW_PATHTWICE_ITEM) {
		if (item->pathindex == NEMOSHOW_ITEM_PATH_STROKE_INDEX)
			NEMOSHOW_ITEM_CC(item, path)->lineTo(x, y);
		else if (item->pathindex == NEMOSHOW_ITEM_PATH_FILL_INDEX)
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
		NEMOBOX_APPEND(item->cmds, item->scmds, item->ncmds, NEMOSHOW_ITEM_PATH_CURVETO_CMD);

		NEMOBOX_APPEND(item->points, item->spoints, item->npoints, x0);
		NEMOBOX_APPEND(item->points, item->spoints, item->npoints, y0);
		NEMOBOX_APPEND(item->points, item->spoints, item->npoints, x1);
		NEMOBOX_APPEND(item->points, item->spoints, item->npoints, y1);
		NEMOBOX_APPEND(item->points, item->spoints, item->npoints, x2);
		NEMOBOX_APPEND(item->points, item->spoints, item->npoints, y2);

		nemoshow_one_dirty(one, NEMOSHOW_POINTS_DIRTY | NEMOSHOW_PATH_DIRTY);

		return item->ncmds - 1;
	} else if (one->sub == NEMOSHOW_PATHTWICE_ITEM) {
		if (item->pathindex == NEMOSHOW_ITEM_PATH_STROKE_INDEX)
			NEMOSHOW_ITEM_CC(item, path)->cubicTo(x0, y0, x1, y1, x2, y2);
		else if (item->pathindex == NEMOSHOW_ITEM_PATH_FILL_INDEX)
			NEMOSHOW_ITEM_CC(item, fillpath)->cubicTo(x0, y0, x1, y1, x2, y2);
	} else {
		NEMOSHOW_ITEM_CC(item, path)->cubicTo(x0, y0, x1, y1, x2, y2);
	}

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);

	return 0;
}

int nemoshow_item_path_close(struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (one->sub == NEMOSHOW_PATHARRAY_ITEM) {
		NEMOBOX_APPEND(item->cmds, item->scmds, item->ncmds, NEMOSHOW_ITEM_PATH_CLOSE_CMD);

		NEMOBOX_APPEND(item->points, item->spoints, item->npoints, 0.0f);
		NEMOBOX_APPEND(item->points, item->spoints, item->npoints, 0.0f);
		NEMOBOX_APPEND(item->points, item->spoints, item->npoints, 0.0f);
		NEMOBOX_APPEND(item->points, item->spoints, item->npoints, 0.0f);
		NEMOBOX_APPEND(item->points, item->spoints, item->npoints, 0.0f);
		NEMOBOX_APPEND(item->points, item->spoints, item->npoints, 0.0f);

		nemoshow_one_dirty(one, NEMOSHOW_POINTS_DIRTY | NEMOSHOW_PATH_DIRTY);

		return item->ncmds - 1;
	} else if (one->sub == NEMOSHOW_PATHTWICE_ITEM) {
		if (item->pathindex == NEMOSHOW_ITEM_PATH_STROKE_INDEX)
			NEMOSHOW_ITEM_CC(item, path)->close();
		else if (item->pathindex == NEMOSHOW_ITEM_PATH_FILL_INDEX)
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
		if (item->pathindex == NEMOSHOW_ITEM_PATH_STROKE_INDEX)
			NEMOSHOW_ITEM_CC(item, path)->addPath(path);
		else if (item->pathindex == NEMOSHOW_ITEM_PATH_FILL_INDEX)
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
		if (item->pathindex == NEMOSHOW_ITEM_PATH_STROKE_INDEX)
			NEMOSHOW_ITEM_CC(item, path)->addArc(rect, from, to);
		else if (item->pathindex == NEMOSHOW_ITEM_PATH_FILL_INDEX)
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
	paint.getTextPath(text, textlength, x, y, &path);

	if (one->sub == NEMOSHOW_PATHTWICE_ITEM) {
		if (item->pathindex == NEMOSHOW_ITEM_PATH_STROKE_INDEX)
			NEMOSHOW_ITEM_CC(item, path)->addPath(path);
		else if (item->pathindex == NEMOSHOW_ITEM_PATH_FILL_INDEX)
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

			if (item->pathindex == NEMOSHOW_ITEM_PATH_STROKE_INDEX) {
				nemoshow_children_for_each(child, src) {
					NEMOSHOW_ITEM_CC(item, path)->addPath(*NEMOSHOW_PATH_ATCC(child, path));
				}
			} else if (item->pathindex == NEMOSHOW_ITEM_PATH_FILL_INDEX) {
				nemoshow_children_for_each(child, src) {
					NEMOSHOW_ITEM_CC(item, fillpath)->addPath(*NEMOSHOW_PATH_ATCC(child, path));
				}
			}
		} else if (src->sub == NEMOSHOW_PATHTWICE_ITEM) {
			NEMOSHOW_ITEM_CC(item, path)->addPath(*NEMOSHOW_ITEM_CC(other, path));
			NEMOSHOW_ITEM_CC(item, fillpath)->addPath(*NEMOSHOW_ITEM_CC(other, fillpath));
		} else if (src->sub == NEMOSHOW_PATH_ITEM) {
			if (item->pathindex == NEMOSHOW_ITEM_PATH_STROKE_INDEX)
				NEMOSHOW_ITEM_CC(item, path)->addPath(*NEMOSHOW_ITEM_CC(other, path));
			else if (item->pathindex == NEMOSHOW_ITEM_PATH_FILL_INDEX)
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
				COLOR_DOUBLE_R(c),
				COLOR_DOUBLE_G(c),
				COLOR_DOUBLE_B(c),
				COLOR_DOUBLE_A(c));
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
				COLOR_DOUBLE_R(c),
				COLOR_DOUBLE_G(c),
				COLOR_DOUBLE_B(c),
				COLOR_DOUBLE_A(c));
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
	SkMatrix matrix;
	int has_fill;
	int has_stroke;

	if (uri == NULL)
		return -1;

	xml = nemoxml_create();
	nemoxml_load_file(xml, uri);
	nemoxml_update(xml);

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

		if (strcmp(node->name, "path") == 0) {
			const char *d;

			d = nemoxml_node_get_attr(node, "d");
			if (d != NULL) {
				SkPath rpath;

				SkParsePath::FromSVGString(d, &rpath);

				if (one->sub == NEMOSHOW_PATHGROUP_ITEM) {
					child = nemoshow_path_create();
					nemoshow_one_attach(one, child);

					NEMOSHOW_PATH_ATCC(child, path)->addPath(rpath);

					nemoshow_item_path_load_style(child, node);
				} else if (one->sub == NEMOSHOW_PATHTWICE_ITEM) {
					if (has_fill != 0)
						NEMOSHOW_ITEM_CC(item, fillpath)->addPath(rpath);
					if (has_stroke != 0)
						NEMOSHOW_ITEM_CC(item, path)->addPath(rpath);
				} else {
					NEMOSHOW_ITEM_CC(item, path)->addPath(rpath);
				}
			}
		} else if (strcmp(node->name, "line") == 0) {
			double x1 = strtod(nemoxml_node_get_attr(node, "x1"), NULL);
			double y1 = strtod(nemoxml_node_get_attr(node, "y1"), NULL);
			double x2 = strtod(nemoxml_node_get_attr(node, "x2"), NULL);
			double y2 = strtod(nemoxml_node_get_attr(node, "y2"), NULL);

			if (one->sub == NEMOSHOW_PATHGROUP_ITEM) {
				child = nemoshow_path_create();
				nemoshow_one_attach(one, child);

				NEMOSHOW_PATH_ATCC(child, path)->moveTo(x1, y1);
				NEMOSHOW_PATH_ATCC(child, path)->lineTo(x2, y2);

				nemoshow_item_path_load_style(child, node);
			} else if (one->sub == NEMOSHOW_PATHTWICE_ITEM) {
				if (has_fill != 0) {
					NEMOSHOW_ITEM_CC(item, fillpath)->moveTo(x1, y1);
					NEMOSHOW_ITEM_CC(item, fillpath)->lineTo(x2, y2);
				}
				if (has_stroke != 0) {
					NEMOSHOW_ITEM_CC(item, path)->moveTo(x1, y1);
					NEMOSHOW_ITEM_CC(item, path)->lineTo(x2, y2);
				}
			} else {
				NEMOSHOW_ITEM_CC(item, path)->moveTo(x1, y1);
				NEMOSHOW_ITEM_CC(item, path)->lineTo(x2, y2);
			}
		} else if (strcmp(node->name, "rect") == 0) {
			double x = strtod(nemoxml_node_get_attr(node, "x"), NULL);
			double y = strtod(nemoxml_node_get_attr(node, "y"), NULL);
			double width = strtod(nemoxml_node_get_attr(node, "width"), NULL);
			double height = strtod(nemoxml_node_get_attr(node, "height"), NULL);

			if (one->sub == NEMOSHOW_PATHGROUP_ITEM) {
				child = nemoshow_path_create();
				nemoshow_one_attach(one, child);

				NEMOSHOW_PATH_ATCC(child, path)->addRect(x, y, x + width, y + height);

				nemoshow_item_path_load_style(child, node);
			} else if (one->sub == NEMOSHOW_PATHTWICE_ITEM) {
				if (has_fill != 0)
					NEMOSHOW_ITEM_CC(item, fillpath)->addRect(x, y, x + width, y + height);
				if (has_stroke != 0)
					NEMOSHOW_ITEM_CC(item, path)->addRect(x, y, x + width, y + height);
			} else {
				NEMOSHOW_ITEM_CC(item, path)->addRect(x, y, x + width, y + height);
			}
		} else if (strcmp(node->name, "circle") == 0) {
			double x = strtod(nemoxml_node_get_attr(node, "cx"), NULL);
			double y = strtod(nemoxml_node_get_attr(node, "cy"), NULL);
			double r = strtod(nemoxml_node_get_attr(node, "r"), NULL);

			if (one->sub == NEMOSHOW_PATHGROUP_ITEM) {
				child = nemoshow_path_create();
				nemoshow_one_attach(one, child);

				NEMOSHOW_PATH_ATCC(child, path)->addCircle(x, y, r);

				nemoshow_item_path_load_style(child, node);
			} else if (one->sub == NEMOSHOW_PATHTWICE_ITEM) {
				if (has_fill != 0)
					NEMOSHOW_ITEM_CC(item, fillpath)->addCircle(x, y, r);
				if (has_stroke != 0)
					NEMOSHOW_ITEM_CC(item, path)->addCircle(x, y, r);
			} else {
				NEMOSHOW_ITEM_CC(item, path)->addCircle(x, y, r);
			}
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

			if (one->sub == NEMOSHOW_PATHGROUP_ITEM) {
				child = nemoshow_path_create();
				nemoshow_one_attach(one, child);

				NEMOSHOW_PATH_ATCC(child, path)->moveTo(
						strtod(nemotoken_get_token(token, 0), NULL),
						strtod(nemotoken_get_token(token, 1), NULL));

				for (i = 2; i < count; i += 2) {
					NEMOSHOW_PATH_ATCC(child, path)->lineTo(
							strtod(nemotoken_get_token(token, i + 0), NULL),
							strtod(nemotoken_get_token(token, i + 1), NULL));
				}

				NEMOSHOW_PATH_ATCC(child, path)->close();

				nemoshow_item_path_load_style(child, node);
			} else if (one->sub == NEMOSHOW_PATHTWICE_ITEM) {
				if (has_fill != 0) {
					NEMOSHOW_ITEM_CC(item, fillpath)->moveTo(
							strtod(nemotoken_get_token(token, 0), NULL),
							strtod(nemotoken_get_token(token, 1), NULL));

					for (i = 2; i < count; i += 2) {
						NEMOSHOW_ITEM_CC(item, fillpath)->lineTo(
								strtod(nemotoken_get_token(token, i + 0), NULL),
								strtod(nemotoken_get_token(token, i + 1), NULL));
					}

					NEMOSHOW_ITEM_CC(item, fillpath)->close();
				}
				if (has_stroke != 0) {
					NEMOSHOW_ITEM_CC(item, path)->moveTo(
							strtod(nemotoken_get_token(token, 0), NULL),
							strtod(nemotoken_get_token(token, 1), NULL));

					for (i = 2; i < count; i += 2) {
						NEMOSHOW_ITEM_CC(item, path)->lineTo(
								strtod(nemotoken_get_token(token, i + 0), NULL),
								strtod(nemotoken_get_token(token, i + 1), NULL));
					}

					NEMOSHOW_ITEM_CC(item, path)->close();
				}
			} else {
				NEMOSHOW_ITEM_CC(item, path)->moveTo(
						strtod(nemotoken_get_token(token, 0), NULL),
						strtod(nemotoken_get_token(token, 1), NULL));

				for (i = 2; i < count; i += 2) {
					NEMOSHOW_ITEM_CC(item, path)->lineTo(
							strtod(nemotoken_get_token(token, i + 0), NULL),
							strtod(nemotoken_get_token(token, i + 1), NULL));
				}

				NEMOSHOW_ITEM_CC(item, path)->close();
			}

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

			if (one->sub == NEMOSHOW_PATHGROUP_ITEM) {
				child = nemoshow_path_create();
				nemoshow_one_attach(one, child);

				NEMOSHOW_PATH_ATCC(child, path)->moveTo(
						strtod(nemotoken_get_token(token, 0), NULL),
						strtod(nemotoken_get_token(token, 1), NULL));

				for (i = 2; i < count; i += 2) {
					NEMOSHOW_PATH_ATCC(child, path)->lineTo(
							strtod(nemotoken_get_token(token, i + 0), NULL),
							strtod(nemotoken_get_token(token, i + 1), NULL));
				}

				nemoshow_item_path_load_style(child, node);
			} else if (one->sub == NEMOSHOW_PATHTWICE_ITEM) {
				if (has_fill != 0) {
					NEMOSHOW_ITEM_CC(item, fillpath)->moveTo(
							strtod(nemotoken_get_token(token, 0), NULL),
							strtod(nemotoken_get_token(token, 1), NULL));

					for (i = 2; i < count; i += 2) {
						NEMOSHOW_ITEM_CC(item, fillpath)->lineTo(
								strtod(nemotoken_get_token(token, i + 0), NULL),
								strtod(nemotoken_get_token(token, i + 1), NULL));
					}
				}
				if (has_stroke != 0) {
					NEMOSHOW_ITEM_CC(item, path)->moveTo(
							strtod(nemotoken_get_token(token, 0), NULL),
							strtod(nemotoken_get_token(token, 1), NULL));

					for (i = 2; i < count; i += 2) {
						NEMOSHOW_ITEM_CC(item, path)->lineTo(
								strtod(nemotoken_get_token(token, i + 0), NULL),
								strtod(nemotoken_get_token(token, i + 1), NULL));
					}
				}
			} else {
				NEMOSHOW_ITEM_CC(item, path)->moveTo(
						strtod(nemotoken_get_token(token, 0), NULL),
						strtod(nemotoken_get_token(token, 1), NULL));

				for (i = 2; i < count; i += 2) {
					NEMOSHOW_ITEM_CC(item, path)->lineTo(
							strtod(nemotoken_get_token(token, i + 0), NULL),
							strtod(nemotoken_get_token(token, i + 1), NULL));
				}
			}

			nemotoken_destroy(token);
		}
	}

	nemoxml_destroy(xml);

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

	return 0;
}

void nemoshow_item_path_use(struct showone *one, int index)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->pathindex = index;
}

void nemoshow_item_path_translate(struct showone *one, double x, double y)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showone *child;
	SkMatrix matrix;

	matrix.setIdentity();
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

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

void nemoshow_item_path_scale(struct showone *one, double sx, double sy)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showone *child;
	SkMatrix matrix;

	matrix.setIdentity();
	matrix.postScale(sx, sy);

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

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

void nemoshow_item_path_rotate(struct showone *one, double ro)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showone *child;
	SkMatrix matrix;

	matrix.setIdentity();
	matrix.postRotate(ro);

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

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

void nemoshow_item_path_set_discrete_effect(struct showone *one, double segment, double deviation, uint32_t seed)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->pathsegment = segment;
	item->pathdeviation = deviation;
	item->pathseed = seed;

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
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

	nemoobject_set_reserved(&one->object, "pathdash", item->pathdashes, sizeof(double) * dashcount);

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

int nemoshow_item_path_contains_point(struct showone *one, double x, double y)
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

void nemoshow_item_clear_points(struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->npoints = 0;
}

int nemoshow_item_append_point(struct showone *one, double x, double y)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	NEMOBOX_APPEND(item->points, item->spoints, item->npoints, x);
	NEMOBOX_APPEND(item->points, item->spoints, item->npoints, y);

	return item->npoints / 2 - 1;
}

int nemoshow_item_set_buffer(struct showone *one, char *buffer, uint32_t width, uint32_t height)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (NEMOSHOW_ITEM_CC(item, bitmap) != NULL)
		delete NEMOSHOW_ITEM_CC(item, bitmap);

	NEMOSHOW_ITEM_CC(item, bitmap) = new SkBitmap;

	if (buffer != NULL) {
		NEMOSHOW_ITEM_CC(item, bitmap)->setInfo(
				SkImageInfo::Make(width, height, kN32_SkColorType, kPremul_SkAlphaType));
		NEMOSHOW_ITEM_CC(item, bitmap)->setPixels(
				buffer);
	} else {
		NEMOSHOW_ITEM_CC(item, bitmap)->allocPixels(
				SkImageInfo::Make(width, height, kN32_SkColorType, kPremul_SkAlphaType));
	}

	NEMOSHOW_ITEM_CC(item, width) = width;
	NEMOSHOW_ITEM_CC(item, height) = height;

	nemoshow_one_dirty(one, NEMOSHOW_REDRAW_DIRTY);

	return 0;
}

void nemoshow_item_put_buffer(struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	delete NEMOSHOW_ITEM_CC(item, bitmap);

	NEMOSHOW_ITEM_CC(item, bitmap) = NULL;

	NEMOSHOW_ITEM_CC(item, width) = 0;
	NEMOSHOW_ITEM_CC(item, height) = 0;

	nemoshow_one_dirty(one, NEMOSHOW_REDRAW_DIRTY);
}

int nemoshow_item_copy_buffer(struct showone *one, char *buffer, uint32_t width, uint32_t height)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	SkBitmap bitmap;

	if (NEMOSHOW_ITEM_CC(item, width) != width ||
			NEMOSHOW_ITEM_CC(item, height) != height) {
		if (NEMOSHOW_ITEM_CC(item, bitmap) != NULL)
			delete NEMOSHOW_ITEM_CC(item, bitmap);

		NEMOSHOW_ITEM_CC(item, bitmap) = new SkBitmap;
		NEMOSHOW_ITEM_CC(item, bitmap)->allocPixels(
				SkImageInfo::Make(width, height, kN32_SkColorType, kPremul_SkAlphaType));

		NEMOSHOW_ITEM_CC(item, width) = width;
		NEMOSHOW_ITEM_CC(item, height) = height;
	}

	bitmap.setInfo(SkImageInfo::Make(width, height, kN32_SkColorType, kPremul_SkAlphaType));
	bitmap.setPixels(buffer);

	bitmap.copyTo(NEMOSHOW_ITEM_CC(item, bitmap));

	nemoshow_one_dirty(one, NEMOSHOW_REDRAW_DIRTY);

	return 0;
}

int nemoshow_item_fill_buffer(struct showone *one, double r, double g, double b, double a)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (NEMOSHOW_ITEM_CC(item, bitmap) != NULL) {
		SkBitmapDevice device(*NEMOSHOW_ITEM_CC(item, bitmap));
		SkCanvas canvas(&device);

		canvas.clear(SkColorSetARGB(a, r, g, b));
	}

	nemoshow_one_dirty(one, NEMOSHOW_REDRAW_DIRTY);

	return 0;
}

int nemoshow_item_set_bitmap(struct showone *one, SkBitmap *bitmap)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (NEMOSHOW_ITEM_CC(item, bitmap) != NULL)
		delete NEMOSHOW_ITEM_CC(item, bitmap);

	NEMOSHOW_ITEM_CC(item, bitmap) = bitmap;

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
			if (child->tag != 0) {
				if (nemoshow_item_contain_one(child, x, y) != 0)
					return child;
			} else {
				pick = nemoshow_item_pick_one(child, x, y);
				if (pick != NULL)
					return pick;
			}
		} else {
			if (child->tag != 0) {
				if (nemoshow_item_contain_one(child, x, y) != 0)
					return child;
			}
		}
	}

	return NULL;
}
