#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

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
#include <showfont.h>
#include <showfont.hpp>
#include <showhelper.hpp>
#include <nemoshow.h>
#include <fonthelper.h>
#include <svghelper.h>
#include <nemoxml.h>
#include <nemobox.h>
#include <nemomisc.h>

#define	NEMOSHOW_ANTIALIAS_EPSILON			(3.0f)

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
	NEMOSHOW_ITEM_CC(item, viewbox) = new SkMatrix;
	NEMOSHOW_ITEM_CC(item, path) = new SkPath;
	NEMOSHOW_ITEM_CC(item, fill) = new SkPaint;
	NEMOSHOW_ITEM_CC(item, fill)->setStyle(SkPaint::kFill_Style);
	NEMOSHOW_ITEM_CC(item, fill)->setAntiAlias(true);
	NEMOSHOW_ITEM_CC(item, stroke) = new SkPaint;
	NEMOSHOW_ITEM_CC(item, stroke)->setStyle(SkPaint::kStroke_Style);
	NEMOSHOW_ITEM_CC(item, stroke)->setAntiAlias(true);
	NEMOSHOW_ITEM_CC(item, points) = NULL;
	NEMOSHOW_ITEM_CC(item, bitmap) = NULL;

	item->alpha = 1.0f;
	item->from = 0.0f;
	item->to = 1.0f;

	item->sx = 1.0f;
	item->sy = 1.0f;

	one = &item->base;
	one->type = NEMOSHOW_ITEM_TYPE;
	one->sub = type;
	one->update = nemoshow_item_update;
	one->destroy = nemoshow_item_destroy;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "x", &item->x, sizeof(double));
	nemoobject_set_reserved(&one->object, "y", &item->y, sizeof(double));
	nemoobject_set_reserved(&one->object, "rx", &item->rx, sizeof(double));
	nemoobject_set_reserved(&one->object, "ry", &item->ry, sizeof(double));
	nemoobject_set_reserved(&one->object, "width", &item->width, sizeof(double));
	nemoobject_set_reserved(&one->object, "height", &item->height, sizeof(double));
	nemoobject_set_reserved(&one->object, "r", &item->r, sizeof(double));
	nemoobject_set_reserved(&one->object, "inner", &item->inner, sizeof(double));

	nemoobject_set_reserved(&one->object, "tx", &item->tx, sizeof(double));
	nemoobject_set_reserved(&one->object, "ty", &item->ty, sizeof(double));
	nemoobject_set_reserved(&one->object, "sx", &item->sx, sizeof(double));
	nemoobject_set_reserved(&one->object, "sy", &item->sy, sizeof(double));
	nemoobject_set_reserved(&one->object, "px", &item->px, sizeof(double));
	nemoobject_set_reserved(&one->object, "py", &item->py, sizeof(double));
	nemoobject_set_reserved(&one->object, "ro", &item->ro, sizeof(double));

	nemoobject_set_reserved(&one->object, "from", &item->from, sizeof(double));
	nemoobject_set_reserved(&one->object, "to", &item->to, sizeof(double));

	nemoobject_set_reserved(&one->object, "stroke", &item->stroke, sizeof(uint32_t));
	nemoobject_set_reserved(&one->object, "stroke:r", &item->strokes[2], sizeof(double));
	nemoobject_set_reserved(&one->object, "stroke:g", &item->strokes[1], sizeof(double));
	nemoobject_set_reserved(&one->object, "stroke:b", &item->strokes[0], sizeof(double));
	nemoobject_set_reserved(&one->object, "stroke:a", &item->strokes[3], sizeof(double));
	nemoobject_set_reserved(&one->object, "stroke-width", &item->stroke_width, sizeof(double));
	nemoobject_set_reserved(&one->object, "fill", &item->fill, sizeof(uint32_t));
	nemoobject_set_reserved(&one->object, "fill:r", &item->fills[2], sizeof(double));
	nemoobject_set_reserved(&one->object, "fill:g", &item->fills[1], sizeof(double));
	nemoobject_set_reserved(&one->object, "fill:b", &item->fills[0], sizeof(double));
	nemoobject_set_reserved(&one->object, "fill:a", &item->fills[3], sizeof(double));

	nemoobject_set_reserved(&one->object, "font-size", &item->fontsize, sizeof(double));

	nemoobject_set_reserved(&one->object, "alpha", &item->alpha, sizeof(double));

	nemoobject_set_reserved(&one->object, "event", &item->event, sizeof(int32_t));

	nemoshow_one_reference_one(one, one, NEMOSHOW_STYLE_REF);

	return one;
}

void nemoshow_item_destroy(struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (item->canvas != NULL) {
		nemoshow_canvas_damage_one(item->canvas, one);
	}

	nemoshow_one_unreference_all(one);

	while (one->nchildren > 0) {
		nemoshow_one_destroy_with_children(one->children[0]);
	}

	nemoshow_one_finish(one);

	if (NEMOSHOW_ITEM_CC(item, matrix) != NULL)
		delete NEMOSHOW_ITEM_CC(item, matrix);
	if (NEMOSHOW_ITEM_CC(item, viewbox) != NULL)
		delete NEMOSHOW_ITEM_CC(item, viewbox);
	if (NEMOSHOW_ITEM_CC(item, path) != NULL)
		delete NEMOSHOW_ITEM_CC(item, path);
	if (NEMOSHOW_ITEM_CC(item, fill) != NULL)
		delete NEMOSHOW_ITEM_CC(item, fill);
	if (NEMOSHOW_ITEM_CC(item, stroke) != NULL)
		delete NEMOSHOW_ITEM_CC(item, stroke);
	if (NEMOSHOW_ITEM_CC(item, points) != NULL)
		delete NEMOSHOW_ITEM_CC(item, points);
	if (NEMOSHOW_ITEM_CC(item, bitmap) != NULL)
		delete NEMOSHOW_ITEM_CC(item, bitmap);

	delete static_cast<showitem_t *>(item->cc);

	if (item->uri != NULL)
		free(item->uri);

	free(item);
}

int nemoshow_item_arrange(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showone *style;
	struct showone *filter;
	struct showone *shader;
	struct showone *matrix;
	struct showone *path;
	struct showone *clip;
	struct showone *font;
	const char *v;
	int i;

	v = nemoobject_gets(&one->object, "style");
	if (v != NULL && (style = nemoshow_search_one(show, v)) != NULL) {
		nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_STYLE_REF));
		nemoshow_one_reference_one(one, style, NEMOSHOW_STYLE_REF);
	} else {
		v = nemoobject_gets(&one->object, "filter");
		if (v != NULL && (filter = nemoshow_search_one(show, v)) != NULL) {
			nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_FILTER_REF));
			nemoshow_one_reference_one(one, filter, NEMOSHOW_FILTER_REF);
		}

		v = nemoobject_gets(&one->object, "shader");
		if (v != NULL && (shader = nemoshow_search_one(show, v)) != NULL) {
			nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_SHADER_REF));
			nemoshow_one_reference_one(one, shader, NEMOSHOW_SHADER_REF);
		}
	}

	v = nemoobject_gets(&one->object, "matrix");
	if (v != NULL) {
		if (strcmp(v, "tsr") == 0) {
			item->transform = NEMOSHOW_TSR_TRANSFORM;
		} else if ((matrix = nemoshow_search_one(show, v)) != NULL) {
			item->transform = NEMOSHOW_EXTERN_TRANSFORM;

			nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_MATRIX_REF));
			nemoshow_one_reference_one(one, matrix, NEMOSHOW_MATRIX_REF);
		}
	} else {
		for (i = 0; i < one->nchildren; i++) {
			if (one->children[i]->type == NEMOSHOW_MATRIX_TYPE) {
				item->transform = NEMOSHOW_CHILDREN_TRANSFORM;

				break;
			}
		}
	}

	v = nemoobject_gets(&one->object, "clip");
	if (v != NULL && (clip = nemoshow_search_one(show, v)) != NULL) {
		nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_CLIP_REF));
		nemoshow_one_reference_one(one, clip, NEMOSHOW_CLIP_REF);
	}

	v = nemoobject_gets(&one->object, "path");
	if (v != NULL && (path = nemoshow_search_one(show, v)) != NULL) {
		nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_PATH_REF));
		nemoshow_one_reference_one(one, path, NEMOSHOW_PATH_REF);
	}

	v = nemoobject_gets(&one->object, "font");
	if (v != NULL && (font = nemoshow_search_one(show, v)) != NULL) {
		nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_FONT_REF));
		nemoshow_one_reference_one(one, font, NEMOSHOW_FONT_REF);
	}

	v = nemoobject_gets(&one->object, "uri");
	if (v != NULL)
		item->uri = strdup(v);

	return 0;
}

static inline void nemoshow_item_update_uri(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	bool r;

	if (NEMOSHOW_ITEM_CC(item, bitmap) != NULL)
		delete NEMOSHOW_ITEM_CC(item, bitmap);

	if (item->uri != NULL) {
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

	if (NEMOSHOW_REF(one, NEMOSHOW_STYLE_REF) == one) {
		if (item->fill != 0) {
			NEMOSHOW_ITEM_CC(item, fill)->setColor(
					SkColorSetARGB(255.0f * item->alpha, item->fills[2], item->fills[1], item->fills[0]));
		}
		if (item->stroke != 0) {
			NEMOSHOW_ITEM_CC(item, stroke)->setStrokeWidth(item->stroke_width);
			NEMOSHOW_ITEM_CC(item, stroke)->setColor(
					SkColorSetARGB(255.0f * item->alpha, item->strokes[2], item->strokes[1], item->strokes[0]));
		}
	}

	if (one->sub == NEMOSHOW_RING_ITEM) {
		NEMOSHOW_ITEM_CC(item, stroke)->setStyle(SkPaint::kFill_Style);
	}
}

static inline void nemoshow_item_update_filter(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (NEMOSHOW_REF(one, NEMOSHOW_FILTER_REF) != NULL) {
		if (item->fill != 0 && item->stroke == 0)
			NEMOSHOW_ITEM_CC(item, fill)->setMaskFilter(NEMOSHOW_FILTER_CC(NEMOSHOW_FILTER(NEMOSHOW_REF(one, NEMOSHOW_FILTER_REF)), filter));
		else
			NEMOSHOW_ITEM_CC(item, stroke)->setMaskFilter(NEMOSHOW_FILTER_CC(NEMOSHOW_FILTER(NEMOSHOW_REF(one, NEMOSHOW_FILTER_REF)), filter));

		one->dirty |= NEMOSHOW_SHAPE_DIRTY;
	}
}

static inline void nemoshow_item_update_shader(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (NEMOSHOW_REF(one, NEMOSHOW_SHADER_REF) != NULL) {
		if (item->fill != 0)
			NEMOSHOW_ITEM_CC(item, fill)->setShader(NEMOSHOW_SHADER_CC(NEMOSHOW_SHADER(NEMOSHOW_REF(one, NEMOSHOW_SHADER_REF)), shader));
	}
}

static inline void nemoshow_item_update_font(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (NEMOSHOW_REF(one, NEMOSHOW_FONT_REF) != NULL) {
		SkSafeUnref(
				NEMOSHOW_ITEM_CC(item, fill)->setTypeface(
					NEMOSHOW_FONT_CC(NEMOSHOW_FONT(NEMOSHOW_REF(one, NEMOSHOW_FONT_REF)), face)));
		SkSafeUnref(
				NEMOSHOW_ITEM_CC(item, stroke)->setTypeface(
					NEMOSHOW_FONT_CC(NEMOSHOW_FONT(NEMOSHOW_REF(one, NEMOSHOW_FONT_REF)), face)));

		one->dirty |= NEMOSHOW_SHAPE_DIRTY;
	}
}

static inline void nemoshow_item_update_text(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (one->sub == NEMOSHOW_TEXT_ITEM) {
		item->text = nemoobject_gets(&one->object, "d");
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

				hbglyphs = hb_buffer_get_glyph_infos(hbbuffer, &nhbglyphs);
				hbglyphspos = hb_buffer_get_glyph_positions(hbbuffer, NULL);

				fontscale = item->fontsize / NEMOSHOW_FONT_AT(NEMOSHOW_REF(one, NEMOSHOW_FONT_REF), max_advance_height);

				if (NEMOSHOW_ITEM_CC(item, points) != NULL)
					delete[] NEMOSHOW_ITEM_CC(item, points);

				NEMOSHOW_ITEM_CC(item, points) = new SkPoint[strlen(item->text)];

				item->textwidth = 0.0f;

				for (i = 0; i < strlen(item->text); i++) {
					NEMOSHOW_ITEM_CC(item, points)[i].set(
							hbglyphspos[i].x_offset * fontscale + item->textwidth + item->x,
							item->y - item->fontascent);

					item->textwidth += hbglyphspos[i].x_advance * fontscale;
				}

				item->textheight = item->fontdescent - item->fontascent;

				hb_buffer_destroy(hbbuffer);
			}
		} else {
			item->textwidth = 0.0f;
			item->textheight = 0.0f;
		}

		one->dirty |= NEMOSHOW_SHAPE_DIRTY;
	}
}

static inline void nemoshow_item_update_path(struct nemoshow *show, struct showitem *item, struct showone *one)
{
	struct showone *child;
	struct showpath *path;
	int i;

	for (i = 0; i < one->nchildren; i++) {
		child = one->children[i];

		if (child->type == NEMOSHOW_PATH_TYPE) {
			path = NEMOSHOW_PATH(child);

			if (child->sub == NEMOSHOW_MOVETO_PATH) {
				NEMOSHOW_ITEM_CC(item, path)->moveTo(path->x0, path->y0);
			} else if (child->sub == NEMOSHOW_LINETO_PATH) {
				NEMOSHOW_ITEM_CC(item, path)->lineTo(path->x0, path->y0);
			} else if (child->sub == NEMOSHOW_CURVETO_PATH) {
				NEMOSHOW_ITEM_CC(item, path)->cubicTo(
						path->x0, path->y0,
						path->x1, path->y1,
						path->x2, path->y2);
			} else if (child->sub == NEMOSHOW_CLOSE_PATH) {
				NEMOSHOW_ITEM_CC(item, path)->close();
			} else if (child->sub == NEMOSHOW_CMD_PATH) {
				SkPath rpath;

				SkParsePath::FromSVGString(nemoobject_gets(&child->object, "d"), &rpath);

				NEMOSHOW_ITEM_CC(item, path)->addPath(rpath);
			} else if (child->sub == NEMOSHOW_RECT_PATH) {
				NEMOSHOW_ITEM_CC(item, path)->addRect(path->x0, path->y0, path->x0 + path->width, path->y0 + path->height);
			} else if (child->sub == NEMOSHOW_CIRCLE_PATH) {
				NEMOSHOW_ITEM_CC(item, path)->addCircle(path->x0, path->y0, path->r);
			} else if (child->sub == NEMOSHOW_TEXT_PATH) {
				SkPaint paint;
				SkPath rpath;
				SkTypeface *face;
				const char *text;

				text = nemoobject_gets(&child->object, "d");

				SkSafeUnref(paint.setTypeface(
							SkTypeface::CreateFromFile(
								fontconfig_get_path(
									nemoobject_gets(&child->object, "font"),
									NULL,
									FC_SLANT_ROMAN,
									FC_WEIGHT_NORMAL,
									FC_WIDTH_NORMAL,
									FC_MONO), 0)));

				paint.setAntiAlias(true);
				paint.setTextSize(nemoobject_getd(&child->object, "font-size"));
				paint.getTextPath(text, strlen(text), path->x0, path->y0, &rpath);

				NEMOSHOW_ITEM_CC(item, path)->addPath(rpath);
			} else if (child->sub == NEMOSHOW_SVG_PATH) {
				nemoshow_item_update_path(show, item, child);
			}
		}
	}
}

static inline void nemoshow_item_update_child(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showone *child;
	int i;

	if (item->transform == NEMOSHOW_CHILDREN_TRANSFORM) {
		NEMOSHOW_ITEM_CC(item, matrix)->setIdentity();

		for (i = 0; i < one->nchildren; i++) {
			child = one->children[i];

			if (child->type == NEMOSHOW_MATRIX_TYPE) {
				NEMOSHOW_ITEM_CC(item, matrix)->postConcat(
						*NEMOSHOW_MATRIX_CC(
							NEMOSHOW_MATRIX(child),
							matrix));
			}
		}

		one->dirty |= NEMOSHOW_SHAPE_DIRTY;
	}

	if (one->sub == NEMOSHOW_PATHGROUP_ITEM) {
		NEMOSHOW_ITEM_CC(item, path)->reset();

		nemoshow_item_update_path(show, item, one);

		item->pathlength = nemoshow_helper_get_path_length(NEMOSHOW_ITEM_CC(item, path));

		one->dirty |= NEMOSHOW_SHAPE_DIRTY;
	}
}

static inline void nemoshow_item_update_matrix(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (item->transform == NEMOSHOW_TSR_TRANSFORM) {
		NEMOSHOW_ITEM_CC(item, matrix)->setIdentity();

		if (item->px != 0.0f || item->py != 0.0f) {
			NEMOSHOW_ITEM_CC(item, matrix)->postTranslate(-item->px, -item->py);

			if (item->ro != 0.0f) {
				NEMOSHOW_ITEM_CC(item, matrix)->postRotate(item->ro);
			}
			if (item->sx != 1.0f || item->sy != 1.0f) {
				NEMOSHOW_ITEM_CC(item, matrix)->postScale(item->sx, item->sy);
			}

			NEMOSHOW_ITEM_CC(item, matrix)->postTranslate(item->px, item->py);
		} else {
			if (item->ro != 0.0f) {
				NEMOSHOW_ITEM_CC(item, matrix)->postRotate(item->ro);
			}
			if (item->sx != 1.0f || item->sy != 1.0f) {
				NEMOSHOW_ITEM_CC(item, matrix)->postScale(item->sx, item->sy);
			}
		}

		NEMOSHOW_ITEM_CC(item, matrix)->postTranslate(item->tx, item->ty);
	}

	one->dirty |= NEMOSHOW_SHAPE_DIRTY;
}

static inline void nemoshow_item_update_shape(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (one->sub == NEMOSHOW_PATH_ITEM) {
		item->pathlength = nemoshow_helper_get_path_length(NEMOSHOW_ITEM_CC(item, path));
	} else if (one->sub == NEMOSHOW_DONUT_ITEM) {
		SkRect outr = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);
		SkRect inr = SkRect::MakeXYWH(item->x + item->inner, item->y + item->inner, item->width - item->inner * 2, item->height - item->inner * 2);

		NEMOSHOW_ITEM_CC(item, path)->reset();

		if (item->from == 0.0f && item->to == 360.0f) {
			NEMOSHOW_ITEM_CC(item, path)->addArc(inr, 0.0f, 360.0f);
			NEMOSHOW_ITEM_CC(item, path)->addArc(outr, 0.0f, 360.0f);
			NEMOSHOW_ITEM_CC(item, path)->setFillType(SkPath::kEvenOdd_FillType);
		} else if (item->from != item->to) {
			NEMOSHOW_ITEM_CC(item, path)->addArc(outr, item->to, item->from - item->to);
			NEMOSHOW_ITEM_CC(item, path)->lineTo(
					item->x + item->width / 2.0f + cos(item->from * M_PI / 180.0f) * (item->width / 2.0f - item->inner),
					item->y + item->height / 2.0f + sin(item->from * M_PI / 180.0f) * (item->height / 2.0f - item->inner));
			NEMOSHOW_ITEM_CC(item, path)->addArc(inr, item->from, item->to - item->from);
			NEMOSHOW_ITEM_CC(item, path)->lineTo(
					item->x + item->width / 2.0f + cos(item->to * M_PI / 180.0f) * (item->width / 2.0f),
					item->y + item->height / 2.0f + sin(item->to * M_PI / 180.0f) * (item->height / 2.0f));
		}
	} else if (one->sub == NEMOSHOW_RING_ITEM) {
		SkRect outr = SkRect::MakeXYWH(item->x - item->r, item->y - item->r, item->r * 2, item->r * 2);
		SkRect inr = SkRect::MakeXYWH(item->x - item->inner, item->y - item->inner, item->inner * 2, item->inner * 2);

		NEMOSHOW_ITEM_CC(item, path)->reset();

		if (item->from == 0.0f && item->to == 360.0f) {
			NEMOSHOW_ITEM_CC(item, path)->addArc(inr, 0.0f, 360.0f);
			NEMOSHOW_ITEM_CC(item, path)->addArc(outr, 0.0f, 360.0f);
			NEMOSHOW_ITEM_CC(item, path)->setFillType(SkPath::kEvenOdd_FillType);
		} else if (item->from != item->to) {
			NEMOSHOW_ITEM_CC(item, path)->addArc(outr, item->to, item->from - item->to);
			NEMOSHOW_ITEM_CC(item, path)->lineTo(
					item->x + cos(item->from * M_PI / 180.0f) * (item->r - item->inner),
					item->y + sin(item->from * M_PI / 180.0f) * (item->r - item->inner));
			NEMOSHOW_ITEM_CC(item, path)->addArc(inr, item->from, item->to - item->from);
			NEMOSHOW_ITEM_CC(item, path)->lineTo(
					item->x + cos(item->to * M_PI / 180.0f) * item->r,
					item->y + sin(item->to * M_PI / 180.0f) * item->r);
		}
	}
}

void nemoshow_item_update_boundingbox(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showone *parent;
	SkRect box;
	SkPoint anchor;
	char attr[NEMOSHOW_SYMBOL_MAX];
	double outer;
	int i;

	if (one->sub == NEMOSHOW_RECT_ITEM) {
		box = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);
	} else if (one->sub == NEMOSHOW_RRECT_ITEM) {
		box = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);
	} else if (one->sub == NEMOSHOW_CIRCLE_ITEM) {
		box = SkRect::MakeXYWH(item->x - item->r, item->y - item->r, item->r * 2, item->r * 2);
	} else if (one->sub == NEMOSHOW_ARC_ITEM) {
		box = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);
	} else if (one->sub == NEMOSHOW_PIE_ITEM) {
		box = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);
	} else if (one->sub == NEMOSHOW_DONUT_ITEM) {
		box = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);
	} else if (one->sub == NEMOSHOW_RING_ITEM) {
		box = SkRect::MakeXYWH(item->x - item->r, item->y - item->r, item->r * 2, item->r * 2);
	} else if (one->sub == NEMOSHOW_PATH_ITEM || one->sub == NEMOSHOW_PATHGROUP_ITEM) {
		box = NEMOSHOW_ITEM_CC(item, path)->getBounds();
	} else if (one->sub == NEMOSHOW_TEXT_ITEM) {
		if (NEMOSHOW_REF(one, NEMOSHOW_PATH_REF) == NULL) {
			if (NEMOSHOW_FONT_AT(NEMOSHOW_REF(one, NEMOSHOW_FONT_REF), layout) == NEMOSHOW_NORMAL_LAYOUT) {
				SkRect bounds[strlen(item->text)];
				int i, count;

				count = NEMOSHOW_ITEM_CC(item, stroke)->getTextWidths(item->text, strlen(item->text), NULL, bounds);

				for (i = 0; i < count; i++) {
					box.join(bounds[i]);
				}
			} else if (NEMOSHOW_FONT_AT(NEMOSHOW_REF(one, NEMOSHOW_FONT_REF), layout) == NEMOSHOW_HARFBUZZ_LAYOUT) {
				box = SkRect::MakeXYWH(item->x, item->y, item->textwidth, item->textheight);
			}
		} else {
			box = NEMOSHOW_ITEM_CC(NEMOSHOW_ITEM(NEMOSHOW_REF(one, NEMOSHOW_PATH_REF)), path)->getBounds();

			box.outset(item->fontsize, item->fontsize);
		}
	} else if (one->sub == NEMOSHOW_BITMAP_ITEM) {
		box = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);
	} else if (one->sub == NEMOSHOW_IMAGE_ITEM) {
		box = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);
	} else if (one->sub == NEMOSHOW_SVG_ITEM) {
		box = SkRect::MakeXYWH(0, 0, item->width, item->height);
	} else {
		box = SkRect::MakeXYWH(0, 0, 0, 0);
	}

	anchor = SkPoint::Make(item->x + item->width / 2.0f, item->y + item->height / 2.0f);

	if (item->stroke != 0)
		box.outset(item->stroke_width, item->stroke_width);

	if (item->transform & NEMOSHOW_EXTERN_TRANSFORM) {
		NEMOSHOW_MATRIX_CC(NEMOSHOW_MATRIX(NEMOSHOW_REF(one, NEMOSHOW_MATRIX_REF)), matrix)->mapRect(&box);
		NEMOSHOW_MATRIX_CC(NEMOSHOW_MATRIX(NEMOSHOW_REF(one, NEMOSHOW_MATRIX_REF)), matrix)->mapPoints(&anchor, 1);
	} else if (item->transform & NEMOSHOW_INTERN_TRANSFORM) {
		NEMOSHOW_ITEM_CC(item, matrix)->mapRect(&box);
		NEMOSHOW_ITEM_CC(item, matrix)->mapPoints(&anchor, 1);
	}

	for (parent = one->parent; parent != NULL; parent = parent->parent) {
		if (parent->type == NEMOSHOW_ITEM_TYPE && parent->sub == NEMOSHOW_GROUP_ITEM) {
			struct showitem *group = NEMOSHOW_ITEM(parent);

			nemoshow_one_update_alone(show, parent);

			if (group->transform & NEMOSHOW_EXTERN_TRANSFORM) {
				NEMOSHOW_MATRIX_CC(NEMOSHOW_MATRIX(NEMOSHOW_REF(parent, NEMOSHOW_MATRIX_REF)), matrix)->mapRect(&box);
				NEMOSHOW_MATRIX_CC(NEMOSHOW_MATRIX(NEMOSHOW_REF(parent, NEMOSHOW_MATRIX_REF)), matrix)->mapPoints(&anchor, 1);
			} else if (group->transform & NEMOSHOW_INTERN_TRANSFORM) {
				NEMOSHOW_ITEM_CC(group, matrix)->mapRect(&box);
				NEMOSHOW_ITEM_CC(group, matrix)->mapPoints(&anchor, 1);
			}
		}
	}

	outer = NEMOSHOW_ANTIALIAS_EPSILON;
	if (NEMOSHOW_REF(one, NEMOSHOW_FILTER_REF) != NULL)
		outer += NEMOSHOW_FILTER_AT(NEMOSHOW_REF(one, NEMOSHOW_FILTER_REF), r) * 2.0f;
	box.outset(outer, outer);

	one->x = MAX(floor(box.x()), 0);
	one->y = MAX(floor(box.y()), 0);
	one->width = ceil(box.width());
	one->height = ceil(box.height());
	one->outer = outer;

	one->ax = anchor.x();
	one->ay = anchor.y();

	snprintf(attr, NEMOSHOW_SYMBOL_MAX, "%s_x", one->id);
	nemoshow_update_symbol(show, attr, one->x);
	snprintf(attr, NEMOSHOW_SYMBOL_MAX, "%s_y", one->id);
	nemoshow_update_symbol(show, attr, one->y);
	snprintf(attr, NEMOSHOW_SYMBOL_MAX, "%s_w", one->id);
	nemoshow_update_symbol(show, attr, one->width);
	snprintf(attr, NEMOSHOW_SYMBOL_MAX, "%s_h", one->id);
	nemoshow_update_symbol(show, attr, one->height);
}

int nemoshow_item_update(struct nemoshow *show, struct showone *one)
{
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
	if ((one->dirty & NEMOSHOW_CHILD_DIRTY) != 0)
		nemoshow_item_update_child(show, one);
	if ((one->dirty & NEMOSHOW_MATRIX_DIRTY) != 0)
		nemoshow_item_update_matrix(show, one);

	if ((one->dirty & NEMOSHOW_SHAPE_DIRTY) != 0) {
		if (item->canvas != NULL)
			nemoshow_canvas_damage_one(item->canvas, one);

		nemoshow_item_update_shape(show, one);

		nemoshow_item_update_boundingbox(show, one);
	}

	if (item->canvas != NULL)
		nemoshow_canvas_damage_one(item->canvas, one);

	return 0;
}

void nemoshow_item_set_matrix(struct showone *one, double m[9])
{
	SkScalar args[9] = {
		SkDoubleToScalar(m[0]), SkDoubleToScalar(m[1]), SkDoubleToScalar(m[2]),
		SkDoubleToScalar(m[3]), SkDoubleToScalar(m[4]), SkDoubleToScalar(m[5]),
		SkDoubleToScalar(m[6]), SkDoubleToScalar(m[7]), SkDoubleToScalar(m[8])
	};
	struct showitem *item = NEMOSHOW_ITEM(one);

	NEMOSHOW_ITEM_CC(item, matrix)->set9(args);

	item->transform = NEMOSHOW_DIRECT_TRANSFORM;
}

void nemoshow_item_set_tsr(struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->transform = NEMOSHOW_TSR_TRANSFORM;
}

void nemoshow_item_set_shader(struct showone *one, struct showone *shader)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->fill = 1;

	nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_SHADER_REF));
	nemoshow_one_reference_one(one, shader, NEMOSHOW_SHADER_REF);
}

void nemoshow_item_set_filter(struct showone *one, struct showone *filter)
{
	nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_FILTER_REF));
	nemoshow_one_reference_one(one, filter, NEMOSHOW_FILTER_REF);
}

void nemoshow_item_set_clip(struct showone *one, struct showone *clip)
{
	nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_CLIP_REF));
	nemoshow_one_reference_one(one, clip, NEMOSHOW_CLIP_REF);
}

void nemoshow_item_set_font(struct showone *one, struct showone *font)
{
	nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_FONT_REF));
	nemoshow_one_reference_one(one, font, NEMOSHOW_FONT_REF);

	nemoshow_one_dirty(one, NEMOSHOW_FONT_DIRTY);
}

void nemoshow_item_set_path(struct showone *one, struct showone *path)
{
	nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_PATH_REF));
	nemoshow_one_reference_one(one, path, NEMOSHOW_PATH_REF);

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

void nemoshow_item_set_src(struct showone *one, struct showone *src)
{
	nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_SRC_REF));
	nemoshow_one_reference_one(one, src, NEMOSHOW_SRC_REF);

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
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
	nemoobject_sets(&one->object, "d", text, strlen(text));

	nemoshow_one_dirty(one, NEMOSHOW_TEXT_DIRTY);
}

void nemoshow_item_attach_one(struct showone *parent, struct showone *one)
{
	if (one->parent != NULL) {
		if (one->parent->sub == NEMOSHOW_GROUP_ITEM)
			nemoshow_one_unreference_one(one, one->parent);

		nemoshow_one_detach_one(one->parent, one);
	}

	nemoshow_one_attach_one(parent, one);

	if (parent->sub == NEMOSHOW_GROUP_ITEM)
		nemoshow_one_reference_one(one, parent, NEMOSHOW_GROUP_REF);

	if (parent->type == NEMOSHOW_CANVAS_TYPE)
		NEMOSHOW_ITEM_AT(one, canvas) = parent;
	else
		NEMOSHOW_ITEM_AT(one, canvas) = NEMOSHOW_ITEM_AT(parent, canvas);
}

void nemoshow_item_detach_one(struct showone *parent, struct showone *one)
{
	if (parent->sub == NEMOSHOW_GROUP_ITEM)
		nemoshow_one_unreference_one(one, parent);

	nemoshow_one_detach_one(parent, one);
}

void nemoshow_item_path_clear(struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	NEMOSHOW_ITEM_CC(item, path)->reset();
}

void nemoshow_item_path_moveto(struct showone *one, double x, double y)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	NEMOSHOW_ITEM_CC(item, path)->moveTo(x, y);
}

void nemoshow_item_path_lineto(struct showone *one, double x, double y)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	NEMOSHOW_ITEM_CC(item, path)->lineTo(x, y);
}

void nemoshow_item_path_cubicto(struct showone *one, double x0, double y0, double x1, double y1, double x2, double y2)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	NEMOSHOW_ITEM_CC(item, path)->cubicTo(x0, y0, x1, y1, x2, y2);
}

void nemoshow_item_path_close(struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	NEMOSHOW_ITEM_CC(item, path)->close();
}

void nemoshow_item_path_cmd(struct showone *one, const char *cmd)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	SkPath path;

	SkParsePath::FromSVGString(cmd, &path);

	NEMOSHOW_ITEM_CC(item, path)->addPath(path);
}

void nemoshow_item_path_arc(struct showone *one, double x, double y, double width, double height, double from, double to)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	SkRect rect = SkRect::MakeXYWH(x, y, width, height);

	NEMOSHOW_ITEM_CC(item, path)->addArc(rect, from, to);
}
