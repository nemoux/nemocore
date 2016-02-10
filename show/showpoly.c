#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showpoly.h>
#include <nemomisc.h>

static const float quad_vertices[] = {
	-1.0f, -1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	-1.0f, 1.0f, 0.0f
};

static const float quad_texcoords[] = {
	0.0f, 1.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 0.0f
};

static const float quad_normals[] = {
	0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f
};

static const float cube_vertices[] = {
	-1.0f, -1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,
	1.0f, -1.0f, 1.0f,
	-1.0f, -1.0f, -1.0f,
	1.0f, -1.0f, 1.0f,
	-1.0f, -1.0f, 1.0f,

	-1.0f, 1.0f, -1.0f,
	-1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	-1.0f, 1.0f, -1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, -1.0f,

	-1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f, 1.0f,
	-1.0f, 1.0f, 1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f, 1.0f, 1.0f,
	-1.0f, 1.0f, -1.0f,

	1.0f, -1.0f, -1.0f,
	1.0f, 1.0f, -1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, -1.0f, -1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, -1.0f, 1.0f,

	-1.0f, -1.0f, -1.0f,
	-1.0f, 1.0f, -1.0f,
	1.0f, 1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	1.0f, 1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,

	-1.0f, 1.0f, 1.0f,
	-1.0f, -1.0f, 1.0f,
	1.0f, -1.0f, 1.0f,
	-1.0f, 1.0f, 1.0f,
	1.0f, -1.0f, 1.0f,
	1.0f, 1.0f, 1.0f
};

static const float cube_texcoords[] = {
	0.0f, 1.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 0.0f,

	0.0f, 1.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 0.0f,

	0.0f, 1.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 0.0f,

	0.0f, 1.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 0.0f,

	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	0.0f, 0.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,

	0.0f, 1.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 0.0f
};

static const float cube_normals[] = {
	0.0f, -1.0f, 0.0f,
	0.0f, -1.0f, 0.0f,
	0.0f, -1.0f, 0.0f,
	0.0f, -1.0f, 0.0f,
	0.0f, -1.0f, 0.0f,
	0.0f, -1.0f, 0.0f,

	0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f,

	-1.0f, 0.0f, 0.0f,
	-1.0f, 0.0f, 0.0f,
	-1.0f, 0.0f, 0.0f,
	-1.0f, 0.0f, 0.0f,
	-1.0f, 0.0f, 0.0f,
	-1.0f, 0.0f, 0.0f,

	1.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f,

	0.0f, 0.0f, -1.0f,
	0.0f, 0.0f, -1.0f,
	0.0f, 0.0f, -1.0f,
	0.0f, 0.0f, -1.0f,
	0.0f, 0.0f, -1.0f,
	0.0f, 0.0f, -1.0f,

	0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f
};

struct showone *nemoshow_poly_create(int type)
{
	struct showpoly *poly;
	struct showone *one;

	poly = (struct showpoly *)malloc(sizeof(struct showpoly));
	if (poly == NULL)
		return NULL;
	memset(poly, 0, sizeof(struct showpoly));

	poly->sx = 1.0f;
	poly->sy = 1.0f;
	poly->sz = 1.0f;

	one = &poly->base;
	one->type = NEMOSHOW_POLY_TYPE;
	one->sub = type;
	one->update = nemoshow_poly_update;
	one->destroy = nemoshow_poly_destroy;
	one->attach = nemoshow_poly_attach_one;
	one->detach = nemoshow_poly_detach_one;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "color", poly->colors, sizeof(float[4]));
	nemoobject_set_reserved(&one->object, "matrix", poly->modelview.d, sizeof(float[16]));

	nemoobject_set_reserved(&one->object, "tx", &poly->tx, sizeof(double));
	nemoobject_set_reserved(&one->object, "ty", &poly->ty, sizeof(double));
	nemoobject_set_reserved(&one->object, "tz", &poly->tz, sizeof(double));
	nemoobject_set_reserved(&one->object, "sx", &poly->sx, sizeof(double));
	nemoobject_set_reserved(&one->object, "sy", &poly->sy, sizeof(double));
	nemoobject_set_reserved(&one->object, "sz", &poly->sz, sizeof(double));
	nemoobject_set_reserved(&one->object, "rx", &poly->rx, sizeof(double));
	nemoobject_set_reserved(&one->object, "ry", &poly->ry, sizeof(double));
	nemoobject_set_reserved(&one->object, "rz", &poly->rz, sizeof(double));

	if (one->sub == NEMOSHOW_QUAD_POLY) {
		poly->vertices = (float *)malloc(sizeof(float) * 12);
		poly->elements = 12 / 3;

		poly->mode = GL_TRIANGLE_FAN;

		nemoobject_set_reserved(&one->object, "vertex", poly->vertices, sizeof(float[12]));

		memcpy(poly->vertices, quad_vertices, sizeof(float[12]));
	} else if (one->sub == NEMOSHOW_CUBE_POLY) {
		poly->vertices = (float *)malloc(sizeof(float) * 18 * 6);
		poly->elements = 18 * 6 / 3;

		poly->mode = GL_TRIANGLES;

		nemoobject_set_reserved(&one->object, "vertex", poly->vertices, sizeof(float[18 * 6]));

		memcpy(poly->vertices, cube_vertices, sizeof(float[18 * 6]));
	}

	return one;
}

void nemoshow_poly_destroy(struct showone *one)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);

	nemoshow_one_finish(one);

	if (poly->vertices != NULL)
		free(poly->vertices);

	if (poly->on_texcoords != 0) {
		free(poly->texcoords);
	}
	if (poly->on_normals != 0) {
		free(poly->normals);
	}
	if (poly->on_vbo != 0) {
		glDeleteBuffers(1, &poly->vvertex);
		glDeleteBuffers(1, &poly->vtexcoord);
		glDeleteBuffers(1, &poly->vnormal);
		glDeleteBuffers(1, &poly->vindex);
		glDeleteVertexArrays(1, &poly->varray);
	}

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
		if (poly->on_vbo != 0) {
			glBindVertexArray(poly->varray);

			glBindBuffer(GL_ARRAY_BUFFER, poly->vvertex);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void *)0);
			glEnableVertexAttribArray(0);
			glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat[3]) * poly->elements, poly->vertices, GL_STATIC_DRAW);

			if (poly->on_texcoords != 0) {
				glBindBuffer(GL_ARRAY_BUFFER, poly->vtexcoord);
				glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void *)0);
				glEnableVertexAttribArray(1);
				glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat[2]) * poly->elements, poly->texcoords, GL_STATIC_DRAW);
			}

			if (poly->on_normals != 0) {
				glBindBuffer(GL_ARRAY_BUFFER, poly->vnormal);
				glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void *)0);
				glEnableVertexAttribArray(2);
				glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat[3]) * poly->elements, poly->normals, GL_STATIC_DRAW);
			}

			glBindBuffer(GL_ARRAY_BUFFER, 0);

			glBindVertexArray(0);
		}
	}

	if ((one->dirty & NEMOSHOW_MATRIX_DIRTY) != 0) {
		nemomatrix_init_identity(&poly->modelview);
		nemomatrix_rotate_x(&poly->modelview, cos(poly->rx), sin(poly->rx));
		nemomatrix_rotate_y(&poly->modelview, cos(poly->ry), sin(poly->ry));
		nemomatrix_rotate_z(&poly->modelview, cos(poly->rz), sin(poly->rz));
		nemomatrix_scale_xyz(&poly->modelview, poly->sx, poly->sy, poly->sz);
		nemomatrix_translate_xyz(&poly->modelview, poly->tx, poly->ty, poly->tz);
	}

	return 0;
}

void nemoshow_poly_set_canvas(struct showone *one, struct showone *canvas)
{
	nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_CANVAS_REF));
	nemoshow_one_reference_one(one, canvas, NEMOSHOW_CANVAS_DIRTY, NEMOSHOW_CANVAS_REF);
}

void nemoshow_poly_use_texcoords(struct showone *one, int on_texcoords)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);

	if (poly->on_texcoords == 0 && on_texcoords != 0) {
		poly->texcoords = (float *)malloc(sizeof(float[2]) * poly->elements);

		nemoobject_set_reserved(&one->object, "texcoord", poly->texcoords, sizeof(float[2]) * poly->elements);

		if (one->sub == NEMOSHOW_QUAD_POLY)
			memcpy(poly->texcoords, quad_texcoords, sizeof(float[2]) * poly->elements);
		else if (one->sub == NEMOSHOW_CUBE_POLY)
			memcpy(poly->texcoords, cube_texcoords, sizeof(float[2]) * poly->elements);
	} else if (poly->on_texcoords != 0 && on_texcoords == 0) {
		free(poly->texcoords);
	}

	poly->on_texcoords = on_texcoords;
}

void nemoshow_poly_use_normals(struct showone *one, int on_normals)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);

	if (poly->on_normals == 0 && on_normals != 0) {
		poly->normals = (float *)malloc(sizeof(float[3]) * poly->elements);

		nemoobject_set_reserved(&one->object, "normal", poly->normals, sizeof(float[3]) * poly->elements);

		if (one->sub == NEMOSHOW_QUAD_POLY)
			memcpy(poly->normals, quad_normals, sizeof(float[3]) * poly->elements);
		else if (one->sub == NEMOSHOW_CUBE_POLY)
			memcpy(poly->normals, cube_normals, sizeof(float[3]) * poly->elements);
	} else if (poly->on_normals != 0 && on_normals == 0) {
		free(poly->normals);
	}

	poly->on_normals = on_normals;
}

void nemoshow_poly_use_vbo(struct showone *one, int on_vbo)
{
	struct showpoly *poly = NEMOSHOW_POLY(one);

	if (poly->on_vbo == 0 && on_vbo != 0) {
		glGenVertexArrays(1, &poly->varray);
		glGenBuffers(1, &poly->vvertex);
		glGenBuffers(1, &poly->vtexcoord);
		glGenBuffers(1, &poly->vnormal);
		glGenBuffers(1, &poly->vindex);
	} else if (poly->on_vbo != 0 && on_vbo == 0) {
		glDeleteBuffers(1, &poly->vvertex);
		glDeleteBuffers(1, &poly->vtexcoord);
		glDeleteBuffers(1, &poly->vnormal);
		glDeleteBuffers(1, &poly->vindex);
		glDeleteVertexArrays(1, &poly->varray);
	}

	poly->on_vbo = on_vbo;
}
