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
#include <showblur.h>
#include <showblur.hpp>
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
	NEMOSHOW_ITEM_CC(item, path) = NULL;
	NEMOSHOW_ITEM_CC(item, points) = NULL;

	item->alpha = 1.0f;
	item->from = 0.0f;
	item->to = 1.0f;

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
	struct showone *blur;
	struct showone *shader;
	struct showone *matrix;
	struct showone *path;
	struct showone *font;
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

		blur = nemoshow_search_one(show, nemoobject_gets(&one->object, "blur"));
		if (blur != NULL) {
			item->blur = blur;

			NEMOBOX_APPEND(blur->refs, blur->srefs, blur->nrefs, one);
		}

		shader = nemoshow_search_one(show, nemoobject_gets(&one->object, "shader"));
		if (shader != NULL) {
			item->shader = shader;

			NEMOBOX_APPEND(shader->refs, shader->srefs, shader->nrefs, one);
		}

		item->style = one;
	}

	matrix = nemoshow_search_one(show, nemoobject_gets(&one->object, "matrix"));
	if (matrix != NULL) {
		item->transform = NEMOSHOW_EXTERN_TRANSFORM;

		item->matrix = matrix;

		NEMOBOX_APPEND(matrix->refs, matrix->srefs, matrix->nrefs, one);
	} else {
		for (i = 0; i < one->nchildren; i++) {
			if (one->children[i]->type == NEMOSHOW_MATRIX_TYPE) {
				item->transform = NEMOSHOW_INTERN_TRANSFORM;

				break;
			}
		}
	}

	path = nemoshow_search_one(show, nemoobject_gets(&one->object, "path"));
	if (path != NULL) {
		item->path = path;

		NEMOBOX_APPEND(path->refs, path->srefs, path->nrefs, one);
	}

	font = nemoshow_search_one(show, nemoobject_gets(&one->object, "font"));
	if (font != NULL) {
		item->font = font;

		SkSafeUnref(
				NEMOSHOW_ITEM_CC(item, fill)->setTypeface(
					NEMOSHOW_FONT_CC(NEMOSHOW_FONT(item->font), face)));
		SkSafeUnref(
				NEMOSHOW_ITEM_CC(item, stroke)->setTypeface(
					NEMOSHOW_FONT_CC(NEMOSHOW_FONT(item->font), face)));

		NEMOBOX_APPEND(font->refs, font->srefs, font->nrefs, one);
	}

	if (one->sub == NEMOSHOW_PATH_ITEM || one->sub == NEMOSHOW_PATHGROUP_ITEM) {
		NEMOSHOW_ITEM_CC(item, path) = new SkPath;
	}

	item->canvas = nemoshow_one_get_canvas(one);
	item->group = nemoshow_one_get_parent(one, NEMOSHOW_GROUP_ITEM);

	return 0;
}

void nemoshow_item_update_style(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

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

	if (item->blur != NULL) {
		if (item->fill != 0 && item->stroke == 0)
			NEMOSHOW_ITEM_CC(item, fill)->setMaskFilter(NEMOSHOW_BLUR_CC(NEMOSHOW_BLUR(item->blur), filter));
		else
			NEMOSHOW_ITEM_CC(item, stroke)->setMaskFilter(NEMOSHOW_BLUR_CC(NEMOSHOW_BLUR(item->blur), filter));
	}

	if (item->shader != NULL) {
		if (item->fill != 0)
			NEMOSHOW_ITEM_CC(item, fill)->setShader(NEMOSHOW_SHADER_CC(NEMOSHOW_SHADER(item->shader), shader));
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
			} else if (child->sub == NEMOSHOW_RECT_PATH) {
				NEMOSHOW_ITEM_CC(item, path)->addRect(
						path->x0, path->y0,
						path->x1, path->y1);
			} else if (child->sub == NEMOSHOW_CIRCLE_PATH) {
				NEMOSHOW_ITEM_CC(item, path)->addCircle(
						path->x0, path->y0,
						path->r);
			}
		}
	}
}

void nemoshow_item_update_child(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showone *child;
	int i;

	if (item->transform == NEMOSHOW_INTERN_TRANSFORM) {
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

		item->length = nemoshow_helper_get_path_length(NEMOSHOW_ITEM_CC(item, path));

		one->dirty |= NEMOSHOW_SHAPE_DIRTY;
	}
}

void nemoshow_item_update_shape(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (one->sub == NEMOSHOW_PATH_ITEM) {
		item->length = nemoshow_helper_get_path_length(NEMOSHOW_ITEM_CC(item, path));
	} else if (one->sub == NEMOSHOW_TEXT_ITEM && (one->dirty & NEMOSHOW_TEXT_DIRTY) != 0) {
		item->text = nemoobject_gets(&one->object, "d");
		if (item->text != NULL) {
			if (NEMOSHOW_FONT_AT(item->font, layout) == NEMOSHOW_NORMAL_LAYOUT) {
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

				NEMOSHOW_ITEM_CC(item, fill)->setTextSize((NEMOSHOW_FONT_AT(item->font, upem) / NEMOSHOW_FONT_AT(item->font, max_advance_height)) * item->fontsize);
				NEMOSHOW_ITEM_CC(item, stroke)->setTextSize((NEMOSHOW_FONT_AT(item->font, upem) / NEMOSHOW_FONT_AT(item->font, max_advance_height)) * item->fontsize);

				NEMOSHOW_ITEM_CC(item, stroke)->getFontMetrics(&metrics, 0);
				item->fontascent = metrics.fAscent;
				item->fontdescent = metrics.fDescent;

				hbbuffer = hb_buffer_create();
				hb_buffer_add_utf8(hbbuffer, item->text, strlen(item->text), 0, strlen(item->text));
				hb_buffer_set_direction(hbbuffer, HB_DIRECTION_LTR);
				hb_buffer_set_script(hbbuffer, HB_SCRIPT_LATIN);
				hb_buffer_set_language(hbbuffer, HB_LANGUAGE_INVALID);
				hb_shape_full(NEMOSHOW_FONT_AT(item->font, hbfont), hbbuffer, NULL, 0, NULL);

				hbglyphs = hb_buffer_get_glyph_infos(hbbuffer, &nhbglyphs);
				hbglyphspos = hb_buffer_get_glyph_positions(hbbuffer, NULL);

				fontscale = item->fontsize / NEMOSHOW_FONT_AT(item->font, max_advance_height);

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

void nemoshow_item_update_boundingbox(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showone *group;
	SkRect box;
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
	} else if (one->sub == NEMOSHOW_PATH_ITEM || one->sub == NEMOSHOW_PATHGROUP_ITEM) {
		box = NEMOSHOW_ITEM_CC(item, path)->getBounds();
	} else if (one->sub == NEMOSHOW_TEXT_ITEM) {
		if (item->path == NULL) {
			if (NEMOSHOW_FONT_AT(item->font, layout) == NEMOSHOW_NORMAL_LAYOUT) {
				SkRect bounds[strlen(item->text)];
				int i, count;

				count = NEMOSHOW_ITEM_CC(item, stroke)->getTextWidths(item->text, strlen(item->text), NULL, bounds);

				for (i = 0; i < count; i++) {
					box.join(bounds[i]);
				}
			} else if (NEMOSHOW_FONT_AT(item->font, layout) == NEMOSHOW_HARFBUZZ_LAYOUT) {
				box = SkRect::MakeXYWH(item->x, item->y, item->textwidth, item->textheight);
			}
		} else {
			box = NEMOSHOW_ITEM_CC(NEMOSHOW_ITEM(item->path), path)->getBounds();

			box.outset(item->fontsize, item->fontsize);
		}
	} else {
		box = SkRect::MakeXYWH(0, 0, 0, 0);
	}

	if (item->stroke != 0)
		box.outset(item->stroke_width, item->stroke_width);

	if (item->transform == NEMOSHOW_EXTERN_TRANSFORM) {
		NEMOSHOW_MATRIX_CC(NEMOSHOW_MATRIX(item->matrix), matrix)->mapRect(&box);
	} else if (item->transform == NEMOSHOW_INTERN_TRANSFORM || item->transform == NEMOSHOW_DIRECT_TRANSFORM) {
		NEMOSHOW_ITEM_CC(item, matrix)->mapRect(&box);
	}

	for (group = item->group; group != NULL; group = NEMOSHOW_ITEM_AT(group, group)) {
		struct showitem *pitem = NEMOSHOW_ITEM(group);

		nemoshow_one_update_alone(show, group);

		if (pitem->transform == NEMOSHOW_EXTERN_TRANSFORM) {
			NEMOSHOW_MATRIX_CC(NEMOSHOW_MATRIX(pitem->matrix), matrix)->mapRect(&box);
		} else if (pitem->transform == NEMOSHOW_INTERN_TRANSFORM || pitem->transform == NEMOSHOW_DIRECT_TRANSFORM) {
			NEMOSHOW_ITEM_CC(pitem, matrix)->mapRect(&box);
		}
	}

	outer = NEMOSHOW_ANTIALIAS_EPSILON;
	if (item->blur != NULL)
		outer += NEMOSHOW_BLUR_AT(item->blur, r);
	box.outset(outer, outer);

	if (item->canvas != NULL) {
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
	}
}

int nemoshow_item_update(struct nemoshow *show, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if ((one->dirty & NEMOSHOW_STYLE_DIRTY) != 0)
		nemoshow_item_update_style(show, one);
	if ((one->dirty & NEMOSHOW_CHILD_DIRTY) != 0 && one->nchildren > 0)
		nemoshow_item_update_child(show, one);

	if ((one->dirty & NEMOSHOW_SHAPE_DIRTY) != 0) {
		nemoshow_canvas_damage_one(item->canvas, one);

		nemoshow_item_update_shape(show, one);

		nemoshow_item_update_boundingbox(show, one);
	}

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

void nemoshow_item_set_shader(struct showone *one, struct showone *shader)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->shader = shader;

	item->fill = 1;

	NEMOBOX_APPEND(shader->refs, shader->srefs, shader->nrefs, one);
}

double nemoshow_item_get_outer(struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	double outer = 0.0f;

	if (item->stroke != 0)
		outer += item->stroke_width;

	outer += NEMOSHOW_ANTIALIAS_EPSILON;
	if (item->blur != NULL)
		outer += NEMOSHOW_BLUR_AT(item->blur, r);

	return outer;
}
