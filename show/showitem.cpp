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
#include <showpath.h>
#include <showfont.h>
#include <showfont.hpp>
#include <nemoshow.h>
#include <nemoxml.h>
#include <nemobox.h>
#include <nemomisc.h>

struct showone *nemoshow_item_create(int type)
{
	struct showitem *item;
	struct showone *one;

	item = (struct showitem *)malloc(sizeof(struct showitem));
	if (item == NULL)
		return NULL;
	memset(item, 0, sizeof(struct showitem));

	item->cc = new showitem_t;
	NEMOSHOW_ITEM_CC(item, matrix) = NULL;
	NEMOSHOW_ITEM_CC(item, path) = NULL;
	NEMOSHOW_ITEM_CC(item, points) = NULL;

	item->alpha = 1.0f;

	one = &item->base;
	one->type = NEMOSHOW_ITEM_TYPE;
	one->sub = type;
	one->update = nemoshow_item_update;
	one->destroy = nemoshow_item_destroy;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "x", &item->x, sizeof(double));
	nemoobject_set_reserved(&one->object, "y", &item->y, sizeof(double));
	nemoobject_set_reserved(&one->object, "width", &item->width, sizeof(double));
	nemoobject_set_reserved(&one->object, "height", &item->height, sizeof(double));
	nemoobject_set_reserved(&one->object, "r", &item->r, sizeof(double));

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

	return one;
}

void nemoshow_item_destroy(struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	nemoshow_one_finish(one);

	delete static_cast<showitem_t *>(item->cc);

	free(item);
}

int nemoshow_item_arrange(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showone *style;
	struct showone *matrix;
	struct showone *path;
	struct showone *child;
	const char *font;
	int i;

	style = nemoshow_search_one(show, nemoobject_gets(&one->object, "style"));
	if (style != NULL) {
		item->style = style;

		NEMOBOX_APPEND(style->refs, style->srefs, style->nrefs, one);
	} else {
		NEMOSHOW_ITEM_CC(item, fill) = new SkPaint;
		NEMOSHOW_ITEM_CC(item, fill)->setAntiAlias(true);
		NEMOSHOW_ITEM_CC(item, stroke) = new SkPaint;
		NEMOSHOW_ITEM_CC(item, stroke)->setAntiAlias(true);

		item->style = one;
	}

	matrix = nemoshow_search_one(show, nemoobject_gets(&one->object, "matrix"));
	if (matrix != NULL) {
		item->matrix = matrix;

		NEMOBOX_APPEND(matrix->refs, matrix->srefs, matrix->nrefs, one);
	}

	path = nemoshow_search_one(show, nemoobject_gets(&one->object, "path"));
	if (path != NULL) {
		item->path = path;

		NEMOBOX_APPEND(path->refs, path->srefs, path->nrefs, one);
	}

	font = nemoobject_gets(&one->object, "font");
	if (font != NULL) {
		const char *fontpath;

		item->font = nemoshow_font_create();

		fontpath = fontconfig_get_path(
				font,
				NULL,
				FC_SLANT_ROMAN,
				FC_WEIGHT_NORMAL,
				FC_WIDTH_NORMAL,
				FC_MONO);

		nemoshow_font_load(item->font, fontpath);
		nemoshow_font_use_harfbuzz(item->font);

		SkSafeUnref(
				NEMOSHOW_ITEM_CC(item, fill)->setTypeface(
					NEMOSHOW_FONT_CC(item->font, face)));
		SkSafeUnref(
				NEMOSHOW_ITEM_CC(item, stroke)->setTypeface(
					NEMOSHOW_FONT_CC(item->font, face)));
	}

	if (one->sub == NEMOSHOW_PATH_ITEM) {
		NEMOSHOW_ITEM_CC(item, path) = new SkPath;
	}

	for (i = 0; i < one->nchildren; i++) {
		if (one->children[i]->type == NEMOSHOW_MATRIX_TYPE) {
			NEMOSHOW_ITEM_CC(item, matrix) = new SkMatrix;
			break;
		}
	}

	return 0;
}

int nemoshow_item_update(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showone *child;
	SkRect box;
	char attr[NEMOSHOW_SYMBOL_MAX];
	int i;

	if (item->style == one) {
		if (item->fill != 0) {
			NEMOSHOW_ITEM_CC(item, fill)->setStyle(SkPaint::kFill_Style);
			NEMOSHOW_ITEM_CC(item, fill)->setColor(
					SkColorSetARGB(255.0f * item->alpha, item->fills[2], item->fills[1], item->fills[0]));
		}
		if (item->stroke != 0) {
			NEMOSHOW_ITEM_CC(item, stroke)->setStyle(SkPaint::kStroke_Style);
			NEMOSHOW_ITEM_CC(item, stroke)->setStrokeWidth(item->stroke_width);
			NEMOSHOW_ITEM_CC(item, stroke)->setColor(
					SkColorSetARGB(255.0f * item->alpha, item->strokes[2], item->strokes[1], item->strokes[0]));
		}
	}

	if (NEMOSHOW_ITEM_CC(item, matrix) != NULL) {
		NEMOSHOW_ITEM_CC(item, matrix)->setIdentity();

		for (i = 0; i < one->nchildren; i++) {
			child = one->children[i];

			if (child->type == NEMOSHOW_MATRIX_TYPE) {
				if (child->dirty != 0) {
					nemoshow_matrix_update(show, child);

					child->dirty = 0;
				}

				NEMOSHOW_ITEM_CC(item, matrix)->postConcat(
						*NEMOSHOW_MATRIX_CC(
							NEMOSHOW_MATRIX(child),
							matrix));
			}
		}
	}

	if (one->sub == NEMOSHOW_PATH_ITEM) {
		struct showpath *path;

		NEMOSHOW_ITEM_CC(item, path)->reset();

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
				}
			}
		}
	} else if (one->sub == NEMOSHOW_TEXT_ITEM) {
		item->text = nemoobject_gets(&one->object, "d");
		if (item->text != NULL) {
			if (item->font->layout == NEMOSHOW_NORMAL_LAYOUT) {
				NEMOSHOW_ITEM_CC(item, fill)->setTextSize(item->fontsize);
				NEMOSHOW_ITEM_CC(item, stroke)->setTextSize(item->fontsize);
			} else {
				SkPaint::FontMetrics metrics;
				hb_buffer_t *hbbuffer;
				hb_glyph_info_t *hbglyphs;
				hb_glyph_position_t *hbglyphspos;
				double fontscale;
				unsigned int nhbglyphs;
				int i;

				NEMOSHOW_ITEM_CC(item, fill)->setTextSize((item->font->upem / item->font->max_advance_height) * item->fontsize);
				NEMOSHOW_ITEM_CC(item, stroke)->setTextSize((item->font->upem / item->font->max_advance_height) * item->fontsize);

				NEMOSHOW_ITEM_CC(item, stroke)->getFontMetrics(&metrics, 0);

				hbbuffer = hb_buffer_create();
				hb_buffer_add_utf8(hbbuffer, item->text, strlen(item->text), 0, strlen(item->text));
				hb_buffer_set_direction(hbbuffer, HB_DIRECTION_LTR);
				hb_buffer_set_script(hbbuffer, HB_SCRIPT_LATIN);
				hb_buffer_set_language(hbbuffer, HB_LANGUAGE_INVALID);
				hb_shape_full(item->font->hbfont, hbbuffer, NULL, 0, NULL);

				hbglyphs = hb_buffer_get_glyph_infos(hbbuffer, &nhbglyphs);
				hbglyphspos = hb_buffer_get_glyph_positions(hbbuffer, NULL);

				fontscale = item->fontsize / item->font->max_advance_height;

				if (NEMOSHOW_ITEM_CC(item, points) != NULL)
					delete NEMOSHOW_ITEM_CC(item, points);

				NEMOSHOW_ITEM_CC(item, points) = new SkPoint[strlen(item->text)];

				item->textwidth = 0.0f;

				for (i = 0; i < strlen(item->text); i++) {
					NEMOSHOW_ITEM_CC(item, points)[i].set(
							hbglyphspos[i].x_offset * fontscale + item->textwidth + item->x,
							item->textheight + item->y - metrics.fAscent);

					item->textwidth += hbglyphspos[i].x_advance * fontscale;
				}

				item->textheight = metrics.fDescent - metrics.fAscent;

				hb_buffer_destroy(hbbuffer);
			}
		}
	}

	if (one->sub == NEMOSHOW_RECT_ITEM) {
		box = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);
	} else if (one->sub == NEMOSHOW_CIRCLE_ITEM) {
		box = SkRect::MakeXYWH(item->x - item->r, item->y - item->r, item->r * 2, item->r * 2);
	} else if (one->sub == NEMOSHOW_ARC_ITEM) {
		box = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);
	} else if (one->sub == NEMOSHOW_PIE_ITEM) {
		box = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);
	} else if (one->sub == NEMOSHOW_PATH_ITEM) {
		box = NEMOSHOW_ITEM_CC(item, path)->getBounds();
	} else if (one->sub == NEMOSHOW_TEXT_ITEM) {
		if (item->path == NULL) {
			if (item->font->layout == NEMOSHOW_NORMAL_LAYOUT) {
				SkRect bounds[strlen(item->text)];
				int i, count;

				count = NEMOSHOW_ITEM_CC(item, stroke)->getTextWidths(item->text, strlen(item->text), NULL, bounds);

				for (i = 0; i < count; i++) {
					box.join(bounds[i]);
				}
			} else if (item->font->layout == NEMOSHOW_HARFBUZZ_LAYOUT) {
				box = SkRect::MakeXYWH(item->x, item->y, item->textwidth, item->textheight);
			}
		} else {
			box = NEMOSHOW_ITEM_CC(NEMOSHOW_ITEM(item->path), path)->getBounds();

			box.outset(item->fontsize, item->fontsize);
		}
	}

	if (item->stroke != 0)
		box.outset(
				item->stroke_width / 2.0f + 1.0f,
				item->stroke_width / 2.0f + 1.0f);

	if (item->matrix != NULL)
		NEMOSHOW_MATRIX_CC(NEMOSHOW_MATRIX(item->matrix), matrix)->mapRect(&box);
	else if (NEMOSHOW_ITEM_CC(item, matrix) != NULL)
		NEMOSHOW_ITEM_CC(item, matrix)->mapRect(&box);

	one->x = MAX(floor(box.x()), 0);
	one->y = MAX(floor(box.y()), 0);
	one->width = ceil(box.width());
	one->height = ceil(box.height());

	snprintf(attr, NEMOSHOW_SYMBOL_MAX, "%s_x", one->id);
	nemoshow_update_symbol(show, attr, one->x);
	snprintf(attr, NEMOSHOW_SYMBOL_MAX, "%s_y", one->id);
	nemoshow_update_symbol(show, attr, one->y);
	snprintf(attr, NEMOSHOW_SYMBOL_MAX, "%s_w", one->id);
	nemoshow_update_symbol(show, attr, one->width);
	snprintf(attr, NEMOSHOW_SYMBOL_MAX, "%s_h", one->id);
	nemoshow_update_symbol(show, attr, one->height);

	return 0;
}
