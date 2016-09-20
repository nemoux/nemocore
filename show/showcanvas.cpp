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
#include <showpath.hpp>
#include <showfont.h>
#include <showfont.hpp>
#include <fbohelper.h>
#include <oshelper.h>
#include <nemoshow.h>
#include <nemobox.h>
#include <nemometro.h>
#include <nemolog.h>
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
	NEMOSHOW_CANVAS_CC(canvas, matrix) = new SkMatrix;
	NEMOSHOW_CANVAS_CC(canvas, inverse) = new SkMatrix;
	NEMOSHOW_CANVAS_CC(canvas, bitmap) = NULL;
	NEMOSHOW_CANVAS_CC(canvas, damage) = NULL;
	NEMOSHOW_CANVAS_CC(canvas, picture) = NULL;

	canvas->viewport.sx = 1.0f;
	canvas->viewport.sy = 1.0f;
	canvas->viewport.rx = 1.0f;
	canvas->viewport.ry = 1.0f;

	canvas->sx = 1.0f;
	canvas->sy = 1.0f;
	canvas->px = 0.5f;
	canvas->py = 0.5f;

	canvas->alpha = 1.0f;

	canvas->prepare_render = nemoshow_canvas_prepare_none;
	canvas->finish_render = nemoshow_canvas_finish_none;
	canvas->dispatch_redraw = nemoshow_canvas_render_none;
	canvas->dispatch_record = NULL;
	canvas->dispatch_replay = NULL;

	nemolist_init(&canvas->redraw_link);

	one = &canvas->base;
	one->type = NEMOSHOW_CANVAS_TYPE;
	one->update = nemoshow_canvas_update;
	one->destroy = nemoshow_canvas_destroy;
	one->attach = nemoshow_canvas_attach_one;
	one->detach = nemoshow_canvas_detach_one;
	one->above = nemoshow_canvas_above_one;
	one->below = nemoshow_canvas_below_one;

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

	nemoshow_one_finish(one);

	nemolist_remove(&canvas->redraw_link);

	nemotale_node_destroy(canvas->node);

	if (NEMOSHOW_CANVAS_CC(canvas, matrix) != NULL)
		delete NEMOSHOW_CANVAS_CC(canvas, matrix);
	if (NEMOSHOW_CANVAS_CC(canvas, inverse) != NULL)
		delete NEMOSHOW_CANVAS_CC(canvas, inverse);
	if (NEMOSHOW_CANVAS_CC(canvas, damage) != NULL)
		delete NEMOSHOW_CANVAS_CC(canvas, damage);
	if (NEMOSHOW_CANVAS_CC(canvas, bitmap) != NULL)
		delete NEMOSHOW_CANVAS_CC(canvas, bitmap);

	delete static_cast<showcanvas_t *>(canvas->cc);

	free(canvas);
}

void nemoshow_canvas_attach_one(struct showone *parent, struct showone *one)
{
	nemoshow_one_dirty(parent, NEMOSHOW_CHILDREN_DIRTY);

	nemoshow_one_attach_one(parent, one);
}

void nemoshow_canvas_detach_one(struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	if (one->parent != NULL)
		nemoshow_one_dirty(one->parent, NEMOSHOW_CHILDREN_DIRTY);

	nemoshow_one_detach_one(one);
}

int nemoshow_canvas_above_one(struct showone *one, struct showone *above)
{
	if (above != NULL && one->parent != above->parent)
		return -1;

	nemoshow_one_dirty(one->parent, NEMOSHOW_CHILDREN_DIRTY);

	nemoshow_one_above_one(one, above);

	return 0;
}

int nemoshow_canvas_below_one(struct showone *one, struct showone *below)
{
	if (below != NULL && one->parent != below->parent)
		return -1;

	nemoshow_one_dirty(one->parent, NEMOSHOW_CHILDREN_DIRTY);

	nemoshow_one_below_one(one, below);

	return 0;
}

static int nemoshow_canvas_compare(const void *a, const void *b)
{
	return strcasecmp((const char *)a, (const char *)b);
}

int nemoshow_canvas_set_type(struct showone *one, int type)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	if (type == NEMOSHOW_CANVAS_VECTOR_TYPE) {
#ifdef NEMOUX_WITH_OPENGL_PBO
		canvas->node = nemotale_node_create_gl(canvas->width, canvas->height);
#else
		canvas->node = nemotale_node_create_pixman(canvas->width, canvas->height);
		nemotale_node_prepare_gl(canvas->node);
#endif

		NEMOSHOW_CANVAS_CC(canvas, damage) = new SkRegion;

		canvas->prepare_render = nemoshow_canvas_prepare_vector;
		canvas->finish_render = nemoshow_canvas_finish_vector;
		canvas->dispatch_redraw = nemoshow_canvas_render_vector;
		canvas->dispatch_record = nemoshow_canvas_record_vector;
		canvas->dispatch_replay = nemoshow_canvas_replay_vector;

		nemoshow_canvas_set_state(canvas, NEMOSHOW_CANVAS_POOLING_STATE);
	} else if (type == NEMOSHOW_CANVAS_PIPELINE_TYPE) {
		canvas->node = nemotale_node_create_gl(canvas->width, canvas->height);

		canvas->dispatch_redraw = nemoshow_canvas_render_pipeline;
	} else if (type == NEMOSHOW_CANVAS_PIXMAN_TYPE) {
		canvas->node = nemotale_node_create_pixman(canvas->width, canvas->height);
		nemotale_node_prepare_gl(canvas->node);
	} else if (type == NEMOSHOW_CANVAS_OPENGL_TYPE) {
		canvas->node = nemotale_node_create_gl(canvas->width, canvas->height);
	} else if (type == NEMOSHOW_CANVAS_BACK_TYPE) {
		canvas->node = nemotale_node_create_pixman(canvas->width, canvas->height);
		nemotale_node_prepare_gl(canvas->node);
		nemotale_node_opaque(canvas->node, 0, 0, canvas->width, canvas->height);

		canvas->dispatch_redraw = nemoshow_canvas_render_back;
	}

	one->sub = type;

	nemotale_node_set_data(canvas->node, one);

	return 0;
}

void nemoshow_canvas_set_alpha(struct showone *one, double alpha)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	canvas->alpha = alpha;

	nemoshow_one_dirty(one, NEMOSHOW_STYLE_DIRTY);
}

int nemoshow_canvas_set_shader(struct showone *one, const char *shader)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_set_filter(canvas->node, shader);

	nemoshow_one_dirty(one, NEMOSHOW_FILTER_DIRTY);

	return 0;
}

int nemoshow_canvas_load_shader(struct showone *one, const char *shaderpath)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);
	char *shader;

	if (os_load_path(shaderpath, &shader, NULL) < 0)
		return -1;

	nemotale_node_set_filter(canvas->node, shader);

	free(shader);

	nemoshow_one_dirty(one, NEMOSHOW_FILTER_DIRTY);

	return 0;
}

static inline void nemoshow_canvas_update_size(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	if (one->sub == NEMOSHOW_CANVAS_VECTOR_TYPE) {
		canvas->viewport.sx = canvas->viewport.width / canvas->width;
		canvas->viewport.sy = canvas->viewport.height / canvas->height;
		canvas->viewport.rx = canvas->width / canvas->viewport.width;
		canvas->viewport.ry = canvas->height / canvas->viewport.height;
	}

	one->dirty |= NEMOSHOW_VIEWPORT_DIRTY;
}

static inline void nemoshow_canvas_update_viewport(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	if (one->parent != NULL && one->parent->type == NEMOSHOW_SCENE_TYPE) {
		struct showone *scene = one->parent;

		nemoshow_canvas_set_viewport(one,
				(double)show->width / (double)NEMOSHOW_SCENE_AT(scene, width),
				(double)show->height / (double)NEMOSHOW_SCENE_AT(scene, height));

		if (canvas->dispatch_resize != NULL)
			canvas->dispatch_resize(show, one, canvas->viewport.width, canvas->viewport.height);
	} else {
		nemoshow_canvas_set_viewport(one, 1.0f, 1.0f);
	}
}

static inline void nemoshow_canvas_update_matrix(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	NEMOSHOW_CANVAS_CC(canvas, matrix)->setIdentity();

	if (canvas->ro != 0.0f) {
		NEMOSHOW_CANVAS_CC(canvas, matrix)->postTranslate(-canvas->width * canvas->px, -canvas->height * canvas->py);
		NEMOSHOW_CANVAS_CC(canvas, matrix)->postRotate(canvas->ro);
		NEMOSHOW_CANVAS_CC(canvas, matrix)->postTranslate(canvas->width * canvas->px, canvas->height * canvas->py);
	}

	if (canvas->sx != 1.0f || canvas->sy != 1.0f) {
		NEMOSHOW_CANVAS_CC(canvas, matrix)->postTranslate(-canvas->width * canvas->px, -canvas->height * canvas->py);
		NEMOSHOW_CANVAS_CC(canvas, matrix)->postScale(canvas->sx, canvas->sy);
		NEMOSHOW_CANVAS_CC(canvas, matrix)->postTranslate(canvas->width * canvas->px, canvas->height * canvas->py);
	}

	NEMOSHOW_CANVAS_CC(canvas, matrix)->postTranslate(canvas->tx, canvas->ty);

	NEMOSHOW_CANVAS_CC(canvas, matrix)->invert(NEMOSHOW_CANVAS_CC(canvas, inverse));

	nemotale_node_translate(canvas->node, canvas->tx * canvas->viewport.sx, canvas->ty * canvas->viewport.sy);
	nemotale_node_rotate(canvas->node, canvas->ro * M_PI / 180.0f);
	nemotale_node_scale(canvas->node, canvas->sx, canvas->sy);
	nemotale_node_pivot(canvas->node, canvas->px, canvas->py);
}

int nemoshow_canvas_update(struct showone *one)
{
	struct nemoshow *show = one->show;
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	if ((one->dirty & NEMOSHOW_SIZE_DIRTY) != 0) {
		nemoshow_canvas_update_size(show, one);
	}
	if ((one->dirty & NEMOSHOW_VIEWPORT_DIRTY) != 0) {
		nemoshow_canvas_update_viewport(show, one);
	}
	if ((one->dirty & NEMOSHOW_STYLE_DIRTY) != 0) {
		nemotale_node_set_alpha(canvas->node, canvas->alpha);
	}
	if ((one->dirty & NEMOSHOW_MATRIX_DIRTY) != 0) {
		nemoshow_canvas_update_matrix(show, one);
	}
	if ((one->dirty & NEMOSHOW_REDRAW_DIRTY) != 0) {
		nemotale_node_damage_all(canvas->node);

		nemoshow_canvas_set_state(canvas, NEMOSHOW_CANVAS_REDRAW_STATE | NEMOSHOW_CANVAS_REDRAW_FULL_STATE);
	}
	if ((one->dirty & NEMOSHOW_FILTER_DIRTY) != 0) {
		nemotale_node_damage_filter(canvas->node);
	}

	if (nemolist_empty(&canvas->redraw_link) != 0)
		nemolist_insert(&show->redraw_list, &canvas->redraw_link);

	return 0;
}

static inline void nemoshow_canvas_update_geometry(struct showone *one)
{
	if ((one->dirty & NEMOSHOW_SIZE_DIRTY) != 0) {
		nemoshow_canvas_update_size(one->show, one);

		one->dirty &= ~NEMOSHOW_SIZE_DIRTY;
	}
	if ((one->dirty & NEMOSHOW_VIEWPORT_DIRTY) != 0) {
		nemoshow_canvas_update_viewport(one->show, one);

		one->dirty &= ~NEMOSHOW_VIEWPORT_DIRTY;
	}
	if ((one->dirty & NEMOSHOW_MATRIX_DIRTY) != 0) {
		nemoshow_canvas_update_matrix(one->show, one);

		one->dirty &= ~NEMOSHOW_MATRIX_DIRTY;
	}
}

static inline void nemoshow_canvas_render_one(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one, SkRegion *region);

static inline void nemoshow_canvas_render_item_none(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one, SkRegion *region)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
}

static inline void nemoshow_canvas_render_item_line(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one, SkRegion *region)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (nemoshow_one_has_state(one, NEMOSHOW_FILL_STATE))
		_canvas->drawLine(item->x, item->y, item->width, item->height, *NEMOSHOW_ITEM_CC(item, fill));
	if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE))
		_canvas->drawLine(item->x, item->y, item->width, item->height, *NEMOSHOW_ITEM_CC(item, stroke));
}

static inline void nemoshow_canvas_render_item_rect(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one, SkRegion *region)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	SkRect rect = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);

	if (nemoshow_one_has_state(one, NEMOSHOW_FILL_STATE))
		_canvas->drawRect(rect, *NEMOSHOW_ITEM_CC(item, fill));
	if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE))
		_canvas->drawRect(rect, *NEMOSHOW_ITEM_CC(item, stroke));
}

static inline void nemoshow_canvas_render_item_rrect(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one, SkRegion *region)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	SkRect rect = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);

	if (nemoshow_one_has_state(one, NEMOSHOW_FILL_STATE))
		_canvas->drawRoundRect(rect, item->ox, item->oy, *NEMOSHOW_ITEM_CC(item, fill));
	if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE))
		_canvas->drawRoundRect(rect, item->ox, item->oy, *NEMOSHOW_ITEM_CC(item, stroke));
}

static inline void nemoshow_canvas_render_item_circle(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one, SkRegion *region)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (nemoshow_one_has_state(one, NEMOSHOW_FILL_STATE))
		_canvas->drawCircle(item->cx, item->cy, item->r, *NEMOSHOW_ITEM_CC(item, fill));
	if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE))
		_canvas->drawCircle(item->cx, item->cy, item->r, *NEMOSHOW_ITEM_CC(item, stroke));
}

static inline void nemoshow_canvas_render_item_arc(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one, SkRegion *region)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	SkRect rect = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);

	if (nemoshow_one_has_state(one, NEMOSHOW_FILL_STATE))
		_canvas->drawArc(rect, item->from, item->to - item->from, false, *NEMOSHOW_ITEM_CC(item, fill));
	if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE))
		_canvas->drawArc(rect, item->from, item->to - item->from, false, *NEMOSHOW_ITEM_CC(item, stroke));
}

static inline void nemoshow_canvas_render_item_text(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one, SkRegion *region)
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

static inline void nemoshow_canvas_render_item_textbox(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one, SkRegion *region)
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

static inline void nemoshow_canvas_render_item_path(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one, SkRegion *region)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (item->from == 0.0f && item->to == 1.0f) {
		if (nemoshow_one_has_state(one, NEMOSHOW_FILL_STATE))
			_canvas->drawPath(*NEMOSHOW_ITEM_CC(item, path), *NEMOSHOW_ITEM_CC(item, fill));
		if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE))
			_canvas->drawPath(*NEMOSHOW_ITEM_CC(item, path), *NEMOSHOW_ITEM_CC(item, stroke));
	} else {
		_canvas->drawPath(*NEMOSHOW_ITEM_CC(item, subpath), *NEMOSHOW_ITEM_CC(item, stroke));
	}
}

static inline void nemoshow_canvas_render_item_pathtwice(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one, SkRegion *region)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (item->from == 0.0f && item->to == 1.0f) {
		if (nemoshow_one_has_state(one, NEMOSHOW_FILL_STATE))
			_canvas->drawPath(*NEMOSHOW_ITEM_CC(item, fillpath), *NEMOSHOW_ITEM_CC(item, fill));
		if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE))
			_canvas->drawPath(*NEMOSHOW_ITEM_CC(item, path), *NEMOSHOW_ITEM_CC(item, stroke));
	} else {
		_canvas->drawPath(*NEMOSHOW_ITEM_CC(item, subpath), *NEMOSHOW_ITEM_CC(item, stroke));
	}
}

static inline void nemoshow_canvas_render_item_patharray(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one, SkRegion *region)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (item->from == 0.0f && item->to == 1.0f) {
		if (nemoshow_one_has_state(one, NEMOSHOW_FILL_STATE))
			_canvas->drawPath(*NEMOSHOW_ITEM_CC(item, path), *NEMOSHOW_ITEM_CC(item, fill));
		if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE))
			_canvas->drawPath(*NEMOSHOW_ITEM_CC(item, path), *NEMOSHOW_ITEM_CC(item, stroke));
	} else {
		_canvas->drawPath(*NEMOSHOW_ITEM_CC(item, subpath), *NEMOSHOW_ITEM_CC(item, stroke));
	}
}

static inline void nemoshow_canvas_render_item_pathlist(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one, SkRegion *region)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (item->from == 0.0f && item->to == 1.0f) {
		if (nemoshow_one_has_state(one, NEMOSHOW_FILL_STATE))
			_canvas->drawPath(*NEMOSHOW_ITEM_CC(item, path), *NEMOSHOW_ITEM_CC(item, fill));
		if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE))
			_canvas->drawPath(*NEMOSHOW_ITEM_CC(item, path), *NEMOSHOW_ITEM_CC(item, stroke));
	} else {
		_canvas->drawPath(*NEMOSHOW_ITEM_CC(item, subpath), *NEMOSHOW_ITEM_CC(item, stroke));
	}
}

static inline void nemoshow_canvas_render_item_pathgroup(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one, SkRegion *region)
{
	struct showitem *item = NEMOSHOW_ITEM(one);
	struct showone *child;
	struct showpath *path;

	nemoshow_children_for_each(child, one) {
		path = NEMOSHOW_PATH(child);

		if (nemoshow_one_has_state(child, NEMOSHOW_FILL_STATE))
			_canvas->drawPath(*NEMOSHOW_PATH_CC(path, path), *NEMOSHOW_PATH_CC(path, fill));
		if (nemoshow_one_has_state(child, NEMOSHOW_STROKE_STATE))
			_canvas->drawPath(*NEMOSHOW_PATH_CC(path, path), *NEMOSHOW_PATH_CC(path, stroke));
	}
}

static inline void nemoshow_canvas_render_item_points(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one, SkRegion *region)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (nemoshow_one_has_state(one, NEMOSHOW_FILL_STATE))
		_canvas->drawPoints(SkCanvas::kPoints_PointMode, item->npoints / 2, NEMOSHOW_ITEM_CC(item, points), *NEMOSHOW_ITEM_CC(item, fill));
	if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE))
		_canvas->drawPoints(SkCanvas::kPoints_PointMode, item->npoints / 2, NEMOSHOW_ITEM_CC(item, points), *NEMOSHOW_ITEM_CC(item, stroke));
}

static inline void nemoshow_canvas_render_item_polyline(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one, SkRegion *region)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (nemoshow_one_has_state(one, NEMOSHOW_FILL_STATE))
		_canvas->drawPoints(SkCanvas::kLines_PointMode, item->npoints / 2, NEMOSHOW_ITEM_CC(item, points), *NEMOSHOW_ITEM_CC(item, fill));
	if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE))
		_canvas->drawPoints(SkCanvas::kLines_PointMode, item->npoints / 2, NEMOSHOW_ITEM_CC(item, points), *NEMOSHOW_ITEM_CC(item, stroke));
}

static inline void nemoshow_canvas_render_item_polygon(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one, SkRegion *region)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	if (nemoshow_one_has_state(one, NEMOSHOW_FILL_STATE))
		_canvas->drawPoints(SkCanvas::kPolygon_PointMode, item->npoints / 2, NEMOSHOW_ITEM_CC(item, points), *NEMOSHOW_ITEM_CC(item, fill));
	if (nemoshow_one_has_state(one, NEMOSHOW_STROKE_STATE))
		_canvas->drawPoints(SkCanvas::kPolygon_PointMode, item->npoints / 2, NEMOSHOW_ITEM_CC(item, points), *NEMOSHOW_ITEM_CC(item, stroke));
}

static inline void nemoshow_canvas_render_item_image(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one, SkRegion *region)
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

static inline void nemoshow_canvas_render_item_group(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one, SkRegion *region)
{
	struct showone *child;

	nemoshow_children_for_each(child, one) {
		nemoshow_canvas_render_one(canvas, _canvas, child, region);
	}
}

static inline void nemoshow_canvas_render_item_container(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one, SkRegion *region)
{
	struct showone *child;

	nemoshow_children_for_each(child, one) {
		nemoshow_canvas_render_one(canvas, _canvas, child, region);
	}
}

typedef void (*nemoshow_canvas_render_t)(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one, SkRegion *region);

static inline void nemoshow_canvas_render_one(struct showcanvas *canvas, SkCanvas *_canvas, struct showone *one, SkRegion *region)
{
	static nemoshow_canvas_render_t renderers[NEMOSHOW_LAST_ITEM] = {
		nemoshow_canvas_render_item_none,
		nemoshow_canvas_render_item_line,
		nemoshow_canvas_render_item_rect,
		nemoshow_canvas_render_item_rrect,
		nemoshow_canvas_render_item_circle,
		nemoshow_canvas_render_item_arc,
		nemoshow_canvas_render_item_text,
		nemoshow_canvas_render_item_textbox,
		nemoshow_canvas_render_item_path,
		nemoshow_canvas_render_item_pathtwice,
		nemoshow_canvas_render_item_patharray,
		nemoshow_canvas_render_item_pathlist,
		nemoshow_canvas_render_item_pathgroup,
		nemoshow_canvas_render_item_points,
		nemoshow_canvas_render_item_polyline,
		nemoshow_canvas_render_item_polygon,
		nemoshow_canvas_render_item_image,
		nemoshow_canvas_render_item_group,
		nemoshow_canvas_render_item_container
	};

	if (region != NULL && region->intersects(SkIRect::MakeXYWH(one->sx, one->sy, one->sw, one->sh)) == false)
		return;

	if (nemoshow_one_has_state(one, NEMOSHOW_TRANSFORM_STATE | NEMOSHOW_CLIP_STATE)) {
		struct showitem *item = NEMOSHOW_ITEM(one);

		_canvas->save();

		if (nemoshow_one_has_state(one, NEMOSHOW_TRANSFORM_STATE))
			_canvas->concat(*NEMOSHOW_ITEM_CC(item, modelview));

		if (nemoshow_one_has_state(one, NEMOSHOW_CLIP_STATE)) {
			struct showone *ref = NEMOSHOW_REF(one, NEMOSHOW_CLIP_REF);
			struct showitem *clip = NEMOSHOW_ITEM(ref);

			if (nemoshow_one_has_state(ref, NEMOSHOW_TRANSFORM_STATE)) {
				SkMatrix matrix = _canvas->getTotalMatrix();

				_canvas->resetMatrix();
				_canvas->scale(
						nemoshow_canvas_get_viewport_sx(one->canvas),
						nemoshow_canvas_get_viewport_sy(one->canvas));
				_canvas->concat(*NEMOSHOW_ITEM_CC(clip, modelview));

				_canvas->clipPath(*NEMOSHOW_ITEM_CC(clip, path));

				_canvas->setMatrix(matrix);
			} else {
				_canvas->clipPath(*NEMOSHOW_ITEM_CC(clip, path));
			}
		}

		renderers[one->sub](canvas, _canvas, one, region);

		_canvas->restore();
	} else {
		renderers[one->sub](canvas, _canvas, one, region);
	}
}

int nemoshow_canvas_prepare_vector(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	NEMOSHOW_CANVAS_CC(canvas, bitmap) = new SkBitmap;
	NEMOSHOW_CANVAS_CC(canvas, bitmap)->setInfo(
			SkImageInfo::Make(canvas->viewport.width, canvas->viewport.height, kN32_SkColorType, kPremul_SkAlphaType));
	NEMOSHOW_CANVAS_CC(canvas, bitmap)->setPixels(
			nemotale_node_map(canvas->node));

	return 0;
}

void nemoshow_canvas_finish_vector(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	delete NEMOSHOW_CANVAS_CC(canvas, bitmap);
	NEMOSHOW_CANVAS_CC(canvas, bitmap) = NULL;

	nemoshow_canvas_put_state(canvas, NEMOSHOW_CANVAS_REDRAW_STATE | NEMOSHOW_CANVAS_REDRAW_FULL_STATE);

	NEMOSHOW_CANVAS_CC(canvas, damage)->setEmpty();

	nemotale_node_unmap(canvas->node);
	nemotale_node_flush(canvas->node);
}

void nemoshow_canvas_render_vector(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);
	struct showone *child;
	SkBitmapDevice _device(*NEMOSHOW_CANVAS_CC(canvas, bitmap));
	SkCanvas _canvas(&_device);

	if (nemoshow_canvas_has_state(canvas, NEMOSHOW_CANVAS_REDRAW_FULL_STATE)) {
		_canvas.clear(SK_ColorTRANSPARENT);
		_canvas.scale(canvas->viewport.sx, canvas->viewport.sy);

		nemoshow_children_for_each(child, one) {
			nemoshow_canvas_render_one(canvas, &_canvas, child, NULL);
		}
	} else {
		_canvas.clipRegion(*NEMOSHOW_CANVAS_CC(canvas, damage));
		_canvas.clear(SK_ColorTRANSPARENT);
		_canvas.scale(canvas->viewport.sx, canvas->viewport.sy);

		nemoshow_children_for_each(child, one) {
			nemoshow_canvas_render_one(canvas, &_canvas, child, NEMOSHOW_CANVAS_CC(canvas, damage));
		}
	}
}

void nemoshow_canvas_record_vector(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);
	struct showone *child;
	SkPictureRecorder recorder;
	SkCanvas *_canvas;

	_canvas = recorder.beginRecording(canvas->viewport.width, canvas->viewport.height);

	_canvas->clear(SK_ColorTRANSPARENT);
	_canvas->scale(canvas->viewport.sx, canvas->viewport.sy);

	nemoshow_children_for_each(child, one) {
		nemoshow_canvas_render_one(canvas, _canvas, child, NULL);
	}

	NEMOSHOW_CANVAS_CC(canvas, picture) = recorder.finishRecordingAsPicture();
}

void nemoshow_canvas_replay_vector(struct nemoshow *show, struct showone *one, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);
	SkBitmapDevice _device(*NEMOSHOW_CANVAS_CC(canvas, bitmap));
	SkCanvas _canvas(&_device);
	SkRegion region;

	if (nemoshow_canvas_has_state(canvas, NEMOSHOW_CANVAS_REDRAW_FULL_STATE))
		region.setRect(SkIRect::MakeXYWH(x, y, width, height));
	else
		region.op(*NEMOSHOW_CANVAS_CC(canvas, damage), SkIRect::MakeXYWH(x, y, width, height), SkRegion::kIntersect_Op);

	_canvas.clipRegion(region);
	_canvas.drawPicture(NEMOSHOW_CANVAS_CC(canvas, picture));
}

void nemoshow_canvas_render_back(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_fill_pixman(canvas->node, canvas->fills[2], canvas->fills[1], canvas->fills[0], canvas->fills[3]);
}

int nemoshow_canvas_prepare_none(struct nemoshow *show, struct showone *one)
{
	return 0;
}

void nemoshow_canvas_finish_none(struct nemoshow *show, struct showone *one)
{
}

void nemoshow_canvas_render_none(struct nemoshow *show, struct showone *one)
{
}

int nemoshow_canvas_redraw(struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	if (one->sub == NEMOSHOW_CANVAS_VECTOR_TYPE)
		nemoshow_canvas_render_vector(one->show, one);
	else if (one->sub == NEMOSHOW_CANVAS_PIPELINE_TYPE)
		nemoshow_canvas_render_pipeline(one->show, one);
	else if (one->sub == NEMOSHOW_CANVAS_BACK_TYPE)
		nemoshow_canvas_render_back(one->show, one);

	return 0;
}

int nemoshow_canvas_set_viewport(struct showone *one, double sx, double sy)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	canvas->viewport.sx = sx;
	canvas->viewport.sy = sy;
	canvas->viewport.rx = 1.0f / sx;
	canvas->viewport.ry = 1.0f / sy;

	canvas->viewport.width = canvas->width * sx;
	canvas->viewport.height = canvas->height * sy;

	nemotale_node_translate(canvas->node, canvas->tx * canvas->viewport.sx, canvas->ty * canvas->viewport.sy);
	nemotale_node_resize(canvas->node, canvas->viewport.width, canvas->viewport.height);

	if (one->sub == NEMOSHOW_CANVAS_VECTOR_TYPE) {
		nemoshow_one_dirty_all(one, NEMOSHOW_SHAPE_DIRTY);
	} else if (one->sub == NEMOSHOW_CANVAS_PIPELINE_TYPE) {
		fbo_prepare_context(
				nemotale_node_get_texture(canvas->node),
				canvas->viewport.width,
				canvas->viewport.height,
				&canvas->fbo, &canvas->dbo);
	} else if (one->sub == NEMOSHOW_CANVAS_BACK_TYPE) {
		nemotale_node_opaque(canvas->node, 0, 0, canvas->viewport.width, canvas->viewport.height);
	}

	nemoshow_canvas_damage_all(one);

	return 0;
}

int nemoshow_canvas_set_smooth(struct showone *one, int has_smooth)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_set_smooth(canvas->node, has_smooth);

	return 0;
}

void nemoshow_canvas_damage(struct showone *one, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_damage(canvas->node, x, y, width, height);

	nemoshow_canvas_set_state(canvas, NEMOSHOW_CANVAS_REDRAW_STATE);

	if (one->show != NULL && nemolist_empty(&canvas->redraw_link) != 0)
		nemolist_insert(&one->show->redraw_list, &canvas->redraw_link);
}

void nemoshow_canvas_damage_one(struct showone *one, struct showone *child)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	NEMOSHOW_CANVAS_CC(canvas, damage)->op(
			SkIRect::MakeXYWH(child->sx, child->sy, child->sw, child->sh),
			SkRegion::kUnion_Op);

	nemotale_node_damage(canvas->node, child->sx, child->sy, child->sw, child->sh);

	nemoshow_canvas_set_state(canvas, NEMOSHOW_CANVAS_REDRAW_STATE);

	if (one->show != NULL && nemolist_empty(&canvas->redraw_link) != 0)
		nemolist_insert(&one->show->redraw_list, &canvas->redraw_link);
}

void nemoshow_canvas_damage_all(struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_node_damage_all(canvas->node);

	nemoshow_canvas_set_state(canvas, NEMOSHOW_CANVAS_REDRAW_STATE | NEMOSHOW_CANVAS_REDRAW_FULL_STATE);

	if (one->show != NULL && nemolist_empty(&canvas->redraw_link) != 0)
		nemolist_insert(&one->show->redraw_list, &canvas->redraw_link);
}

void nemoshow_canvas_damage_below(struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_damage_below(one->show->tale, canvas->node);

	if (one->show != NULL && nemolist_empty(&canvas->redraw_link) != 0)
		nemolist_insert(&one->show->redraw_list, &canvas->redraw_link);
}

static inline struct showone *nemoshow_canvas_pick_item(struct showone *one, float x, float y)
{
	struct showone *child;
	struct showone *pick;

	nemoshow_children_for_each_reverse(child, one) {
		if (child->sub == NEMOSHOW_GROUP_ITEM) {
			if (nemoshow_one_has_state(child, NEMOSHOW_PICK_STATE)) {
				if (nemoshow_item_contain_one(child, x, y) != 0)
					return child;
			} else {
				pick = nemoshow_canvas_pick_item(child, x, y);
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

static inline struct showone *nemoshow_canvas_pick_poly(struct showone *one, float x, float y)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);
	struct showone *pone, *cone;
	struct showone *pick = NULL;
	float min = FLT_MAX;
	float t;

	nemoshow_children_for_each(pone, one) {
		nemoshow_children_for_each(cone, pone) {
			if (nemoshow_one_has_state(cone, NEMOSHOW_PICK_STATE)) {
				t = nemoshow_poly_contain_point(one, pone, cone, x, y);
				if (min > t) {
					min = t;
					pick = cone;
				}
			}
		}
	}

	return pick;
}

struct showone *nemoshow_canvas_pick_one(struct showone *one, float x, float y)
{
	if (one->sub == NEMOSHOW_CANVAS_PIPELINE_TYPE)
		return nemoshow_canvas_pick_poly(one, x, y);

	return nemoshow_canvas_pick_item(one, x, y);
}

void nemoshow_canvas_set_one(struct showone *canvas, struct showone *one)
{
	one->canvas = canvas;
}

void nemoshow_canvas_put_one(struct showone *one)
{
	if (one->canvas != NULL) {
		nemoshow_canvas_damage_one(one->canvas, one);

		one->canvas = NULL;
	}
}

void nemoshow_canvas_set_ones(struct showone *canvas, struct showone *one)
{
	struct showone *child;

	if (one->canvas == canvas)
		return;

	one->canvas = canvas;

	nemoshow_children_for_each(child, one)
		nemoshow_canvas_set_ones(canvas, child);
}

void nemoshow_canvas_put_ones(struct showone *one)
{
	struct showone *child;

	if (one->canvas != NULL) {
		nemoshow_canvas_damage_one(one->canvas, one);

		one->canvas = NULL;
	}

	nemoshow_children_for_each(child, one)
		nemoshow_canvas_put_ones(child);
}

void nemoshow_canvas_transform_to_global(struct showone *one, float sx, float sy, float *x, float *y)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemoshow_canvas_update_geometry(one);

	SkPoint p = NEMOSHOW_CANVAS_CC(canvas, matrix)->mapXY(sx, sy);

	*x = p.x();
	*y = p.y();
}

void nemoshow_canvas_transform_from_global(struct showone *one, float x, float y, float *sx, float *sy)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemoshow_canvas_update_geometry(one);

	SkPoint p = NEMOSHOW_CANVAS_CC(canvas, inverse)->mapXY(x, y);

	*sx = p.x();
	*sy = p.y();
}

void nemoshow_canvas_transform_to_viewport(struct showone *one, float x, float y, float *sx, float *sy)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemoshow_canvas_update_geometry(one);

	*sx = x * canvas->viewport.sx;
	*sy = y * canvas->viewport.sy;
}

void nemoshow_canvas_transform_from_viewport(struct showone *one, float sx, float sy, float *x, float *y)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemoshow_canvas_update_geometry(one);

	*x = sx * canvas->viewport.rx;
	*y = sy * canvas->viewport.ry;
}
