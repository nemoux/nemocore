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

	if ((one->dirty & NEMOSHOW_SHAPE_DIRTY) != 0) {
		if (poly->has_vbo != 0) {
			glBindVertexArray(poly->varray);

			if (one->sub == NEMOSHOW_QUAD_POLY) {
				glBindBuffer(GL_ARRAY_BUFFER, poly->vbuffer);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void *)0);
				glEnableVertexAttribArray(0);
				glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat[3]) * poly->nvertices, poly->vertices, GL_STATIC_DRAW);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
			} else if (one->sub == NEMOSHOW_QUAD_TEX_POLY) {
				glBindBuffer(GL_ARRAY_BUFFER, poly->vbuffer);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void *)0);
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void *)sizeof(GLfloat[3]));
				glEnableVertexAttribArray(1);
				glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat[5]) * poly->nvertices, poly->vertices, GL_STATIC_DRAW);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}

			glBindVertexArray(0);
		}
	}

	return 0;
}

void nemoshow_poly_set_canvas(struct showone *one, struct showone *canvas)
{
	nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_CANVAS_REF));
	nemoshow_one_reference_one(one, canvas, NEMOSHOW_CANVAS_DIRTY, NEMOSHOW_CANVAS_REF);
}

void nemoshow_poly_set_vbo(struct showone *one, int has_vbo)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);

	if (poly->has_vbo == 0 && has_vbo != 0) {
		glGenVertexArrays(1, &poly->varray);
		glGenBuffers(1, &poly->vbuffer);
		glGenBuffers(1, &poly->vindex);
	} else if (poly->has_vbo != 0 && has_vbo == 0) {
		glDeleteBuffers(1, &poly->vbuffer);
		glDeleteBuffers(1, &poly->vindex);
		glDeleteVertexArrays(1, &poly->varray);
	}

	poly->has_vbo = has_vbo;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}
