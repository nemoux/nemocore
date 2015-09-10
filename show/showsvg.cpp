#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showsvg.h>
#include <showsvg.hpp>
#include <showitem.h>
#include <showitem.hpp>
#include <showcolor.h>
#include <showmatrix.h>
#include <showmatrix.hpp>
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

struct showone *nemoshow_svg_create(void)
{
	struct showsvg *svg;
	struct showone *one;

	svg = (struct showsvg *)malloc(sizeof(struct showsvg));
	if (svg == NULL)
		return NULL;
	memset(svg, 0, sizeof(struct showsvg));

	svg->cc = new showsvg_t;
	NEMOSHOW_SVG_CC(svg, viewbox) = new SkMatrix;
	NEMOSHOW_SVG_CC(svg, matrix) = new SkMatrix;

	svg->sx = 1.0f;
	svg->sy = 1.0f;

	one = &svg->base;
	one->type = NEMOSHOW_SVG_TYPE;
	one->update = nemoshow_svg_update;
	one->destroy = nemoshow_svg_destroy;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "width", &svg->width, sizeof(double));
	nemoobject_set_reserved(&one->object, "height", &svg->height, sizeof(double));

	nemoobject_set_reserved(&one->object, "tx", &svg->tx, sizeof(double));
	nemoobject_set_reserved(&one->object, "ty", &svg->ty, sizeof(double));
	nemoobject_set_reserved(&one->object, "sx", &svg->sx, sizeof(double));
	nemoobject_set_reserved(&one->object, "sy", &svg->sy, sizeof(double));
	nemoobject_set_reserved(&one->object, "px", &svg->px, sizeof(double));
	nemoobject_set_reserved(&one->object, "py", &svg->py, sizeof(double));
	nemoobject_set_reserved(&one->object, "ro", &svg->ro, sizeof(double));

	return one;
}

void nemoshow_svg_destroy(struct showone *one)
{
	struct showsvg *svg = NEMOSHOW_SVG(one);

	nemoshow_one_unreference_all(one);

	nemoshow_one_finish(one);

	delete static_cast<showsvg_t *>(svg->cc);

	free(svg);
}

int nemoshow_svg_arrange(struct nemoshow *show, struct showone *one)
{
	struct showsvg *svg = NEMOSHOW_SVG(one);
	struct showone *matrix;
	struct showone *clip;
	const char *v;
	int i;

	v = nemoobject_gets(&one->object, "matrix");
	if (v != NULL) {
		if (strcmp(v, "tsr") == 0) {
			svg->transform = NEMOSHOW_TSR_TRANSFORM;
		} else if ((matrix = nemoshow_search_one(show, v)) != NULL) {
			svg->transform = NEMOSHOW_EXTERN_TRANSFORM;

			nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_MATRIX_REF));
			nemoshow_one_reference_one(one, matrix, NEMOSHOW_MATRIX_REF);
		}
	} else {
		for (i = 0; i < one->nchildren; i++) {
			if (one->children[i]->type == NEMOSHOW_MATRIX_TYPE) {
				svg->transform = NEMOSHOW_CHILDREN_TRANSFORM;

				break;
			}
		}
	}

	v = nemoobject_gets(&one->object, "clip");
	if (v != NULL && (clip = nemoshow_search_one(show, v)) != NULL) {
		nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_CLIP_REF));
		nemoshow_one_reference_one(one, clip, NEMOSHOW_CLIP_REF);
	}

	v = nemoobject_gets(&one->object, "uri");
	if (v != NULL)
		svg->uri = strdup(v);

	return 0;
}

static inline void nemoshow_svg_update_uri(struct nemoshow *show, struct showone *one)
{
	struct showsvg *svg = NEMOSHOW_SVG(one);

	if (svg->uri != NULL) {
		nemoshow_svg_load_uri(show, one, svg->uri);

		one->dirty |= NEMOSHOW_SHAPE_DIRTY;
	}
}

static inline void nemoshow_svg_update_child(struct nemoshow *show, struct showone *one)
{
	struct showsvg *svg = NEMOSHOW_SVG(one);
	struct showone *child;
	int i;

	if (svg->transform == NEMOSHOW_CHILDREN_TRANSFORM) {
		NEMOSHOW_SVG_CC(svg, matrix)->setIdentity();

		for (i = 0; i < one->nchildren; i++) {
			child = one->children[i];

			if (child->type == NEMOSHOW_MATRIX_TYPE) {
				NEMOSHOW_SVG_CC(svg, matrix)->postConcat(
						*NEMOSHOW_MATRIX_CC(
							NEMOSHOW_MATRIX(child),
							matrix));
			}
		}

		one->dirty |= NEMOSHOW_SHAPE_DIRTY;
	}
}

static inline void nemoshow_svg_update_matrix(struct nemoshow *show, struct showone *one)
{
	struct showsvg *svg = NEMOSHOW_SVG(one);

	if (svg->transform == NEMOSHOW_TSR_TRANSFORM) {
		NEMOSHOW_SVG_CC(svg, matrix)->setIdentity();

		if (svg->px != 0.0f || svg->py != 0.0f) {
			NEMOSHOW_SVG_CC(svg, matrix)->postTranslate(-svg->px, -svg->py);

			if (svg->ro != 0.0f) {
				NEMOSHOW_SVG_CC(svg, matrix)->postRotate(svg->ro);
			}
			if (svg->sx != 1.0f || svg->sy != 1.0f) {
				NEMOSHOW_SVG_CC(svg, matrix)->postScale(svg->sx, svg->sy);
			}

			NEMOSHOW_SVG_CC(svg, matrix)->postTranslate(svg->px, svg->py);
		} else {
			if (svg->ro != 0.0f) {
				NEMOSHOW_SVG_CC(svg, matrix)->postRotate(svg->ro);
			}
			if (svg->sx != 1.0f || svg->sy != 1.0f) {
				NEMOSHOW_SVG_CC(svg, matrix)->postScale(svg->sx, svg->sy);
			}
		}

		NEMOSHOW_SVG_CC(svg, matrix)->postTranslate(svg->tx, svg->ty);

		one->dirty |= NEMOSHOW_SHAPE_DIRTY;
	}
}

static inline void nemoshow_svg_update_boundingbox(struct nemoshow *show, struct showone *one)
{
	struct showsvg *svg = NEMOSHOW_SVG(one);
	struct showone *parent;
	SkRect box;
	char attr[NEMOSHOW_SYMBOL_MAX];

	box = SkRect::MakeXYWH(0, 0, svg->width, svg->height);

	if (svg->transform & NEMOSHOW_EXTERN_TRANSFORM) {
		NEMOSHOW_MATRIX_CC(NEMOSHOW_MATRIX(NEMOSHOW_REF(one, NEMOSHOW_MATRIX_REF)), matrix)->mapRect(&box);
	} else if (svg->transform & NEMOSHOW_INTERN_TRANSFORM) {
		NEMOSHOW_SVG_CC(svg, matrix)->mapRect(&box);
	}

	for (parent = one->parent; parent != NULL; parent = parent->parent) {
		if (parent->type == NEMOSHOW_ITEM_TYPE && parent->sub == NEMOSHOW_GROUP_ITEM) {
			struct showitem *group = NEMOSHOW_ITEM(parent);

			nemoshow_one_update_alone(show, parent);

			if (group->transform & NEMOSHOW_EXTERN_TRANSFORM) {
				NEMOSHOW_MATRIX_CC(NEMOSHOW_MATRIX(NEMOSHOW_REF(parent, NEMOSHOW_MATRIX_REF)), matrix)->mapRect(&box);
			} else if (group->transform & NEMOSHOW_INTERN_TRANSFORM) {
				NEMOSHOW_ITEM_CC(group, matrix)->mapRect(&box);
			}
		}
	}

	box.outset(
			NEMOSHOW_ANTIALIAS_EPSILON,
			NEMOSHOW_ANTIALIAS_EPSILON);

	if (svg->canvas != NULL) {
		one->x = MAX(floor(box.x()), 0);
		one->y = MAX(floor(box.y()), 0);
		one->width = ceil(box.width());
		one->height = ceil(box.height());
		one->outer = NEMOSHOW_ANTIALIAS_EPSILON;

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

int nemoshow_svg_update(struct nemoshow *show, struct showone *one)
{
	struct showsvg *svg = NEMOSHOW_SVG(one);

	if (svg->canvas == NULL)
		svg->canvas = nemoshow_one_get_parent(one, NEMOSHOW_CANVAS_TYPE, 0);

	if ((one->dirty & NEMOSHOW_URI_DIRTY) != 0)
		nemoshow_svg_update_uri(show, one);
	if ((one->dirty & NEMOSHOW_CHILD_DIRTY) != 0)
		nemoshow_svg_update_child(show, one);
	if ((one->dirty & NEMOSHOW_MATRIX_DIRTY) != 0)
		nemoshow_svg_update_matrix(show, one);

	if ((one->dirty & NEMOSHOW_SHAPE_DIRTY) != 0) {
		nemoshow_canvas_damage_one(svg->canvas, one);

		nemoshow_svg_update_boundingbox(show, one);
	}

	nemoshow_canvas_damage_one(svg->canvas, one);

	return 0;
}

void nemoshow_svg_set_clip(struct showone *one, struct showone *clip)
{
	struct showsvg *svg = NEMOSHOW_SVG(one);

	nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_CLIP_REF));
	nemoshow_one_reference_one(one, clip, NEMOSHOW_CLIP_REF);
}

void nemoshow_svg_set_tsr(struct showone *one)
{
	struct showsvg *svg = NEMOSHOW_SVG(one);

	svg->transform = NEMOSHOW_TSR_TRANSFORM;
}

void nemoshow_svg_set_uri(struct showone *one, const char *uri)
{
	struct showsvg *svg = NEMOSHOW_SVG(one);

	if (svg->uri != NULL)
		free(svg->uri);

	svg->uri = strdup(uri);

	nemoshow_one_dirty(one, NEMOSHOW_URI_DIRTY);
}
