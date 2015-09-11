#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showcanvas.h>
#include <showcanvas.hpp>
#include <showitem.h>
#include <showitem.hpp>
#include <showsvg.h>
#include <showsvg.hpp>
#include <showmatrix.h>
#include <showmatrix.hpp>
#include <showpath.h>
#include <showfont.h>
#include <showfont.hpp>
#include <showhelper.hpp>
#include <nemoshow.h>
#include <nemoxml.h>
#include <nemobox.h>
#include <nemomisc.h>

struct showone *nemoshow_canvas_create(void)
{
	struct showcanvas *canvas;
	struct showone *one;

	canvas = (struct showcanvas *)malloc(sizeof(struct showcanvas));
	if (canvas == NULL)
		return NULL;
	memset(canvas, 0, sizeof(struct showcanvas));

	canvas->cc = new showcanvas_t;
	NEMOSHOW_CANVAS_CC(canvas, canvas) = NULL;
	NEMOSHOW_CANVAS_CP(canvas, canvas) = NULL;

	canvas->viewport.sx = 1.0f;
	canvas->viewport.sy = 1.0f;

	canvas->needs_redraw = 1;
	canvas->needs_full_redraw = 1;

	canvas->needs_redraw_picker = 1;
	canvas->needs_full_redraw_picker = 1;

	one = &canvas->base;
	one->type = NEMOSHOW_CANVAS_TYPE;
	one->update = nemoshow_canvas_update;
	one->destroy = nemoshow_canvas_destroy;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "type", canvas->type, NEMOSHOW_CANVAS_TYPE_MAX);
	nemoobject_set_reserved(&one->object, "event", &canvas->event, sizeof(uint32_t));
	nemoobject_set_reserved(&one->object, "width", &canvas->width, sizeof(double));
	nemoobject_set_reserved(&one->object, "height", &canvas->height, sizeof(double));

	nemoobject_set_reserved(&one->object, "fill", &canvas->fill, sizeof(uint32_t));
	nemoobject_set_reserved(&one->object, "fill:r", &canvas->fills[2], sizeof(double));
	nemoobject_set_reserved(&one->object, "fill:g", &canvas->fills[1], sizeof(double));
	nemoobject_set_reserved(&one->object, "fill:b", &canvas->fills[0], sizeof(double));
	nemoobject_set_reserved(&one->object, "fill:a", &canvas->fills[3], sizeof(double));

	nemoobject_set_reserved(&one->object, "alpha", &canvas->alpha, sizeof(double));

	return one;
}

void nemoshow_canvas_destroy(struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemoshow_one_finish(one);

	delete static_cast<showcanvas_t *>(canvas->cc);

	free(canvas);
}

static int nemoshow_canvas_compare(const void *a, const void *b)
{
	return strcasecmp((const char *)a, (const char *)b);
}

int nemoshow_canvas_arrange(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	if (strcmp(canvas->type, "back") == 0) {
		nemoshow_canvas_set_type(one, NEMOSHOW_CANVAS_BACK_TYPE);
	} else if (strcmp(canvas->type, "opengl") == 0) {
		nemoshow_canvas_set_type(one, NEMOSHOW_CANVAS_OPENGL_TYPE);
	} else if (strcmp(canvas->type, "pixman") == 0) {
		nemoshow_canvas_set_type(one, NEMOSHOW_CANVAS_PIXMAN_TYPE);
	} else if (strcmp(canvas->type, "vector") == 0) {
		nemoshow_canvas_set_type(one, NEMOSHOW_CANVAS_VECTOR_TYPE);
	}

	nemoshow_canvas_set_event(one, canvas->event);

	return 0;
}

int nemoshow_canvas_set_type(struct showone *one, int type)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	if (type == NEMOSHOW_CANVAS_VECTOR_TYPE) {
		canvas->node = nemotale_node_create_pixman(canvas->width, canvas->height);

		NEMOSHOW_CANVAS_CC(canvas, bitmap) = new SkBitmap;
		NEMOSHOW_CANVAS_CC(canvas, bitmap)->setInfo(
				SkImageInfo::Make(canvas->width, canvas->height, kN32_SkColorType, kPremul_SkAlphaType));
		NEMOSHOW_CANVAS_CC(canvas, bitmap)->setPixels(
				nemotale_node_get_buffer(canvas->node));

		NEMOSHOW_CANVAS_CC(canvas, device) = new SkBitmapDevice(*NEMOSHOW_CANVAS_CC(canvas, bitmap));
		NEMOSHOW_CANVAS_CC(canvas, canvas) = new SkCanvas(NEMOSHOW_CANVAS_CC(canvas, device));

		NEMOSHOW_CANVAS_CC(canvas, damage) = new SkRegion;

		NEMOSHOW_CANVAS_CP(canvas, bitmap) = new SkBitmap;
		NEMOSHOW_CANVAS_CP(canvas, bitmap)->allocPixels(
				SkImageInfo::Make(canvas->width, canvas->height, kN32_SkColorType, kPremul_SkAlphaType));

		NEMOSHOW_CANVAS_CP(canvas, device) = new SkBitmapDevice(*NEMOSHOW_CANVAS_CP(canvas, bitmap));
		NEMOSHOW_CANVAS_CP(canvas, canvas) = new SkCanvas(NEMOSHOW_CANVAS_CP(canvas, device));

		NEMOSHOW_CANVAS_CP(canvas, damage) = new SkRegion;
	} else if (type == NEMOSHOW_CANVAS_PIXMAN_TYPE) {
		canvas->node = nemotale_node_create_pixman(canvas->width, canvas->height);
	} else if (type == NEMOSHOW_CANVAS_OPENGL_TYPE) {
		canvas->node = nemotale_node_create_gl(canvas->width, canvas->height);
	} else if (type == NEMOSHOW_CANVAS_BACK_TYPE) {
		canvas->node = nemotale_node_create_pixman(canvas->width, canvas->height);
		nemotale_node_opaque(canvas->node, 0, 0, canvas->width, canvas->height);
	}

	one->sub = type;

	return 0;
}

void nemoshow_canvas_set_event(struct showone *one, uint32_t event)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	if (event == 0)
		nemotale_node_set_pick_type(canvas->node, NEMOTALE_PICK_NO_TYPE);
	else
		nemotale_node_set_id(canvas->node, event);

	canvas->event = event;
}

int nemoshow_canvas_update(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	return 0;
}

static inline void nemoshow_canvas_render_one(struct nemoshow *show, struct showcanvas *canvas, struct showone *one);

static inline void nemoshow_canvas_render_item(struct nemoshow *show, struct showcanvas *canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showitem *style = NEMOSHOW_ITEM(NEMOSHOW_REF(one, NEMOSHOW_STYLE_REF));

	if (one->sub == NEMOSHOW_RECT_ITEM) {
		SkRect rect = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);

		if (style->fill != 0)
			NEMOSHOW_CANVAS_CC(canvas, canvas)->drawRect(rect, *NEMOSHOW_ITEM_CC(style, fill));
		if (style->stroke != 0)
			NEMOSHOW_CANVAS_CC(canvas, canvas)->drawRect(rect, *NEMOSHOW_ITEM_CC(style, stroke));
	} else if (one->sub == NEMOSHOW_RRECT_ITEM) {
		SkRect rect = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);

		if (style->fill != 0)
			NEMOSHOW_CANVAS_CC(canvas, canvas)->drawRoundRect(rect, item->rx, item->ry, *NEMOSHOW_ITEM_CC(style, fill));
		if (style->stroke != 0)
			NEMOSHOW_CANVAS_CC(canvas, canvas)->drawRoundRect(rect, item->rx, item->ry, *NEMOSHOW_ITEM_CC(style, stroke));
	} else if (one->sub == NEMOSHOW_CIRCLE_ITEM) {
		if (style->fill != 0)
			NEMOSHOW_CANVAS_CC(canvas, canvas)->drawCircle(item->x, item->y, item->r, *NEMOSHOW_ITEM_CC(style, fill));
		if (style->stroke != 0)
			NEMOSHOW_CANVAS_CC(canvas, canvas)->drawCircle(item->x, item->y, item->r, *NEMOSHOW_ITEM_CC(style, stroke));
	} else if (one->sub == NEMOSHOW_ARC_ITEM) {
		SkRect rect = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);

		if (style->fill != 0)
			NEMOSHOW_CANVAS_CC(canvas, canvas)->drawArc(rect, item->from, item->to - item->from, false, *NEMOSHOW_ITEM_CC(style, fill));
		if (style->stroke != 0)
			NEMOSHOW_CANVAS_CC(canvas, canvas)->drawArc(rect, item->from, item->to - item->from, false, *NEMOSHOW_ITEM_CC(style, stroke));
	} else if (one->sub == NEMOSHOW_PIE_ITEM) {
		SkRect rect = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);

		if (style->fill != 0)
			NEMOSHOW_CANVAS_CC(canvas, canvas)->drawArc(rect, item->from, item->to - item->from, true, *NEMOSHOW_ITEM_CC(style, fill));
		if (style->stroke != 0)
			NEMOSHOW_CANVAS_CC(canvas, canvas)->drawArc(rect, item->from, item->to - item->from, true, *NEMOSHOW_ITEM_CC(style, stroke));
	} else if (one->sub == NEMOSHOW_DONUT_ITEM) {
		if (style->fill != 0)
			NEMOSHOW_CANVAS_CC(canvas, canvas)->drawPath(*NEMOSHOW_ITEM_CC(item, path), *NEMOSHOW_ITEM_CC(style, fill));
		if (style->stroke != 0)
			NEMOSHOW_CANVAS_CC(canvas, canvas)->drawPath(*NEMOSHOW_ITEM_CC(item, path), *NEMOSHOW_ITEM_CC(style, stroke));
	} else if (one->sub == NEMOSHOW_RING_ITEM) {
		if (style->fill != 0) {
			NEMOSHOW_CANVAS_CC(canvas, canvas)->drawCircle(item->x, item->y, item->inner, *NEMOSHOW_ITEM_CC(style, fill));
		}
		if (style->stroke != 0) {
			NEMOSHOW_CANVAS_CC(canvas, canvas)->drawPath(*NEMOSHOW_ITEM_CC(item, path), *NEMOSHOW_ITEM_CC(style, stroke));
		}
	} else if (one->sub == NEMOSHOW_PATH_ITEM || one->sub == NEMOSHOW_PATHGROUP_ITEM) {
		if (item->from == 0.0f && item->to == 1.0f) {
			if (style->fill != 0)
				NEMOSHOW_CANVAS_CC(canvas, canvas)->drawPath(*NEMOSHOW_ITEM_CC(item, path), *NEMOSHOW_ITEM_CC(style, fill));
			if (style->stroke != 0)
				NEMOSHOW_CANVAS_CC(canvas, canvas)->drawPath(*NEMOSHOW_ITEM_CC(item, path), *NEMOSHOW_ITEM_CC(style, stroke));
		} else {
			SkPath path;

			nemoshow_helper_draw_path(
					path,
					NEMOSHOW_ITEM_CC(item, path),
					NEMOSHOW_ITEM_CC(style, fill),
					item->pathlength,
					item->from, item->to);

			if (style->fill != 0)
				NEMOSHOW_CANVAS_CC(canvas, canvas)->drawPath(path, *NEMOSHOW_ITEM_CC(style, fill));
			if (style->stroke != 0)
				NEMOSHOW_CANVAS_CC(canvas, canvas)->drawPath(path, *NEMOSHOW_ITEM_CC(style, stroke));
		}
	} else if (one->sub == NEMOSHOW_TEXT_ITEM) {
		if (NEMOSHOW_REF(one, NEMOSHOW_PATH_REF) == NULL) {
			if (NEMOSHOW_FONT_AT(NEMOSHOW_REF(one, NEMOSHOW_FONT_REF), layout) == NEMOSHOW_NORMAL_LAYOUT) {
				NEMOSHOW_CANVAS_CC(canvas, canvas)->save();
				NEMOSHOW_CANVAS_CC(canvas, canvas)->translate(0.0f, -item->fontascent);

				if (style->fill != 0)
					NEMOSHOW_CANVAS_CC(canvas, canvas)->drawText(
							item->text,
							strlen(item->text),
							item->x,
							item->y,
							*NEMOSHOW_ITEM_CC(style, fill));
				if (style->stroke != 0)
					NEMOSHOW_CANVAS_CC(canvas, canvas)->drawText(
							item->text,
							strlen(item->text),
							item->x,
							item->y,
							*NEMOSHOW_ITEM_CC(style, stroke));

				NEMOSHOW_CANVAS_CC(canvas, canvas)->restore();
			} else if (NEMOSHOW_FONT_AT(NEMOSHOW_REF(one, NEMOSHOW_FONT_REF), layout) == NEMOSHOW_HARFBUZZ_LAYOUT) {
				if (style->fill != 0)
					NEMOSHOW_CANVAS_CC(canvas, canvas)->drawPosText(
							item->text,
							strlen(item->text),
							NEMOSHOW_ITEM_CC(item, points),
							*NEMOSHOW_ITEM_CC(style, fill));
				if (style->stroke != 0)
					NEMOSHOW_CANVAS_CC(canvas, canvas)->drawPosText(
							item->text,
							strlen(item->text),
							NEMOSHOW_ITEM_CC(item, points),
							*NEMOSHOW_ITEM_CC(style, stroke));
			}
		} else {
			if (style->fill != 0)
				NEMOSHOW_CANVAS_CC(canvas, canvas)->drawTextOnPath(
						item->text,
						strlen(item->text),
						*NEMOSHOW_ITEM_CC(NEMOSHOW_ITEM(NEMOSHOW_REF(one, NEMOSHOW_PATH_REF)), path),
						NULL,
						*NEMOSHOW_ITEM_CC(style, fill));
			if (style->stroke != 0)
				NEMOSHOW_CANVAS_CC(canvas, canvas)->drawTextOnPath(
						item->text,
						strlen(item->text),
						*NEMOSHOW_ITEM_CC(NEMOSHOW_ITEM(NEMOSHOW_REF(one, NEMOSHOW_PATH_REF)), path),
						NULL,
						*NEMOSHOW_ITEM_CC(style, stroke));
		}
	} else if (one->sub == NEMOSHOW_IMAGE_ITEM) {
		SkRect rect = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);

		NEMOSHOW_CANVAS_CC(canvas, canvas)->drawBitmapRect(*NEMOSHOW_ITEM_CC(item, bitmap), rect);
	} else if (one->sub == NEMOSHOW_GROUP_ITEM) {
		int i;

		for (i = 0; i < one->nchildren; i++) {
			nemoshow_canvas_render_one(show, canvas, one->children[i]);
		}
	}
}

static inline void nemoshow_canvas_render_item_with_clip(struct nemoshow *show, struct showcanvas *canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (NEMOSHOW_REF(one, NEMOSHOW_CLIP_REF) != NULL) {
		NEMOSHOW_CANVAS_CC(canvas, canvas)->save();
		NEMOSHOW_CANVAS_CC(canvas, canvas)->clipPath(*NEMOSHOW_ITEM_CC(NEMOSHOW_ITEM(NEMOSHOW_REF(one, NEMOSHOW_CLIP_REF)), path));

		nemoshow_canvas_render_item(show, canvas, one);

		NEMOSHOW_CANVAS_CC(canvas, canvas)->restore();
	} else {
		nemoshow_canvas_render_item(show, canvas, one);
	}
}

static inline void nemoshow_canvas_render_one(struct nemoshow *show, struct showcanvas *canvas, struct showone *one)
{
	if (one->type == NEMOSHOW_ITEM_TYPE) {
		struct showitem *item = NEMOSHOW_ITEM(one);

		if (item->transform & NEMOSHOW_EXTERN_TRANSFORM) {
			NEMOSHOW_CANVAS_CC(canvas, canvas)->save();
			NEMOSHOW_CANVAS_CC(canvas, canvas)->concat(*NEMOSHOW_MATRIX_CC(NEMOSHOW_MATRIX(NEMOSHOW_REF(one, NEMOSHOW_MATRIX_REF)), matrix));

			nemoshow_canvas_render_item_with_clip(show, canvas, one);

			NEMOSHOW_CANVAS_CC(canvas, canvas)->restore();
		} else if (item->transform & NEMOSHOW_INTERN_TRANSFORM) {
			NEMOSHOW_CANVAS_CC(canvas, canvas)->save();
			NEMOSHOW_CANVAS_CC(canvas, canvas)->concat(*NEMOSHOW_ITEM_CC(item, matrix));

			nemoshow_canvas_render_item_with_clip(show, canvas, one);

			NEMOSHOW_CANVAS_CC(canvas, canvas)->restore();
		} else {
			nemoshow_canvas_render_item_with_clip(show, canvas, one);
		}
	} else if (one->type == NEMOSHOW_SVG_TYPE) {
		struct showsvg *svg = NEMOSHOW_SVG(one);
		int i;

		NEMOSHOW_CANVAS_CC(canvas, canvas)->save();

		if (svg->transform & NEMOSHOW_EXTERN_TRANSFORM) {
			NEMOSHOW_CANVAS_CC(canvas, canvas)->concat(*NEMOSHOW_MATRIX_CC(NEMOSHOW_MATRIX(NEMOSHOW_REF(one, NEMOSHOW_MATRIX_REF)), matrix));
		} else if (svg->transform & NEMOSHOW_INTERN_TRANSFORM) {
			NEMOSHOW_CANVAS_CC(canvas, canvas)->concat(*NEMOSHOW_SVG_CC(svg, matrix));
		}

		if (NEMOSHOW_REF(one, NEMOSHOW_CLIP_REF) != NULL)
			NEMOSHOW_CANVAS_CC(canvas, canvas)->clipPath(*NEMOSHOW_ITEM_CC(NEMOSHOW_ITEM(NEMOSHOW_REF(one, NEMOSHOW_CLIP_REF)), path));

		NEMOSHOW_CANVAS_CC(canvas, canvas)->concat(*NEMOSHOW_SVG_CC(NEMOSHOW_SVG(one), viewbox));

		for (i = 0; i < one->nchildren; i++) {
			nemoshow_canvas_render_one(show, canvas, one->children[i]);
		}

		NEMOSHOW_CANVAS_CC(canvas, canvas)->restore();
	}
}

void nemoshow_canvas_render_vector(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);
	int i;

	if (canvas->needs_full_redraw == 0) {
		NEMOSHOW_CANVAS_CC(canvas, canvas)->save();
		NEMOSHOW_CANVAS_CC(canvas, canvas)->clipRegion(*NEMOSHOW_CANVAS_CC(canvas, damage));
		NEMOSHOW_CANVAS_CC(canvas, canvas)->clear(SK_ColorTRANSPARENT);
		NEMOSHOW_CANVAS_CC(canvas, canvas)->scale(canvas->viewport.sx, canvas->viewport.sy);

		for (i = 0; i < one->nchildren; i++) {
			nemoshow_canvas_render_one(show, canvas, one->children[i]);
		}

		NEMOSHOW_CANVAS_CC(canvas, canvas)->restore();
	} else {
		NEMOSHOW_CANVAS_CC(canvas, canvas)->save();
		NEMOSHOW_CANVAS_CC(canvas, canvas)->clear(SK_ColorTRANSPARENT);
		NEMOSHOW_CANVAS_CC(canvas, canvas)->scale(canvas->viewport.sx, canvas->viewport.sy);

		for (i = 0; i < one->nchildren; i++) {
			nemoshow_canvas_render_one(show, canvas, one->children[i]);
		}

		NEMOSHOW_CANVAS_CC(canvas, canvas)->restore();

		canvas->needs_full_redraw = 0;
	}

	NEMOSHOW_CANVAS_CC(canvas, damage)->setEmpty();
}

static inline void nemoshow_canvas_render_picker_one(struct nemoshow *show, struct showcanvas *canvas, struct showone *one)
{
	if (one->type == NEMOSHOW_ITEM_TYPE) {
		struct showitem *item = NEMOSHOW_ITEM(one);

		if (one->sub == NEMOSHOW_GROUP_ITEM) {
			int i;

			for (i = 0; i < one->nchildren; i++) {
				nemoshow_canvas_render_picker_one(show, canvas, one->children[i]);
			}
		} else {
			if (item->event > 0) {
				SkRect rect = SkRect::MakeXYWH(one->x + one->outer, one->y + one->outer, one->width - one->outer * 2, one->height - one->outer * 2);
				SkPaint paint;

				paint.setStyle(SkPaint::kFill_Style);
				paint.setColor(SkColorSetARGB(255, item->event, 0, 0));
				paint.setAntiAlias(false);

				NEMOSHOW_CANVAS_CP(canvas, canvas)->drawRect(rect, paint);
			}
		}
	} else if (one->type == NEMOSHOW_SVG_TYPE) {
		struct showsvg *svg = NEMOSHOW_SVG(one);

		if (svg->event > 0) {
			SkRect rect = SkRect::MakeXYWH(one->x + one->outer, one->y + one->outer, one->width - one->outer * 2, one->height - one->outer * 2);
			SkPaint paint;

			paint.setStyle(SkPaint::kFill_Style);
			paint.setColor(SkColorSetARGB(255, svg->event, 0, 0));
			paint.setAntiAlias(false);

			NEMOSHOW_CANVAS_CP(canvas, canvas)->drawRect(rect, paint);
		}
	}
}

void nemoshow_canvas_render_picker(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);
	struct showone *child;
	int i;

	if (canvas->needs_full_redraw_picker == 0) {
		NEMOSHOW_CANVAS_CP(canvas, canvas)->save();
		NEMOSHOW_CANVAS_CP(canvas, canvas)->clipRegion(*NEMOSHOW_CANVAS_CP(canvas, damage));
		NEMOSHOW_CANVAS_CP(canvas, canvas)->clear(SK_ColorTRANSPARENT);
		NEMOSHOW_CANVAS_CP(canvas, canvas)->scale(canvas->viewport.sx, canvas->viewport.sy);

		for (i = 0; i < one->nchildren; i++) {
			nemoshow_canvas_render_picker_one(show, canvas, one->children[i]);
		}

		NEMOSHOW_CANVAS_CP(canvas, canvas)->restore();
	} else {
		NEMOSHOW_CANVAS_CP(canvas, canvas)->save();
		NEMOSHOW_CANVAS_CP(canvas, canvas)->clear(SK_ColorTRANSPARENT);
		NEMOSHOW_CANVAS_CP(canvas, canvas)->scale(canvas->viewport.sx, canvas->viewport.sy);

		for (i = 0; i < one->nchildren; i++) {
			nemoshow_canvas_render_picker_one(show, canvas, one->children[i]);
		}

		NEMOSHOW_CANVAS_CP(canvas, canvas)->restore();

		canvas->needs_full_redraw_picker = 0;
	}

	NEMOSHOW_CANVAS_CP(canvas, damage)->setEmpty();
}

void nemoshow_canvas_render_back(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_fill_pixman(canvas->node, canvas->fills[2], canvas->fills[1], canvas->fills[0], canvas->fills[3] * canvas->alpha);
}

int nemoshow_canvas_set_viewport(struct nemoshow *show, struct showone *one, double sx, double sy)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	if (one->sub == NEMOSHOW_CANVAS_VECTOR_TYPE) {
		canvas->viewport.sx = sx;
		canvas->viewport.sy = sy;

		canvas->viewport.width = canvas->width * sx;
		canvas->viewport.height = canvas->height * sy;

		delete NEMOSHOW_CANVAS_CC(canvas, canvas);
		delete NEMOSHOW_CANVAS_CC(canvas, device);
		delete NEMOSHOW_CANVAS_CC(canvas, bitmap);

		delete NEMOSHOW_CANVAS_CP(canvas, canvas);
		delete NEMOSHOW_CANVAS_CP(canvas, device);
		delete NEMOSHOW_CANVAS_CP(canvas, bitmap);

		nemotale_node_set_viewport_pixman(canvas->node, canvas->viewport.width, canvas->viewport.height);

		NEMOSHOW_CANVAS_CC(canvas, bitmap) = new SkBitmap;
		NEMOSHOW_CANVAS_CC(canvas, bitmap)->setInfo(
				SkImageInfo::Make(canvas->viewport.width, canvas->viewport.height, kN32_SkColorType, kPremul_SkAlphaType));
		NEMOSHOW_CANVAS_CC(canvas, bitmap)->setPixels(
				nemotale_node_get_buffer(canvas->node));

		NEMOSHOW_CANVAS_CC(canvas, device) = new SkBitmapDevice(*NEMOSHOW_CANVAS_CC(canvas, bitmap));
		NEMOSHOW_CANVAS_CC(canvas, canvas) = new SkCanvas(NEMOSHOW_CANVAS_CC(canvas, device));

		NEMOSHOW_CANVAS_CP(canvas, bitmap) = new SkBitmap;
		NEMOSHOW_CANVAS_CP(canvas, bitmap)->allocPixels(
				SkImageInfo::Make(canvas->viewport.width, canvas->viewport.height, kN32_SkColorType, kPremul_SkAlphaType));

		NEMOSHOW_CANVAS_CP(canvas, device) = new SkBitmapDevice(*NEMOSHOW_CANVAS_CP(canvas, bitmap));
		NEMOSHOW_CANVAS_CP(canvas, canvas) = new SkCanvas(NEMOSHOW_CANVAS_CP(canvas, device));

		nemoshow_canvas_damage_all(one);
	}

	return 0;
}

void nemoshow_canvas_damage_region(struct showone *one, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	NEMOSHOW_CANVAS_CC(canvas, damage)->op(
			SkIRect::MakeXYWH(
				x * canvas->viewport.sx,
				y * canvas->viewport.sy,
				width * canvas->viewport.sx,
				height * canvas->viewport.sy),
			SkRegion::kUnion_Op);

	NEMOSHOW_CANVAS_CP(canvas, damage)->op(
			SkIRect::MakeXYWH(
				x * canvas->viewport.sx,
				y * canvas->viewport.sy,
				width * canvas->viewport.sx,
				height * canvas->viewport.sy),
			SkRegion::kUnion_Op);

	nemotale_node_damage(canvas->node, x, y, width, height);

	canvas->needs_redraw = 1;
	canvas->needs_redraw_picker = 1;
}

void nemoshow_canvas_damage_one(struct showone *one, struct showone *child)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	NEMOSHOW_CANVAS_CC(canvas, damage)->op(
			SkIRect::MakeXYWH(
				child->x * canvas->viewport.sx,
				child->y * canvas->viewport.sy,
				child->width * canvas->viewport.sx,
				child->height * canvas->viewport.sy),
			SkRegion::kUnion_Op);

	if (NEMOSHOW_CANVAS_CP(canvas, canvas) != NULL) {
		NEMOSHOW_CANVAS_CP(canvas, damage)->op(
				SkIRect::MakeXYWH(
					child->x * canvas->viewport.sx,
					child->y * canvas->viewport.sy,
					child->width * canvas->viewport.sx,
					child->height * canvas->viewport.sy),
				SkRegion::kUnion_Op);
	}

	nemotale_node_damage(canvas->node, child->x, child->y, child->width, child->height);

	canvas->needs_redraw = 1;
	canvas->needs_redraw_picker = 1;
}

void nemoshow_canvas_damage_all(struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_damage_all(canvas->node);

	canvas->needs_redraw = 1;
	canvas->needs_full_redraw = 1;

	canvas->needs_redraw_picker = 1;
	canvas->needs_full_redraw_picker = 1;
}

void nemoshow_canvas_translate(struct showone *one, float x, float y)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_translate(canvas->node, x, y);
}

void nemoshow_canvas_rotate(struct showone *one, float r)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_rotate(canvas->node, r);
}

void nemoshow_canvas_pivot(struct showone *one, float px, float py)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_pivot(canvas->node, px, py);
}

int32_t nemoshow_canvas_pick_one(struct showone *one, int x, int y)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);
	double px = x * canvas->viewport.sx;
	double py = y * canvas->viewport.sy;

	if (px < 0 || py < 0 || px >= canvas->viewport.width || py >= canvas->viewport.height)
		return 0;

	return SkColorGetR(NEMOSHOW_CANVAS_CP(canvas, bitmap)->getColor(px, py));
}
