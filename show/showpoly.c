#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showpoly.h>
#include <nemomisc.h>

struct showone *nemoshow_poly_create(int type)
{
	struct showpoly *poly;
	struct showone *one;

	poly = (struct showpoly *)malloc(sizeof(struct showpoly));
	if (poly == NULL)
		return NULL;
	memset(poly, 0, sizeof(struct showpoly));

	one = &poly->base;
	one->type = NEMOSHOW_POLY_TYPE;
	one->sub = type;
	one->update = nemoshow_poly_update;
	one->destroy = nemoshow_poly_destroy;
	one->attach = nemoshow_poly_attach_one;
	one->detach = nemoshow_poly_detach_one;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "color", poly->colors, sizeof(float[4]));

	if (one->sub == NEMOSHOW_QUAD_POLY) {
		poly->vertices = (float *)malloc(sizeof(float) * 12);
		poly->nvertices = 4;
		poly->stride = 3;

		poly->mode = GL_TRIANGLE_FAN;
		poly->elements = 4;

		nemoobject_set_reserved(&one->object, "vertex", poly->vertices, sizeof(float[12]));
	} else if (one->sub == NEMOSHOW_QUAD_TEX_POLY) {
		poly->vertices = (float *)malloc(sizeof(float) * 20);
		poly->nvertices = 4;
		poly->stride = 5;

		poly->mode = GL_TRIANGLE_FAN;
		poly->elements = 4;

		nemoobject_set_reserved(&one->object, "vertex", poly->vertices, sizeof(float[20]));
	}

	return one;
}

void nemoshow_poly_destroy(struct showone *one)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);

	nemoshow_one_finish(one);

	free(poly);
}

void nemoshow_poly_attach_one(struct showone *parent, struct showone *one)
{
	nemoshow_one_attach_one(parent, one);

	nemoshow_one_reference_one(parent, one, NEMOSHOW_REDRAW_DIRTY, -1);
}

void nemoshow_poly_detach_one(struct showone *parent, struct showone *one)
{
	nemoshow_one_detach_one(parent, one);
}

int nemoshow_poly_arrange(struct showone *one)
{
	return 0;
}

int nemoshow_poly_update(struct showone *one)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);

	if ((one->dirty & NEMOSHOW_CANVAS_DIRTY) != 0) {
		struct showone *canvas = NEMOSHOW_REF(one, NEMOSHOW_CANVAS_REF);

		if (canvas != NULL) {
			nemoshow_canvas_render_vector(canvas->show, canvas);
			nemoshow_canvas_flush_vector(canvas->show, canvas);
		}
	}

	return 0;
}

void nemoshow_poly_set_canvas(struct showone *one, struct showone *canvas)
{
	nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_CANVAS_REF));
	nemoshow_one_reference_one(one, canvas, NEMOSHOW_CANVAS_DIRTY, NEMOSHOW_CANVAS_REF);
}
