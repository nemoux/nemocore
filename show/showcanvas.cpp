#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <skiaconfig.hpp>

#include <showcanvas.h>
#include <showcanvas.hpp>
#include <showitem.h>
#include <showitem.hpp>
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
	NEMOSHOW_CANVAS_CC(canvas, bitmap) = NULL;
	NEMOSHOW_CANVAS_CC(canvas, device) = NULL;
	NEMOSHOW_CANVAS_CC(canvas, damage) = NULL;

	canvas->viewport.sx = 1.0f;
	canvas->viewport.sy = 1.0f;

	canvas->sx = 1.0f;
	canvas->sy = 1.0f;
	canvas->px = 0.5f;
	canvas->py = 0.5f;

	canvas->alpha = 1.0f;

	canvas->needs_redraw = 1;
	canvas->needs_full_redraw = 1;

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

	nemoobject_set_reserved(&one->object, "tx", &canvas->tx, sizeof(double));
	nemoobject_set_reserved(&one->object, "ty", &canvas->ty, sizeof(double));
	nemoobject_set_reserved(&one->object, "ro", &canvas->ro, sizeof(double));
	nemoobject_set_reserved(&one->object, "px", &canvas->px, sizeof(double));
	nemoobject_set_reserved(&one->object, "py", &canvas->py, sizeof(double));
	nemoobject_set_reserved(&one->object, "sx", &canvas->sx, sizeof(double));
	nemoobject_set_reserved(&one->object, "sy", &canvas->sy, sizeof(double));

	nemoobject_set_reserved(&one->object, "alpha", &canvas->alpha, sizeof(double));

	return one;
}

void nemoshow_canvas_destroy(struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	if (canvas->show != NULL) {
		nemoshow_detach_canvas(canvas->show, one);
	}

	nemotale_node_destroy(canvas->node);

	nemoshow_one_finish(one);

	if (NEMOSHOW_CANVAS_CC(canvas, damage) != NULL)
		delete NEMOSHOW_CANVAS_CC(canvas, damage);
	if (NEMOSHOW_CANVAS_CC(canvas, canvas) != NULL)
		delete NEMOSHOW_CANVAS_CC(canvas, canvas);
	if (NEMOSHOW_CANVAS_CC(canvas, bitmap) != NULL)
		delete NEMOSHOW_CANVAS_CC(canvas, bitmap);
	if (NEMOSHOW_CANVAS_CC(canvas, device) != NULL)
		delete NEMOSHOW_CANVAS_CC(canvas, device);

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

void nemoshow_canvas_set_alpha(struct showone *one, double alpha)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_set_alpha(canvas->node, alpha);

	canvas->alpha = alpha;
}

int nemoshow_canvas_attach_pixman(struct showone *one, void *data, int32_t width, int32_t height)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	return nemotale_node_attach_pixman(canvas->node, data, width, height);
}

void nemoshow_canvas_detach_pixman(struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_detach_pixman(canvas->node);
}

int nemoshow_canvas_resize_pixman(struct showone *one, int32_t width, int32_t height)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	return nemotale_node_resize_pixman(canvas->node, width, height);
}

static inline void nemoshow_canvas_update_style(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_set_alpha(canvas->node, canvas->alpha);

	nemotale_node_damage_all(canvas->node);
}

static inline void nemoshow_canvas_update_matrix(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_translate(canvas->node, canvas->tx, canvas->ty);
	nemotale_node_rotate(canvas->node, canvas->ro * M_PI / 180.0f);
	nemotale_node_scale(canvas->node, canvas->sx, canvas->sy);
	nemotale_node_pivot(canvas->node, canvas->px, canvas->py);

	nemotale_node_damage_all(canvas->node);
}

int nemoshow_canvas_update(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	if ((one->dirty & NEMOSHOW_STYLE_DIRTY) != 0)
		nemoshow_canvas_update_style(show, one);
	if ((one->dirty & NEMOSHOW_MATRIX_DIRTY) != 0)
		nemoshow_canvas_update_matrix(show, one);

	return 0;
}

static inline void nemoshow_canvas_render_one(SkCanvas *canvas, struct showone *one);

static inline void nemoshow_canvas_render_item_none(SkCanvas *canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showitem *style = NEMOSHOW_ITEM(nemoshow_item_get_style(one));
}

static inline void nemoshow_canvas_render_item_rect(SkCanvas *canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showitem *style = NEMOSHOW_ITEM(nemoshow_item_get_style(one));
	SkRect rect = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);

	if (style->fill != 0)
		canvas->drawRect(rect, *NEMOSHOW_ITEM_CC(style, fill));
	if (style->stroke != 0)
		canvas->drawRect(rect, *NEMOSHOW_ITEM_CC(style, stroke));
}

static inline void nemoshow_canvas_render_item_rrect(SkCanvas *canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showitem *style = NEMOSHOW_ITEM(nemoshow_item_get_style(one));
	SkRect rect = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);

	if (style->fill != 0)
		canvas->drawRoundRect(rect, item->rx, item->ry, *NEMOSHOW_ITEM_CC(style, fill));
	if (style->stroke != 0)
		canvas->drawRoundRect(rect, item->rx, item->ry, *NEMOSHOW_ITEM_CC(style, stroke));
}

static inline void nemoshow_canvas_render_item_circle(SkCanvas *canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showitem *style = NEMOSHOW_ITEM(nemoshow_item_get_style(one));

	if (style->fill != 0)
		canvas->drawCircle(item->x, item->y, item->r, *NEMOSHOW_ITEM_CC(style, fill));
	if (style->stroke != 0)
		canvas->drawCircle(item->x, item->y, item->r, *NEMOSHOW_ITEM_CC(style, stroke));
}

static inline void nemoshow_canvas_render_item_arc(SkCanvas *canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showitem *style = NEMOSHOW_ITEM(nemoshow_item_get_style(one));
	SkRect rect = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);

	if (style->fill != 0)
		canvas->drawArc(rect, item->from, item->to - item->from, false, *NEMOSHOW_ITEM_CC(style, fill));
	if (style->stroke != 0)
		canvas->drawArc(rect, item->from, item->to - item->from, false, *NEMOSHOW_ITEM_CC(style, stroke));
}

static inline void nemoshow_canvas_render_item_pie(SkCanvas *canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showitem *style = NEMOSHOW_ITEM(nemoshow_item_get_style(one));
	SkRect rect = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);

	if (style->fill != 0)
		canvas->drawArc(rect, item->from, item->to - item->from, true, *NEMOSHOW_ITEM_CC(style, fill));
	if (style->stroke != 0)
		canvas->drawArc(rect, item->from, item->to - item->from, true, *NEMOSHOW_ITEM_CC(style, stroke));
}

static inline void nemoshow_canvas_render_item_donut(SkCanvas *canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showitem *style = NEMOSHOW_ITEM(nemoshow_item_get_style(one));

	if (style->fill != 0)
		canvas->drawPath(*NEMOSHOW_ITEM_CC(item, path), *NEMOSHOW_ITEM_CC(style, fill));
	if (style->stroke != 0)
		canvas->drawPath(*NEMOSHOW_ITEM_CC(item, path), *NEMOSHOW_ITEM_CC(style, stroke));
}

static inline void nemoshow_canvas_render_item_ring(SkCanvas *canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showitem *style = NEMOSHOW_ITEM(nemoshow_item_get_style(one));

	if (style->fill != 0) {
		canvas->drawCircle(item->x, item->y, item->inner, *NEMOSHOW_ITEM_CC(style, fill));
	}
	if (style->stroke != 0) {
		canvas->drawPath(*NEMOSHOW_ITEM_CC(item, path), *NEMOSHOW_ITEM_CC(style, stroke));
	}
}

static inline void nemoshow_canvas_render_item_path(SkCanvas *canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showitem *style = NEMOSHOW_ITEM(nemoshow_item_get_style(one));

	if (item->from == 0.0f && item->to == 1.0f) {
		if (NEMOSHOW_ITEM_CC(item, fillpath) != NULL && NEMOSHOW_ITEM_CC(item, strokepath) != NULL) {
			if (style->fill != 0)
				canvas->drawPath(*NEMOSHOW_ITEM_CC(item, fillpath), *NEMOSHOW_ITEM_CC(style, fill));
			if (style->stroke != 0)
				canvas->drawPath(*NEMOSHOW_ITEM_CC(item, strokepath), *NEMOSHOW_ITEM_CC(style, stroke));
		} else {
			if (style->fill != 0)
				canvas->drawPath(*NEMOSHOW_ITEM_CC(item, path), *NEMOSHOW_ITEM_CC(style, fill));
			if (style->stroke != 0)
				canvas->drawPath(*NEMOSHOW_ITEM_CC(item, path), *NEMOSHOW_ITEM_CC(style, stroke));
		}
	} else {
		SkPath path;

		nemoshow_helper_draw_path(
				path,
				NEMOSHOW_ITEM_CC(item, path),
				NEMOSHOW_ITEM_CC(style, fill),
				item->pathlength,
				item->from, item->to);

		canvas->drawPath(path, *NEMOSHOW_ITEM_CC(style, stroke));
	}
}

static inline void nemoshow_canvas_render_item_path_group(SkCanvas *canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showitem *style = NEMOSHOW_ITEM(nemoshow_item_get_style(one));

	if (item->from == 0.0f && item->to == 1.0f) {
		if (style->fill != 0)
			canvas->drawPath(*NEMOSHOW_ITEM_CC(item, path), *NEMOSHOW_ITEM_CC(style, fill));
		if (style->stroke != 0)
			canvas->drawPath(*NEMOSHOW_ITEM_CC(item, path), *NEMOSHOW_ITEM_CC(style, stroke));
	} else {
		SkPath path;

		nemoshow_helper_draw_path(
				path,
				NEMOSHOW_ITEM_CC(item, path),
				NEMOSHOW_ITEM_CC(style, fill),
				item->pathlength,
				item->from, item->to);

		if (style->fill != 0)
			canvas->drawPath(path, *NEMOSHOW_ITEM_CC(style, fill));
		if (style->stroke != 0)
			canvas->drawPath(path, *NEMOSHOW_ITEM_CC(style, stroke));
	}
}

static inline void nemoshow_canvas_render_item_text(SkCanvas *canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showitem *style = NEMOSHOW_ITEM(nemoshow_item_get_style(one));

	if (NEMOSHOW_REF(one, NEMOSHOW_PATH_REF) == NULL) {
		if (NEMOSHOW_FONT_AT(NEMOSHOW_REF(one, NEMOSHOW_FONT_REF), layout) == NEMOSHOW_NORMAL_LAYOUT) {
			canvas->save();
			canvas->translate(0.0f, -item->fontascent);

			if (style->fill != 0)
				canvas->drawText(
						item->text,
						strlen(item->text),
						item->x,
						item->y,
						*NEMOSHOW_ITEM_CC(style, fill));
			if (style->stroke != 0)
				canvas->drawText(
						item->text,
						strlen(item->text),
						item->x,
						item->y,
						*NEMOSHOW_ITEM_CC(style, stroke));

			canvas->restore();
		} else if (NEMOSHOW_FONT_AT(NEMOSHOW_REF(one, NEMOSHOW_FONT_REF), layout) == NEMOSHOW_HARFBUZZ_LAYOUT) {
			if (style->fill != 0)
				canvas->drawPosText(
						item->text,
						strlen(item->text),
						NEMOSHOW_ITEM_CC(item, points),
						*NEMOSHOW_ITEM_CC(style, fill));
			if (style->stroke != 0)
				canvas->drawPosText(
						item->text,
						strlen(item->text),
						NEMOSHOW_ITEM_CC(item, points),
						*NEMOSHOW_ITEM_CC(style, stroke));
		}
	} else {
		if (style->fill != 0)
			canvas->drawTextOnPath(
					item->text,
					strlen(item->text),
					*NEMOSHOW_ITEM_CC(NEMOSHOW_ITEM(NEMOSHOW_REF(one, NEMOSHOW_PATH_REF)), path),
					NULL,
					*NEMOSHOW_ITEM_CC(style, fill));
		if (style->stroke != 0)
			canvas->drawTextOnPath(
					item->text,
					strlen(item->text),
					*NEMOSHOW_ITEM_CC(NEMOSHOW_ITEM(NEMOSHOW_REF(one, NEMOSHOW_PATH_REF)), path),
					NULL,
					*NEMOSHOW_ITEM_CC(style, stroke));
	}
}

static inline void nemoshow_canvas_render_vector_on_bitmap(SkBitmap *bitmap, struct showone *one)
{
	struct showitem *src = NEMOSHOW_ITEM(one);
	int i;

	SkBitmapDevice device(*bitmap);
	SkCanvas canvas(&device);

	canvas.clear(SK_ColorTRANSPARENT);
	canvas.scale(bitmap->width() / src->width, bitmap->height() / src->height);
	canvas.concat(*NEMOSHOW_ITEM_CC(src, viewbox));

	for (i = 0; i < one->nchildren; i++) {
		nemoshow_canvas_render_one(&canvas, one->children[i]);
	}
}

static inline void nemoshow_canvas_render_item_bitmap(SkCanvas *canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showitem *style = NEMOSHOW_ITEM(nemoshow_item_get_style(one));
	SkRect rect = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);

	if (nemoshow_one_has_state(one, NEMOSHOW_REDRAW_STATE)) {
		nemoshow_canvas_render_vector_on_bitmap(NEMOSHOW_ITEM_CC(item, bitmap), NEMOSHOW_REF(one, NEMOSHOW_SRC_REF));

		nemoshow_one_put_state(one, NEMOSHOW_REDRAW_STATE);
	}

	canvas->drawBitmapRect(*NEMOSHOW_ITEM_CC(item, bitmap), rect);
}

static inline void nemoshow_canvas_render_item_image(SkCanvas *canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showitem *style = NEMOSHOW_ITEM(nemoshow_item_get_style(one));
	SkRect rect = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);

	canvas->drawBitmapRect(*NEMOSHOW_ITEM_CC(item, bitmap), rect);
}

static inline void nemoshow_canvas_render_item_svg(SkCanvas *canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	int i;

	canvas->save();
	canvas->concat(*NEMOSHOW_ITEM_CC(item, viewbox));

	for (i = 0; i < one->nchildren; i++) {
		nemoshow_canvas_render_one(canvas, one->children[i]);
	}

	canvas->restore();
}

static inline void nemoshow_canvas_render_item_group(SkCanvas *canvas, struct showone *one)
{
	int i;

	for (i = 0; i < one->nchildren; i++) {
		nemoshow_canvas_render_one(canvas, one->children[i]);
	}
}

typedef void (*nemoshow_canvas_render_t)(SkCanvas *canvas, struct showone *one);

static inline void nemoshow_canvas_render_item(SkCanvas *canvas, struct showone *one)
{
	static nemoshow_canvas_render_t renderers[NEMOSHOW_LAST_ITEM] = {
		nemoshow_canvas_render_item_none,
		nemoshow_canvas_render_item_rect,
		nemoshow_canvas_render_item_rrect,
		nemoshow_canvas_render_item_circle,
		nemoshow_canvas_render_item_arc,
		nemoshow_canvas_render_item_pie,
		nemoshow_canvas_render_item_donut,
		nemoshow_canvas_render_item_ring,
		nemoshow_canvas_render_item_text,
		nemoshow_canvas_render_item_path,
		nemoshow_canvas_render_item_path_group,
		nemoshow_canvas_render_item_bitmap,
		nemoshow_canvas_render_item_image,
		nemoshow_canvas_render_item_svg,
		nemoshow_canvas_render_item_group
	};
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (item->has_anchor != 0) {
		canvas->save();
		canvas->translate(-item->width * item->ax, -item->height * item->ay);

		renderers[one->sub](canvas, one);

		canvas->restore();
	} else {
		renderers[one->sub](canvas, one);
	}
}

static inline void nemoshow_canvas_render_one(SkCanvas *canvas, struct showone *one)
{
	if (one->type == NEMOSHOW_ITEM_TYPE) {
		struct showitem *item = NEMOSHOW_ITEM(one);
		struct showitem *clip = NEMOSHOW_REF(one, NEMOSHOW_CLIP_REF) == NULL ? NULL : NEMOSHOW_ITEM(NEMOSHOW_REF(one, NEMOSHOW_CLIP_REF));
		int has_context = item->transform != 0 || clip != NULL;

		if (has_context != 0)
			canvas->save();

		if (item->transform != 0)
			canvas->concat(*NEMOSHOW_ITEM_CC(item, modelview));

		if (clip != NULL) {
			if (clip->transform == 0) {
				canvas->clipPath(*NEMOSHOW_ITEM_CC(clip, path));
			} else {
				SkMatrix matrix = canvas->getTotalMatrix();

				canvas->resetMatrix();
				canvas->scale(
						nemoshow_canvas_get_viewport_sx(item->canvas),
						nemoshow_canvas_get_viewport_sy(item->canvas));
				canvas->concat(*NEMOSHOW_ITEM_CC(clip, modelview));

				canvas->clipPath(*NEMOSHOW_ITEM_CC(clip, path));

				canvas->setMatrix(matrix);
			}
		}

		nemoshow_canvas_render_item(canvas, one);

		if (has_context != 0)
			canvas->restore();
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
			nemoshow_canvas_render_one(NEMOSHOW_CANVAS_CC(canvas, canvas), one->children[i]);
		}

		NEMOSHOW_CANVAS_CC(canvas, canvas)->restore();
	} else {
		NEMOSHOW_CANVAS_CC(canvas, canvas)->save();
		NEMOSHOW_CANVAS_CC(canvas, canvas)->clear(SK_ColorTRANSPARENT);
		NEMOSHOW_CANVAS_CC(canvas, canvas)->scale(canvas->viewport.sx, canvas->viewport.sy);

		for (i = 0; i < one->nchildren; i++) {
			nemoshow_canvas_render_one(NEMOSHOW_CANVAS_CC(canvas, canvas), one->children[i]);
		}

		NEMOSHOW_CANVAS_CC(canvas, canvas)->restore();

		canvas->needs_full_redraw = 0;
	}

	NEMOSHOW_CANVAS_CC(canvas, damage)->setEmpty();
}

void nemoshow_canvas_render_back(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_fill_pixman(canvas->node, canvas->fills[2], canvas->fills[1], canvas->fills[0], canvas->fills[3]);
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

		nemotale_node_set_viewport_pixman(canvas->node, canvas->viewport.width, canvas->viewport.height);

		NEMOSHOW_CANVAS_CC(canvas, bitmap) = new SkBitmap;
		NEMOSHOW_CANVAS_CC(canvas, bitmap)->setInfo(
				SkImageInfo::Make(canvas->viewport.width, canvas->viewport.height, kN32_SkColorType, kPremul_SkAlphaType));
		NEMOSHOW_CANVAS_CC(canvas, bitmap)->setPixels(
				nemotale_node_get_buffer(canvas->node));

		NEMOSHOW_CANVAS_CC(canvas, device) = new SkBitmapDevice(*NEMOSHOW_CANVAS_CC(canvas, bitmap));
		NEMOSHOW_CANVAS_CC(canvas, canvas) = new SkCanvas(NEMOSHOW_CANVAS_CC(canvas, device));

		nemoshow_canvas_damage_all(one);
	}

	return 0;
}

void nemoshow_canvas_damage_region(struct showone *one, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	NEMOSHOW_CANVAS_CC(canvas, damage)->op(
			SkIRect::MakeXYWH(
				floor(x * canvas->viewport.sx),
				floor(y * canvas->viewport.sy),
				ceil(width * canvas->viewport.sx) + 2.0f,
				ceil(height * canvas->viewport.sy) + 2.0f),
			SkRegion::kUnion_Op);

	nemotale_node_damage(canvas->node, x, y, width, height);

	canvas->needs_redraw = 1;
}

void nemoshow_canvas_damage_one(struct showone *one, struct showone *child)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	NEMOSHOW_CANVAS_CC(canvas, damage)->op(
			SkIRect::MakeXYWH(
				floor(child->x * canvas->viewport.sx),
				floor(child->y * canvas->viewport.sy),
				ceil(child->width * canvas->viewport.sx) + 2.0f,
				ceil(child->height * canvas->viewport.sy) + 2.0f),
			SkRegion::kUnion_Op);

	nemotale_node_damage(canvas->node, child->x, child->y, child->width, child->height);

	canvas->needs_redraw = 1;
}

void nemoshow_canvas_damage_all(struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_damage_all(canvas->node);

	canvas->needs_redraw = 1;
	canvas->needs_full_redraw = 1;
}

void nemoshow_canvas_translate(struct showone *one, double tx, double ty)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_translate(canvas->node, tx, ty);

	canvas->tx = tx;
	canvas->ty = ty;
}

void nemoshow_canvas_rotate(struct showone *one, double ro)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_rotate(canvas->node, ro * M_PI / 180.0f);

	canvas->ro = ro;
}

void nemoshow_canvas_pivot(struct showone *one, double px, double py)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_pivot(canvas->node, px, py);

	canvas->px = px;
	canvas->py = py;
}

void nemoshow_canvas_scale(struct showone *one, double sx, double sy)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_scale(canvas->node, sx, sy);

	canvas->sx = sx;
	canvas->sy = sy;
}

static inline struct showone *nemoshow_canvas_pick_one_in(struct showone *one, double px, double py)
{
	struct showone *child;
	int i;

	if (one->nchildren <= 0)
		return NULL;

	for (i = one->nchildren - 1; i >= 0; i--) {
		child = one->children[i];

		if (child->type == NEMOSHOW_ITEM_TYPE) {
			if (child->sub == NEMOSHOW_GROUP_ITEM) {
				struct showone *pick;

				pick = nemoshow_canvas_pick_one_in(child, px, py);
				if (pick != NULL)
					return pick;
			} else if (child->tag != 0) {
				struct showitem *item = NEMOSHOW_ITEM(child);

				if (NEMOSHOW_ITEM_CC(item, has_inverse)) {
					SkPoint p = NEMOSHOW_ITEM_CC(item, inverse)->mapXY(px, py);

					if (child->x0 < p.x() && p.x() < child->x1 &&
							child->y0 < p.y() && p.y() < child->y1)
						return child;
				}
			}
		}
	}

	return NULL;
}

struct showone *nemoshow_canvas_pick_one(struct showone *one, int x, int y)
{
	return nemoshow_canvas_pick_one_in(one, x, y);
}
