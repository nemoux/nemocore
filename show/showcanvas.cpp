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
#include <showmatrix.h>
#include <showmatrix.hpp>
#include <showpath.h>
#include <showfont.h>
#include <showfont.hpp>
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

	canvas->viewport.sx = 1.0f;
	canvas->viewport.sy = 1.0f;

	one = &canvas->base;
	one->type = NEMOSHOW_CANVAS_TYPE;
	one->update = nemoshow_canvas_update;
	one->destroy = nemoshow_canvas_destroy;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "type", canvas->type, NEMOSHOW_CANVAS_TYPE_MAX);
	nemoobject_set_reserved(&one->object, "src", canvas->src, NEMOSHOW_CANVAS_SRC_MAX);
	nemoobject_set_reserved(&one->object, "event", &canvas->event, sizeof(int32_t));
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
	static struct canvasmap {
		char name[32];

		int type;
	} maps[] = {
		{ "back",				NEMOSHOW_CANVAS_BACK_TYPE },
		{ "img",				NEMOSHOW_CANVAS_IMAGE_TYPE },
		{ "opengl",			NEMOSHOW_CANVAS_OPENGL_TYPE },
		{ "pixman",			NEMOSHOW_CANVAS_PIXMAN_TYPE },
		{ "ref",				NEMOSHOW_CANVAS_REF_TYPE },
		{ "scene",			NEMOSHOW_CANVAS_SCENE_TYPE },
		{ "svg",				NEMOSHOW_CANVAS_SVG_TYPE },
		{ "use",				NEMOSHOW_CANVAS_USE_TYPE },
		{ "vec",				NEMOSHOW_CANVAS_VECTOR_TYPE },
	}, *map;
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);
	struct showone *matrix;

	map = static_cast<struct canvasmap *>(bsearch(canvas->type, static_cast<void *>(maps), sizeof(maps) / sizeof(maps[0]), sizeof(maps[0]), nemoshow_canvas_compare));
	if (map == NULL)
		one->sub = NEMOSHOW_CANVAS_NONE_TYPE;
	else
		one->sub = map->type;

	if (one->sub == NEMOSHOW_CANVAS_VECTOR_TYPE) {
		canvas->node = nemotale_node_create_pixman(canvas->width, canvas->height);

		NEMOSHOW_CANVAS_CC(canvas, bitmap) = new SkBitmap;
		NEMOSHOW_CANVAS_CC(canvas, bitmap)->setInfo(
				SkImageInfo::Make(canvas->width, canvas->height, kN32_SkColorType, kPremul_SkAlphaType));
		NEMOSHOW_CANVAS_CC(canvas, bitmap)->setPixels(
				nemotale_node_get_buffer(canvas->node));

		NEMOSHOW_CANVAS_CC(canvas, device) = new SkBitmapDevice(*NEMOSHOW_CANVAS_CC(canvas, bitmap));
		NEMOSHOW_CANVAS_CC(canvas, canvas) = new SkCanvas(NEMOSHOW_CANVAS_CC(canvas, device));

		NEMOSHOW_CANVAS_CC(canvas, damage) = new SkRegion;
	} else if (one->sub == NEMOSHOW_CANVAS_PIXMAN_TYPE) {
		canvas->node = nemotale_node_create_pixman(canvas->width, canvas->height);
	} else if (one->sub == NEMOSHOW_CANVAS_OPENGL_TYPE) {
		canvas->node = nemotale_node_create_gl(canvas->width, canvas->height);
	} else if (one->sub == NEMOSHOW_CANVAS_SCENE_TYPE) {
		struct showone *src;
		struct showone *child;
		struct showscene *scene;
		int i;

		canvas->node = nemotale_node_create_gl(canvas->width, canvas->height);

		canvas->tale = nemotale_create_gl();
		canvas->fbo = nemotale_create_fbo(
				nemotale_node_get_texture(canvas->node),
				canvas->width, canvas->height);
		nemotale_set_backend(canvas->tale, canvas->fbo);
		nemotale_resize(canvas->tale, canvas->width, canvas->height);

		src = nemoshow_search_one(show, canvas->src);
		scene = NEMOSHOW_SCENE(src);

		for (i = 0; i < src->nchildren; i++) {
			child = src->children[i];

			if (child->type == NEMOSHOW_CANVAS_TYPE && child != one) {
				nemotale_attach_node(canvas->tale, NEMOSHOW_CANVAS_AT(child, node));
			}
		}

		nemotale_scale(canvas->tale, canvas->width / scene->width, canvas->height / scene->height);
	} else if (one->sub == NEMOSHOW_CANVAS_BACK_TYPE) {
		canvas->node = nemotale_node_create_pixman(canvas->width, canvas->height);
		nemotale_node_opaque(canvas->node, 0, 0, canvas->width, canvas->height);
	}

	if (canvas->event == 0)
		nemotale_node_set_pick_type(canvas->node, NEMOTALE_PICK_NO_TYPE);
	else
		nemotale_node_set_id(canvas->node, canvas->event);

	matrix = nemoshow_search_one(show, nemoobject_gets(&one->object, "matrix"));
	if (matrix != NULL) {
		canvas->matrix = matrix;

		NEMOBOX_APPEND(matrix->refs, matrix->srefs, matrix->nrefs, one);
	}

	return 0;
}

int nemoshow_canvas_update(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	if (canvas->matrix != NULL) {
		float d[9];

		NEMOSHOW_MATRIX_CC(NEMOSHOW_MATRIX(canvas->matrix), matrix)->get9(d);

		nemotale_node_transform(canvas->node, d);
		nemotale_node_damage_all(canvas->node);
	}

	return 0;
}

static inline void nemoshow_canvas_render_item(struct nemoshow *show, struct showcanvas *canvas, int type, struct showitem *item, struct showitem *style)
{
	if (type == NEMOSHOW_RECT_ITEM) {
		SkRect rect = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);

		if (style->fill != 0)
			NEMOSHOW_CANVAS_CC(canvas, canvas)->drawRect(rect, *NEMOSHOW_ITEM_CC(style, fill));
		if (style->stroke != 0)
			NEMOSHOW_CANVAS_CC(canvas, canvas)->drawRect(rect, *NEMOSHOW_ITEM_CC(style, stroke));
	} else if (type == NEMOSHOW_CIRCLE_ITEM) {
		if (style->fill != 0)
			NEMOSHOW_CANVAS_CC(canvas, canvas)->drawCircle(item->x, item->y, item->r, *NEMOSHOW_ITEM_CC(style, fill));
		if (style->stroke != 0)
			NEMOSHOW_CANVAS_CC(canvas, canvas)->drawCircle(item->x, item->y, item->r, *NEMOSHOW_ITEM_CC(style, stroke));
	} else if (type == NEMOSHOW_ARC_ITEM) {
		SkRect rect = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);

		if (style->fill != 0)
			NEMOSHOW_CANVAS_CC(canvas, canvas)->drawArc(rect, item->from, item->to - item->from, false, *NEMOSHOW_ITEM_CC(style, fill));
		if (style->stroke != 0)
			NEMOSHOW_CANVAS_CC(canvas, canvas)->drawArc(rect, item->from, item->to - item->from, false, *NEMOSHOW_ITEM_CC(style, stroke));
	} else if (type == NEMOSHOW_PIE_ITEM) {
		SkRect rect = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);

		if (style->fill != 0)
			NEMOSHOW_CANVAS_CC(canvas, canvas)->drawArc(rect, item->from, item->to - item->from, true, *NEMOSHOW_ITEM_CC(style, fill));
		if (style->stroke != 0)
			NEMOSHOW_CANVAS_CC(canvas, canvas)->drawArc(rect, item->from, item->to - item->from, true, *NEMOSHOW_ITEM_CC(style, stroke));
	} else if (type == NEMOSHOW_PATH_ITEM) {
		if (style->fill != 0)
			NEMOSHOW_CANVAS_CC(canvas, canvas)->drawPath(
					*NEMOSHOW_ITEM_CC(item, path),
					*NEMOSHOW_ITEM_CC(style, fill));
		if (style->stroke != 0)
			NEMOSHOW_CANVAS_CC(canvas, canvas)->drawPath(
					*NEMOSHOW_ITEM_CC(item, path),
					*NEMOSHOW_ITEM_CC(style, stroke));
	} else if (type == NEMOSHOW_TEXT_ITEM) {
		if (item->path == NULL) {
			if (item->font->layout == NEMOSHOW_NORMAL_LAYOUT) {
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
			} else if (item->font->layout == NEMOSHOW_HARFBUZZ_LAYOUT) {
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
						*NEMOSHOW_ITEM_CC(NEMOSHOW_ITEM(item->path), path),
						NULL,
						*NEMOSHOW_ITEM_CC(style, fill));
			if (style->stroke != 0)
				NEMOSHOW_CANVAS_CC(canvas, canvas)->drawTextOnPath(
						item->text,
						strlen(item->text),
						*NEMOSHOW_ITEM_CC(NEMOSHOW_ITEM(item->path), path),
						NULL,
						*NEMOSHOW_ITEM_CC(style, stroke));
		}
	}
}

static inline void nemoshow_canvas_render_loop(struct nemoshow *show, struct showcanvas *canvas, struct showone *one)
{
	struct showloop *loop = NEMOSHOW_LOOP(one);
	struct showone *child;
	int i, j;

	for (i = loop->begin; i <= loop->end; i++) {
		for (j = 0; j < one->nchildren; j++) {
			child = one->children[j];

			nemoshow_update_symbol(show, one->id, i);
			nemoshow_update_one_expression(show, child);

			if (child->type == NEMOSHOW_ITEM_TYPE) {
				struct showitem *item = NEMOSHOW_ITEM(child);
				struct showitem *style = NEMOSHOW_ITEM(item->style);

				nemoshow_item_update(show, child);

				if (item->matrix != NULL) {
					NEMOSHOW_CANVAS_CC(canvas, canvas)->save();
					NEMOSHOW_CANVAS_CC(canvas, canvas)->concat(*NEMOSHOW_MATRIX_CC(NEMOSHOW_MATRIX(item->matrix), matrix));

					nemoshow_canvas_render_item(show, canvas, child->sub, item, style);

					NEMOSHOW_CANVAS_CC(canvas, canvas)->restore();
				} else if (NEMOSHOW_ITEM_CC(item, matrix) != NULL) {
					NEMOSHOW_CANVAS_CC(canvas, canvas)->save();
					NEMOSHOW_CANVAS_CC(canvas, canvas)->concat(*NEMOSHOW_ITEM_CC(item, matrix));

					nemoshow_canvas_render_item(show, canvas, child->sub, item, style);

					NEMOSHOW_CANVAS_CC(canvas, canvas)->restore();
				} else {
					nemoshow_canvas_render_item(show, canvas, child->sub, item, style);
				}
			}
		}
	}
}

void nemoshow_canvas_render_vector(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);
	struct showone *child;
	int i;

	if (canvas->needs_full_redraw == 0) {
		NEMOSHOW_CANVAS_CC(canvas, canvas)->save();
		NEMOSHOW_CANVAS_CC(canvas, canvas)->clipRegion(*NEMOSHOW_CANVAS_CC(canvas, damage));

		NEMOSHOW_CANVAS_CC(canvas, canvas)->clear(SK_ColorTRANSPARENT);

		NEMOSHOW_CANVAS_CC(canvas, canvas)->scale(canvas->viewport.sx, canvas->viewport.sy);

		for (i = 0; i < one->nchildren; i++) {
			child = one->children[i];

			if (child->type == NEMOSHOW_ITEM_TYPE) {
				struct showitem *item = NEMOSHOW_ITEM(child);
				struct showitem *style = NEMOSHOW_ITEM(item->style);

				if (item->matrix != NULL) {
					NEMOSHOW_CANVAS_CC(canvas, canvas)->save();
					NEMOSHOW_CANVAS_CC(canvas, canvas)->concat(*NEMOSHOW_MATRIX_CC(NEMOSHOW_MATRIX(item->matrix), matrix));

					nemoshow_canvas_render_item(show, canvas, child->sub, item, style);

					NEMOSHOW_CANVAS_CC(canvas, canvas)->restore();
				} else if (NEMOSHOW_ITEM_CC(item, matrix) != NULL) {
					NEMOSHOW_CANVAS_CC(canvas, canvas)->save();
					NEMOSHOW_CANVAS_CC(canvas, canvas)->concat(*NEMOSHOW_ITEM_CC(item, matrix));

					nemoshow_canvas_render_item(show, canvas, child->sub, item, style);

					NEMOSHOW_CANVAS_CC(canvas, canvas)->restore();
				} else {
					nemoshow_canvas_render_item(show, canvas, child->sub, item, style);
				}
			} else if (child->type == NEMOSHOW_LOOP_TYPE) {
				nemoshow_canvas_render_loop(show, canvas, child);
			}
		}

		NEMOSHOW_CANVAS_CC(canvas, canvas)->restore();
	} else {
		nemotale_node_damage_all(canvas->node);

		NEMOSHOW_CANVAS_CC(canvas, canvas)->save();

		NEMOSHOW_CANVAS_CC(canvas, canvas)->clear(SK_ColorTRANSPARENT);

		NEMOSHOW_CANVAS_CC(canvas, canvas)->scale(canvas->viewport.sx, canvas->viewport.sy);

		for (i = 0; i < one->nchildren; i++) {
			child = one->children[i];

			if (child->type == NEMOSHOW_ITEM_TYPE) {
				struct showitem *item = NEMOSHOW_ITEM(child);
				struct showitem *style = NEMOSHOW_ITEM(item->style);

				if (item->matrix != NULL) {
					NEMOSHOW_CANVAS_CC(canvas, canvas)->save();
					NEMOSHOW_CANVAS_CC(canvas, canvas)->concat(*NEMOSHOW_MATRIX_CC(NEMOSHOW_MATRIX(item->matrix), matrix));

					nemoshow_canvas_render_item(show, canvas, child->sub, item, style);

					NEMOSHOW_CANVAS_CC(canvas, canvas)->restore();
				} else if (NEMOSHOW_ITEM_CC(item, matrix) != NULL) {
					NEMOSHOW_CANVAS_CC(canvas, canvas)->save();
					NEMOSHOW_CANVAS_CC(canvas, canvas)->concat(*NEMOSHOW_ITEM_CC(item, matrix));

					nemoshow_canvas_render_item(show, canvas, child->sub, item, style);

					NEMOSHOW_CANVAS_CC(canvas, canvas)->restore();
				} else {
					nemoshow_canvas_render_item(show, canvas, child->sub, item, style);
				}
			} else if (child->type == NEMOSHOW_LOOP_TYPE) {
				nemoshow_canvas_render_loop(show, canvas, child);
			}
		}

		NEMOSHOW_CANVAS_CC(canvas, canvas)->restore();

		canvas->needs_full_redraw = 0;
	}

	NEMOSHOW_CANVAS_CC(canvas, damage)->setEmpty();
}

void nemoshow_canvas_render_back(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_fill_pixman(canvas->node, canvas->fills[2], canvas->fills[1], canvas->fills[0], canvas->fills[3] * canvas->alpha);
}

void nemoshow_canvas_render_scene(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_composite_fbo_full(canvas->tale);
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

		one->dirty = 1;

		canvas->needs_full_redraw = 1;
	}

	return 0;
}

static inline void nemoshow_canvas_dirty_one(struct nemoshow *show, struct showcanvas *canvas, struct showone *child)
{
	if (child->type != NEMOSHOW_LOOP_TYPE) {
		NEMOSHOW_CANVAS_CC(canvas, damage)->op(
				SkIRect::MakeXYWH(
					child->x * canvas->viewport.sx,
					child->y * canvas->viewport.sy,
					child->width * canvas->viewport.sx,
					child->height * canvas->viewport.sy),
				SkRegion::kUnion_Op);

		nemotale_node_damage(canvas->node, child->x, child->y, child->width, child->height);
	} else {
		struct showloop *loop = NEMOSHOW_LOOP(child);
		struct showone *one;
		int i, j;

		for (i = loop->begin; i <= loop->end; i++) {
			for (j = 0; j < child->nchildren; j++) {
				one = child->children[j];

				nemoshow_update_symbol(show, child->id, i);
				nemoshow_update_one_expression(show, one);

				nemoshow_item_update(show, one);

				NEMOSHOW_CANVAS_CC(canvas, damage)->op(
						SkIRect::MakeXYWH(
							one->x * canvas->viewport.sx,
							one->y * canvas->viewport.sy,
							one->width * canvas->viewport.sx,
							one->height * canvas->viewport.sy),
						SkRegion::kUnion_Op);

				nemotale_node_damage(canvas->node, one->x, one->y, one->width, one->height);
			}
		}
	}
}

void nemoshow_canvas_dirty(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);
	struct showone *child;
	int i;

	for (i = 0; i < one->nchildren; i++) {
		child = one->children[i];

		if (child->dirty != 0) {
			nemoshow_canvas_dirty_one(show, canvas, child);

			child->update(show, child);

			nemoshow_canvas_dirty_one(show, canvas, child);

			child->dirty = 0;
		}
	}
}
