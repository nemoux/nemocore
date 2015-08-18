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

	one = &svg->base;
	one->type = NEMOSHOW_SVG_TYPE;
	one->update = nemoshow_svg_update;
	one->destroy = nemoshow_svg_destroy;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "width", &svg->width, sizeof(double));
	nemoobject_set_reserved(&one->object, "height", &svg->height, sizeof(double));

	return one;
}

void nemoshow_svg_destroy(struct showone *one)
{
	struct showsvg *svg = NEMOSHOW_SVG(one);

	nemoshow_one_finish(one);

	free(svg);
}

int nemoshow_svg_arrange(struct nemoshow *show, struct showone *one)
{
	struct showsvg *svg = NEMOSHOW_SVG(one);
	struct showone *matrix;
	int i;

	matrix = nemoshow_search_one(show, nemoobject_gets(&one->object, "matrix"));
	if (matrix != NULL) {
		svg->transform = NEMOSHOW_EXTERN_TRANSFORM;

		svg->matrix = matrix;

		NEMOBOX_APPEND(matrix->refs, matrix->srefs, matrix->nrefs, one);
	} else {
		for (i = 0; i < one->nchildren; i++) {
			if (one->children[i]->type == NEMOSHOW_MATRIX_TYPE) {
				svg->transform = NEMOSHOW_INTERN_TRANSFORM;

				break;
			}
		}
	}

	nemoshow_svg_load_uri(show, one, nemoobject_gets(&one->object, "uri"));

	svg->canvas = nemoshow_one_get_canvas(one);
	svg->group = nemoshow_one_get_parent(one, NEMOSHOW_GROUP_ITEM);

	return 0;
}

static inline void nemoshow_svg_update_child(struct nemoshow *show, struct showone *one)
{
	struct showsvg *svg = NEMOSHOW_SVG(one);
	struct showone *child;
	int i;

	if (NEMOSHOW_SVG_CC(svg, matrix) != NULL) {
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

static inline void nemoshow_svg_update_boundingbox(struct nemoshow *show, struct showone *one)
{
	struct showsvg *svg = NEMOSHOW_SVG(one);
	struct showone *group;
	SkRect box;
	char attr[NEMOSHOW_SYMBOL_MAX];

	box = SkRect::MakeXYWH(0, 0, svg->width, svg->height);

	if (svg->matrix != NULL) {
		NEMOSHOW_MATRIX_CC(NEMOSHOW_MATRIX(svg->matrix), matrix)->mapRect(&box);
	} else if (NEMOSHOW_SVG_CC(svg, matrix) != NULL) {
		NEMOSHOW_SVG_CC(svg, matrix)->mapRect(&box);
	}

	for (group = svg->group; group != NULL; group = NEMOSHOW_ITEM_AT(group, group)) {
		struct showitem *pitem = NEMOSHOW_ITEM(group);

		nemoshow_one_update_alone(show, group);

		if (pitem->matrix != NULL) {
			NEMOSHOW_MATRIX_CC(NEMOSHOW_MATRIX(pitem->matrix), matrix)->mapRect(&box);
		} else if (NEMOSHOW_ITEM_CC(pitem, matrix) != NULL) {
			NEMOSHOW_ITEM_CC(pitem, matrix)->mapRect(&box);
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

	if ((one->dirty & NEMOSHOW_CHILD_DIRTY) != 0)
		nemoshow_svg_update_child(show, one);

	if ((one->dirty & NEMOSHOW_SHAPE_DIRTY) != 0) {
		nemoshow_canvas_damage_one(svg->canvas, one);

		nemoshow_svg_update_boundingbox(show, one);
	}

	nemoshow_canvas_damage_one(svg->canvas, one);

	return 0;
}
