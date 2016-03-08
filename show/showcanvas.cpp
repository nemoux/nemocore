#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <float.h>

#include <skiaconfig.hpp>

#include <showcanvas.h>
#include <showcanvas.hpp>
#include <showitem.h>
#include <showitem.hpp>
#include <showpoly.h>
#include <showpipe.h>
#include <showmatrix.h>
#include <showmatrix.hpp>
#include <showpath.h>
#include <showfont.h>
#include <showfont.hpp>
#include <showhelper.hpp>
#include <fbohelper.h>
#include <oshelper.h>
#include <nemoshow.h>
#include <nemoxml.h>
#include <nemobox.h>
#include <nemometro.h>
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

	canvas->viewport.dirty = 1;
	canvas->viewport.sx = 1.0f;
	canvas->viewport.sy = 1.0f;

	canvas->sx = 1.0f;
	canvas->sy = 1.0f;
	canvas->px = 0.5f;
	canvas->py = 0.5f;

	canvas->alpha = 1.0f;

	canvas->is_mapped = 0;
	canvas->needs_resize = 0;
	canvas->needs_redraw = 1;
	canvas->needs_full_redraw = 1;

	one = &canvas->base;
	one->type = NEMOSHOW_CANVAS_TYPE;
	one->update = nemoshow_canvas_update;
	one->destroy = nemoshow_canvas_destroy;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "type", canvas->type, NEMOSHOW_CANVAS_TYPE_MAX);
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

	if (canvas->show != NULL)
		nemoshow_detach_canvas(canvas->show, one);

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

int nemoshow_canvas_arrange(struct showone *one)
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
	} else if (type == NEMOSHOW_CANVAS_PIPELINE_TYPE) {
		canvas->node = nemotale_node_create_gl(canvas->width, canvas->height);
	} else if (type == NEMOSHOW_CANVAS_PIXMAN_TYPE) {
		canvas->node = nemotale_node_create_pixman(canvas->width, canvas->height);
	} else if (type == NEMOSHOW_CANVAS_OPENGL_TYPE) {
		canvas->node = nemotale_node_create_gl(canvas->width, canvas->height);
	} else if (type == NEMOSHOW_CANVAS_BACK_TYPE) {
		canvas->node = nemotale_node_create_pixman(canvas->width, canvas->height);
		nemotale_node_opaque(canvas->node, 0, 0, canvas->width, canvas->height);
	}

	one->sub = type;

	canvas->needs_resize = 0;

	nemotale_node_set_data(canvas->node, one);

	return 0;
}

int nemoshow_canvas_resize(struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	if (one->sub == NEMOSHOW_CANVAS_VECTOR_TYPE) {
		nemotale_node_resize_pixman(canvas->node, canvas->width, canvas->height);

		delete NEMOSHOW_CANVAS_CC(canvas, damage);
		delete NEMOSHOW_CANVAS_CC(canvas, canvas);
		delete NEMOSHOW_CANVAS_CC(canvas, device);
		delete NEMOSHOW_CANVAS_CC(canvas, bitmap);

		NEMOSHOW_CANVAS_CC(canvas, bitmap) = new SkBitmap;
		NEMOSHOW_CANVAS_CC(canvas, bitmap)->setInfo(
				SkImageInfo::Make(canvas->width, canvas->height, kN32_SkColorType, kPremul_SkAlphaType));
		NEMOSHOW_CANVAS_CC(canvas, bitmap)->setPixels(
				nemotale_node_get_buffer(canvas->node));

		NEMOSHOW_CANVAS_CC(canvas, device) = new SkBitmapDevice(*NEMOSHOW_CANVAS_CC(canvas, bitmap));
		NEMOSHOW_CANVAS_CC(canvas, canvas) = new SkCanvas(NEMOSHOW_CANVAS_CC(canvas, device));

		NEMOSHOW_CANVAS_CC(canvas, damage) = new SkRegion;
	} else if (one->sub == NEMOSHOW_CANVAS_PIPELINE_TYPE) {
		nemotale_node_resize_gl(canvas->node, canvas->width, canvas->height);
	} else if (one->sub == NEMOSHOW_CANVAS_PIXMAN_TYPE) {
		nemotale_node_resize_pixman(canvas->node, canvas->width, canvas->height);
	} else if (one->sub == NEMOSHOW_CANVAS_OPENGL_TYPE) {
		nemotale_node_resize_gl(canvas->node, canvas->width, canvas->height);
	} else if (one->sub == NEMOSHOW_CANVAS_BACK_TYPE) {
		nemotale_node_resize_pixman(canvas->node, canvas->width, canvas->height);
		nemotale_node_opaque(canvas->node, 0, 0, canvas->width, canvas->height);
	}

	return 0;
}

void nemoshow_canvas_set_event(struct showone *one, uint32_t event)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_set_id(canvas->node, event);
}

uint32_t nemoshow_canvas_get_event(struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	return nemotale_node_get_id(canvas->node);
}

void nemoshow_canvas_set_alpha(struct showone *one, double alpha)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_set_alpha(canvas->node, alpha);

	canvas->alpha = alpha;
}

int nemoshow_canvas_set_filter(struct showone *one, const char *shader)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	return nemotale_node_set_filter(canvas->node, shader);
}

int nemoshow_canvas_load_filter(struct showone *one, const char *shaderpath)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);
	char *shader = NULL;
	int r;

	if (os_load_path(shaderpath, &shader, NULL) < 0)
		return -1;

	r = nemotale_node_set_filter(canvas->node, shader);

	free(shader);

	return 0;
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

	nemoshow_canvas_damage_all(one);
}

static inline void nemoshow_canvas_update_matrix(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_translate(canvas->node, canvas->tx, canvas->ty);
	nemotale_node_rotate(canvas->node, canvas->ro * M_PI / 180.0f);
	nemotale_node_scale(canvas->node, canvas->sx, canvas->sy);
	nemotale_node_pivot(canvas->node, canvas->px, canvas->py);

	nemoshow_canvas_damage_all(one);
}

int nemoshow_canvas_update(struct showone *one)
{
	struct nemoshow *show = one->show;
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	if ((one->dirty & NEMOSHOW_STYLE_DIRTY) != 0)
		nemoshow_canvas_update_style(show, one);
	if ((one->dirty & NEMOSHOW_MATRIX_DIRTY) != 0)
		nemoshow_canvas_update_matrix(show, one);
	if ((one->dirty & NEMOSHOW_FILTER_DIRTY) != 0)
		nemoshow_canvas_damage_filter(one);

	return 0;
}

static inline int nemoshow_canvas_check_one(struct showcanvas *canvas, struct showone *one);
static inline void nemoshow_canvas_render_one(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one);

static inline void nemoshow_canvas_render_item_none(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
}

static inline void nemoshow_canvas_render_item_line(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (nemoshow_one_has_state(one, NEMOSHOW_FILL_STATE))
		_canvas->drawLine(item->x, item->y, item->width, item->height, *NEMOSHOW_ITEM_CC(item, fill));
	if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE))
		_canvas->drawLine(item->x, item->y, item->width, item->height, *NEMOSHOW_ITEM_CC(item, stroke));
}

static inline void nemoshow_canvas_render_item_rect(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	SkRect rect = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);

	if (nemoshow_one_has_state(one, NEMOSHOW_FILL_STATE))
		_canvas->drawRect(rect, *NEMOSHOW_ITEM_CC(item, fill));
	if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE))
		_canvas->drawRect(rect, *NEMOSHOW_ITEM_CC(item, stroke));
}

static inline void nemoshow_canvas_render_item_rrect(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	SkRect rect = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);

	if (nemoshow_one_has_state(one, NEMOSHOW_FILL_STATE))
		_canvas->drawRoundRect(rect, item->ox, item->oy, *NEMOSHOW_ITEM_CC(item, fill));
	if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE))
		_canvas->drawRoundRect(rect, item->ox, item->oy, *NEMOSHOW_ITEM_CC(item, stroke));
}

static inline void nemoshow_canvas_render_item_circle(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (nemoshow_one_has_state(one, NEMOSHOW_FILL_STATE))
		_canvas->drawCircle(item->x, item->y, item->r, *NEMOSHOW_ITEM_CC(item, fill));
	if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE))
		_canvas->drawCircle(item->x, item->y, item->r, *NEMOSHOW_ITEM_CC(item, stroke));
}

static inline void nemoshow_canvas_render_item_arc(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	SkRect rect = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);

	if (nemoshow_one_has_state(one, NEMOSHOW_FILL_STATE))
		_canvas->drawArc(rect, item->from, item->to - item->from, false, *NEMOSHOW_ITEM_CC(item, fill));
	if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE))
		_canvas->drawArc(rect, item->from, item->to - item->from, false, *NEMOSHOW_ITEM_CC(item, stroke));
}

static inline void nemoshow_canvas_render_item_pie(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	SkRect rect = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);

	if (nemoshow_one_has_state(one, NEMOSHOW_FILL_STATE))
		_canvas->drawArc(rect, item->from, item->to - item->from, true, *NEMOSHOW_ITEM_CC(item, fill));
	if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE))
		_canvas->drawArc(rect, item->from, item->to - item->from, true, *NEMOSHOW_ITEM_CC(item, stroke));
}

static inline void nemoshow_canvas_render_item_donut(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (nemoshow_one_has_state(one, NEMOSHOW_FILL_STATE))
		_canvas->drawPath(*NEMOSHOW_ITEM_CC(item, path), *NEMOSHOW_ITEM_CC(item, fill));
	if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE))
		_canvas->drawPath(*NEMOSHOW_ITEM_CC(item, path), *NEMOSHOW_ITEM_CC(item, stroke));
}

static inline void nemoshow_canvas_render_item_ring(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (nemoshow_one_has_state(one, NEMOSHOW_FILL_STATE))
		_canvas->drawCircle(item->x, item->y, item->inner, *NEMOSHOW_ITEM_CC(item, fill));
	if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE))
		_canvas->drawPath(*NEMOSHOW_ITEM_CC(item, path), *NEMOSHOW_ITEM_CC(item, stroke));
}

static inline void nemoshow_canvas_render_item_path(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (item->from == 0.0f && item->to == 1.0f) {
		if (nemoshow_one_has_state(one, NEMOSHOW_FILL_STATE)) {
			if (NEMOSHOW_ITEM_CC(item, fillpath) != NULL)
				_canvas->drawPath(*NEMOSHOW_ITEM_CC(item, fillpath), *NEMOSHOW_ITEM_CC(item, fill));
			else
				_canvas->drawPath(*NEMOSHOW_ITEM_CC(item, path), *NEMOSHOW_ITEM_CC(item, fill));
		}
		if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE)) {
			if (NEMOSHOW_ITEM_CC(item, strokepath) != NULL)
				_canvas->drawPath(*NEMOSHOW_ITEM_CC(item, strokepath), *NEMOSHOW_ITEM_CC(item, stroke));
			else
				_canvas->drawPath(*NEMOSHOW_ITEM_CC(item, path), *NEMOSHOW_ITEM_CC(item, stroke));
		}
	} else {
		SkPath path;

		nemoshow_helper_draw_path(
				path,
				NEMOSHOW_ITEM_CC(item, path),
				item->pathlength,
				item->from, item->to);

		_canvas->drawPath(path, *NEMOSHOW_ITEM_CC(item, stroke));
	}
}

static inline void nemoshow_canvas_render_item_path_group(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (item->from == 0.0f && item->to == 1.0f) {
		if (nemoshow_one_has_state(one, NEMOSHOW_FILL_STATE))
			_canvas->drawPath(*NEMOSHOW_ITEM_CC(item, path), *NEMOSHOW_ITEM_CC(item, fill));
		if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE))
			_canvas->drawPath(*NEMOSHOW_ITEM_CC(item, path), *NEMOSHOW_ITEM_CC(item, stroke));
	} else {
		SkPath path;

		nemoshow_helper_draw_path(
				path,
				NEMOSHOW_ITEM_CC(item, path),
				item->pathlength,
				item->from, item->to);

		_canvas->drawPath(path, *NEMOSHOW_ITEM_CC(item, stroke));
	}
}

static inline void nemoshow_canvas_render_item_text(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (NEMOSHOW_REF(one, NEMOSHOW_PATH_REF) == NULL) {
		if (NEMOSHOW_FONT_AT(NEMOSHOW_REF(one, NEMOSHOW_FONT_REF), layout) == NEMOSHOW_NORMAL_LAYOUT) {
			_canvas->save();
			_canvas->translate(0.0f, -item->fontascent);

			if (nemoshow_one_has_state(one, NEMOSHOW_FILL_STATE))
				_canvas->drawText(
						item->text,
						strlen(item->text),
						item->x,
						item->y,
						*NEMOSHOW_ITEM_CC(item, fill));
			if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE))
				_canvas->drawText(
						item->text,
						strlen(item->text),
						item->x,
						item->y,
						*NEMOSHOW_ITEM_CC(item, stroke));

			_canvas->restore();
		} else if (NEMOSHOW_FONT_AT(NEMOSHOW_REF(one, NEMOSHOW_FONT_REF), layout) == NEMOSHOW_HARFBUZZ_LAYOUT) {
			if (nemoshow_one_has_state(one, NEMOSHOW_FILL_STATE))
				_canvas->drawPosText(
						item->text,
						strlen(item->text),
						NEMOSHOW_ITEM_CC(item, points),
						*NEMOSHOW_ITEM_CC(item, fill));
			if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE))
				_canvas->drawPosText(
						item->text,
						strlen(item->text),
						NEMOSHOW_ITEM_CC(item, points),
						*NEMOSHOW_ITEM_CC(item, stroke));
		}
	} else {
		if (nemoshow_one_has_state(one, NEMOSHOW_FILL_STATE))
			_canvas->drawTextOnPath(
					item->text,
					strlen(item->text),
					*NEMOSHOW_ITEM_CC(NEMOSHOW_ITEM(NEMOSHOW_REF(one, NEMOSHOW_PATH_REF)), path),
					NULL,
					*NEMOSHOW_ITEM_CC(item, fill));
		if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE))
			_canvas->drawTextOnPath(
					item->text,
					strlen(item->text),
					*NEMOSHOW_ITEM_CC(NEMOSHOW_ITEM(NEMOSHOW_REF(one, NEMOSHOW_PATH_REF)), path),
					NULL,
					*NEMOSHOW_ITEM_CC(item, stroke));
	}
}

static inline void nemoshow_canvas_render_item_textbox(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (item->text != NULL) {
		if (nemoshow_one_has_state(one, NEMOSHOW_FILL_STATE)) {
			NEMOSHOW_ITEM_CC(item, textbox)->setText(item->text, strlen(item->text), *NEMOSHOW_ITEM_CC(item, fill));
			NEMOSHOW_ITEM_CC(item, textbox)->draw(_canvas);
		}
		if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE)) {
			NEMOSHOW_ITEM_CC(item, textbox)->setText(item->text, strlen(item->text), *NEMOSHOW_ITEM_CC(item, stroke));
			NEMOSHOW_ITEM_CC(item, textbox)->draw(_canvas);
		}
	}
}

static inline void nemoshow_canvas_render_item_image(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (NEMOSHOW_ITEM_CC(item, bitmap) != NULL) {
		SkRect rect = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);

		if (item->alpha == 1.0f) {
			_canvas->drawBitmapRect(*NEMOSHOW_ITEM_CC(item, bitmap), rect, NULL);
		} else {
			_canvas->saveLayerAlpha(&rect, 0xff * item->alpha);
			_canvas->drawBitmapRect(*NEMOSHOW_ITEM_CC(item, bitmap), rect, NULL);
			_canvas->restore();
		}
	}
}

static inline void nemoshow_canvas_render_item_svg(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showone *child;

	_canvas->save();
	_canvas->concat(*NEMOSHOW_ITEM_CC(item, viewbox));

	if (canvas->needs_full_redraw == 0) {
		nemoshow_children_for_each(child, one) {
			if (nemoshow_canvas_check_one(canvas, child) != 0)
				nemoshow_canvas_render_one(canvas, _canvas, child);
		}
	} else {
		nemoshow_children_for_each(child, one) {
			nemoshow_canvas_render_one(canvas, _canvas, child);
		}
	}

	_canvas->restore();
}

static inline void nemoshow_canvas_render_item_group(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one)
{
	struct showone *child;

	if (canvas->needs_full_redraw == 0) {
		nemoshow_children_for_each(child, one) {
			if (nemoshow_canvas_check_one(canvas, child) != 0)
				nemoshow_canvas_render_one(canvas, _canvas, child);
		}
	} else {
		nemoshow_children_for_each(child, one) {
			nemoshow_canvas_render_one(canvas, _canvas, child);
		}
	}
}

static inline int nemoshow_canvas_check_one(struct showcanvas *canvas, struct showone *one)
{
	SkIRect bounds = SkIRect::MakeXYWH(one->sx, one->sy, one->sw, one->sh);

	return NEMOSHOW_CANVAS_CC(canvas, damage)->intersects(bounds);
}

typedef void (*nemoshow_canvas_render_t)(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one);

static inline void nemoshow_canvas_render_one(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one)
{
	static nemoshow_canvas_render_t renderers[NEMOSHOW_LAST_ITEM] = {
		nemoshow_canvas_render_item_none,
		nemoshow_canvas_render_item_line,
		nemoshow_canvas_render_item_rect,
		nemoshow_canvas_render_item_rrect,
		nemoshow_canvas_render_item_circle,
		nemoshow_canvas_render_item_arc,
		nemoshow_canvas_render_item_pie,
		nemoshow_canvas_render_item_donut,
		nemoshow_canvas_render_item_ring,
		nemoshow_canvas_render_item_text,
		nemoshow_canvas_render_item_textbox,
		nemoshow_canvas_render_item_path,
		nemoshow_canvas_render_item_path_group,
		nemoshow_canvas_render_item_image,
		nemoshow_canvas_render_item_svg,
		nemoshow_canvas_render_item_group
	};
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showitem *clip = NEMOSHOW_REF(one, NEMOSHOW_CLIP_REF) == NULL ? NULL : NEMOSHOW_ITEM(NEMOSHOW_REF(one, NEMOSHOW_CLIP_REF));
	int has_context = nemoshow_one_has_state(one, NEMOSHOW_TRANSFORM_STATE) || nemoshow_one_has_state(one, NEMOSHOW_ANCHOR_STATE) || clip != NULL;

	if (has_context != 0)
		_canvas->save();

	if (nemoshow_one_has_state(one, NEMOSHOW_TRANSFORM_STATE))
		_canvas->concat(*NEMOSHOW_ITEM_CC(item, modelview));

	if (nemoshow_one_has_state(one, NEMOSHOW_ANCHOR_STATE))
		_canvas->translate(-item->width * item->ax, -item->height * item->ay);

	if (nemoshow_one_has_state(one, NEMOSHOW_VIEWPORT_STATE))
		_canvas->scale(item->width / item->width0, item->height / item->height0);

	if (clip != NULL) {
		if (clip->transform == 0) {
			_canvas->clipPath(*NEMOSHOW_ITEM_CC(clip, path));
		} else {
			SkMatrix matrix = _canvas->getTotalMatrix();

			_canvas->resetMatrix();
			_canvas->scale(
					nemoshow_canvas_get_viewport_sx(one->canvas),
					nemoshow_canvas_get_viewport_sy(one->canvas));
			_canvas->concat(*NEMOSHOW_ITEM_CC(clip, modelview));

			if (nemoshow_one_has_state(NEMOSHOW_ITEM_ONE(clip), NEMOSHOW_ANCHOR_STATE))
				_canvas->translate(-clip->width * clip->ax, -clip->height * clip->ay);

			_canvas->clipPath(*NEMOSHOW_ITEM_CC(clip, path));

			_canvas->setMatrix(matrix);
		}
	}

	renderers[one->sub](canvas, _canvas, one);

	if (has_context != 0)
		_canvas->restore();
}

void nemoshow_canvas_render_vector(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);
	struct showone *child;

	if (canvas->needs_full_redraw == 0) {
		NEMOSHOW_CANVAS_CC(canvas, canvas)->save();
		NEMOSHOW_CANVAS_CC(canvas, canvas)->clipRegion(*NEMOSHOW_CANVAS_CC(canvas, damage));
		NEMOSHOW_CANVAS_CC(canvas, canvas)->clear(SK_ColorTRANSPARENT);
		NEMOSHOW_CANVAS_CC(canvas, canvas)->scale(canvas->viewport.sx, canvas->viewport.sy);

		nemoshow_children_for_each(child, one) {
			if (nemoshow_canvas_check_one(canvas, child) != 0)
				nemoshow_canvas_render_one(canvas, NEMOSHOW_CANVAS_CC(canvas, canvas), child);
		}

		NEMOSHOW_CANVAS_CC(canvas, canvas)->restore();
	} else {
		NEMOSHOW_CANVAS_CC(canvas, canvas)->save();
		NEMOSHOW_CANVAS_CC(canvas, canvas)->clear(SK_ColorTRANSPARENT);
		NEMOSHOW_CANVAS_CC(canvas, canvas)->scale(canvas->viewport.sx, canvas->viewport.sy);

		nemoshow_children_for_each(child, one) {
			nemoshow_canvas_render_one(canvas, NEMOSHOW_CANVAS_CC(canvas, canvas), child);
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

void nemoshow_canvas_flush_now(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	if (canvas->needs_redraw != 0) {
		canvas->needs_redraw = 0;

		if (canvas->dispatch_redraw != NULL)
			canvas->dispatch_redraw(show, one);
		else
			nemoshow_canvas_redraw_one(show, one);
	}

	if (one->sub == NEMOSHOW_CANVAS_VECTOR_TYPE) {
		nemotale_node_flush_gl(show->tale, canvas->node);
		nemotale_node_filter_gl(show->tale, canvas->node);
	} else if (one->sub == NEMOSHOW_CANVAS_PIXMAN_TYPE) {
		nemotale_node_flush_gl(show->tale, canvas->node);
		nemotale_node_filter_gl(show->tale, canvas->node);
	} else if (one->sub == NEMOSHOW_CANVAS_OPENGL_TYPE || one->sub == NEMOSHOW_CANVAS_PIPELINE_TYPE) {
		nemotale_node_filter_gl(show->tale, canvas->node);
	}
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
		nemoshow_canvas_dirty_all(one, NEMOSHOW_SHAPE_DIRTY);
	} else if (one->sub == NEMOSHOW_CANVAS_PIPELINE_TYPE) {
		canvas->viewport.sx = sx;
		canvas->viewport.sy = sy;

		canvas->viewport.width = canvas->width * sx;
		canvas->viewport.height = canvas->height * sy;

		nemotale_node_set_viewport_gl(canvas->node, canvas->viewport.width, canvas->viewport.height);

		fbo_prepare_context(
				nemotale_node_get_texture(canvas->node),
				canvas->viewport.width,
				canvas->viewport.height,
				&canvas->fbo, &canvas->dbo);

		nemoshow_canvas_damage_all(one);
	} else if (one->sub == NEMOSHOW_CANVAS_PIXMAN_TYPE) {
		canvas->viewport.sx = sx;
		canvas->viewport.sy = sy;

		canvas->viewport.width = canvas->width * sx;
		canvas->viewport.height = canvas->height * sy;

		nemotale_node_set_viewport_pixman(canvas->node, canvas->viewport.width, canvas->viewport.height);

		nemoshow_canvas_damage_all(one);
	} else if (one->sub == NEMOSHOW_CANVAS_OPENGL_TYPE) {
		canvas->viewport.sx = sx;
		canvas->viewport.sy = sy;

		canvas->viewport.width = canvas->width * sx;
		canvas->viewport.height = canvas->height * sy;

		nemotale_node_set_viewport_gl(canvas->node, canvas->viewport.width, canvas->viewport.height);

		nemoshow_canvas_damage_all(one);
	}

	return 0;
}

void nemoshow_canvas_damage_one(struct showone *one, struct showone *child)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	NEMOSHOW_CANVAS_CC(canvas, damage)->op(
			SkIRect::MakeXYWH(child->sx, child->sy, child->sw, child->sh),
			SkRegion::kUnion_Op);

	nemotale_node_damage(canvas->node, child->x, child->y, child->w, child->h);

	canvas->needs_redraw = 1;

	nemoshow_one_dirty(one, NEMOSHOW_CANVAS_DIRTY);
}

void nemoshow_canvas_damage_all(struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_damage_all(canvas->node);

	canvas->needs_redraw = 1;
	canvas->needs_full_redraw = 1;

	nemoshow_one_dirty(one, NEMOSHOW_CANVAS_DIRTY);
}

void nemoshow_canvas_damage_filter(struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_damage_filter(canvas->node);

	nemoshow_one_dirty(one, NEMOSHOW_CANVAS_DIRTY);
}

void nemoshow_canvas_dirty_all(struct showone *one, uint32_t dirty)
{
	struct showone *child;

	nemoshow_children_for_each(child, one)
		nemoshow_one_dirty(child, dirty);
}

static inline struct showone *nemoshow_canvas_pick_item(struct showone *one, double x, double y)
{
	struct showone *child;
	struct showone *pick;

	nemoshow_children_for_each_reverse(child, one) {
		if (child->sub == NEMOSHOW_GROUP_ITEM) {
			if (child->tag != 0) {
				if (nemoshow_item_check_one(child, x, y) != 0)
					return child;
			} else {
				pick = nemoshow_canvas_pick_item(child, x, y);
				if (pick != NULL)
					return pick;
			}
		} else {
			if (child->tag != 0) {
				if (nemoshow_item_check_one(child, x, y) != 0)
					return child;
			}
		}
	}

	return NULL;
}

static inline struct showone *nemoshow_canvas_pick_poly(struct showone *one, double x, double y)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);
	struct showone *pone, *cone;
	struct showone *pick = NULL;
	float min = FLT_MAX;
	float t;

	nemoshow_children_for_each(pone, one) {
		nemoshow_children_for_each(cone, pone) {
			if (cone->tag != 0) {
				t = nemoshow_poly_check_one(one, pone, cone, x, y);
				if (min > t) {
					min = t;
					pick = cone;
				}
			}
		}
	}

	return pick;
}

struct showone *nemoshow_canvas_pick_one(struct showone *one, double x, double y)
{
	if (one->sub == NEMOSHOW_CANVAS_PIPELINE_TYPE)
		return nemoshow_canvas_pick_poly(one, x, y);

	return nemoshow_canvas_pick_item(one, x, y);
}

static void nemoshow_canvas_handle_canvas_destroy_signal(struct nemolistener *listener, void *data)
{
	struct showone *one = (struct showone *)container_of(listener, struct showone, canvas_destroy_listener);

	one->canvas = NULL;

	nemolist_remove(&one->canvas_destroy_listener.link);
	nemolist_init(&one->canvas_destroy_listener.link);
}

void nemoshow_canvas_attach_one(struct showone *canvas, struct showone *one)
{
	one->canvas = canvas;

	one->canvas_destroy_listener.notify = nemoshow_canvas_handle_canvas_destroy_signal;
	nemosignal_add(&canvas->destroy_signal, &one->canvas_destroy_listener);
}

void nemoshow_canvas_detach_one(struct showone *one)
{
	nemolist_remove(&one->canvas_destroy_listener.link);
	nemolist_init(&one->canvas_destroy_listener.link);

	one->canvas = NULL;
}

void nemoshow_canvas_attach_ones(struct showone *canvas, struct showone *one)
{
	struct showone *child;

	if (one->canvas == canvas)
		return;

	if (one->canvas != NULL)
		nemoshow_canvas_detach_one(one);

	nemoshow_canvas_attach_one(canvas, one);

	nemoshow_children_for_each(child, one)
		nemoshow_canvas_attach_ones(canvas, child);
}

void nemoshow_canvas_detach_ones(struct showone *one)
{
	struct showone *child;

	nemoshow_canvas_detach_one(one);

	nemoshow_children_for_each(child, one)
		nemoshow_canvas_detach_ones(child);
}
