#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <cookpoly.h>
#include <cookone.h>
#include <nemomisc.h>

static void nemocook_polygon_update_attrib_simple(struct cookpoly *poly, int nattribs)
{
	int i;

	for (i = 0; i < nattribs; i++) {
		glVertexAttribPointer(i,
				poly->elements[i],
				GL_FLOAT,
				GL_FALSE,
				poly->elements[i] * sizeof(GLfloat),
				poly->buffers[i]);
		glEnableVertexAttribArray(i);
	}
}

static void nemocook_polygon_update_attrib_vbo(struct cookpoly *poly, int nattribs)
{
}

static void nemocook_polygon_update_buffer_simple(struct cookpoly *poly)
{
}

static void nemocook_polygon_update_buffer_vbo(struct cookpoly *poly)
{
	int i;

	if (poly->varray == 0)
		glGenVertexArrays(1, &poly->varray);

	glBindVertexArray(poly->varray);

	for (i = 0; i < NEMOCOOK_POLYGON_ATTRIBS_MAX; i++) {
		if (poly->buffers[i] != NULL && poly->vbuffers[i] == 0) {
			glGenBuffers(1, &poly->vbuffers[i]);

			glBindBuffer(GL_ARRAY_BUFFER, poly->vbuffers[i]);
			glVertexAttribPointer(i,
					poly->elements[i],
					GL_FLOAT,
					GL_FALSE,
					poly->elements[i] * sizeof(GLfloat),
					NULL);
			glEnableVertexAttribArray(i);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
	}

	if (poly->indices != NULL && poly->vindices == 0)
		glGenBuffers(1, &poly->vindices);

	for (i = 0; i < NEMOCOOK_POLYGON_ATTRIBS_MAX; i++) {
		if (poly->vbuffers[i] > 0) {
			glBindBuffer(GL_ARRAY_BUFFER, poly->vbuffers[i]);
			glBufferData(GL_ARRAY_BUFFER,
					poly->count * poly->elements[i] * sizeof(GLfloat),
					poly->buffers[i],
					GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
	}

	if (poly->vindices > 0) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, poly->vindices);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				poly->count * sizeof(GLuint),
				poly->indices,
				GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	glBindVertexArray(0);
}

static void nemocook_polygon_draw_simple(struct cookpoly *poly)
{
	glDrawArrays(poly->type, 0, poly->count);
}

static void nemocook_polygon_draw_indices(struct cookpoly *poly)
{
	glDrawElements(poly->type, poly->count, GL_UNSIGNED_INT, poly->indices);
}

static void nemocook_polygon_draw_texture(struct cookpoly *poly)
{
	nemocook_texture_update_state(poly->texture);

	glBindTexture(GL_TEXTURE_2D, poly->texture->texture);
	glDrawArrays(poly->type, 0, poly->count);
	glBindTexture(GL_TEXTURE_2D, 0);
}

static void nemocook_polygon_draw_indices_texture(struct cookpoly *poly)
{
	nemocook_texture_update_state(poly->texture);

	glBindTexture(GL_TEXTURE_2D, poly->texture->texture);
	glDrawElements(poly->type, poly->count, GL_UNSIGNED_INT, poly->indices);
	glBindTexture(GL_TEXTURE_2D, 0);
}

static void nemocook_polygon_draw_vbo(struct cookpoly *poly)
{
	glBindVertexArray(poly->varray);

	glDrawArrays(poly->type, 0, poly->count);

	glBindVertexArray(0);
}

static void nemocook_polygon_draw_vbo_indices(struct cookpoly *poly)
{
	glBindVertexArray(poly->varray);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, poly->vindices);

	glDrawElements(poly->type, poly->count, GL_UNSIGNED_INT, NULL);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

static void nemocook_polygon_draw_vbo_texture(struct cookpoly *poly)
{
	nemocook_texture_update_state(poly->texture);

	glBindVertexArray(poly->varray);

	glBindTexture(GL_TEXTURE_2D, poly->texture->texture);
	glDrawArrays(poly->type, 0, poly->count);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindVertexArray(0);
}

static void nemocook_polygon_draw_vbo_indices_texture(struct cookpoly *poly)
{
	nemocook_texture_update_state(poly->texture);

	glBindVertexArray(poly->varray);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, poly->vindices);

	glBindTexture(GL_TEXTURE_2D, poly->texture->texture);
	glDrawElements(poly->type, poly->count, GL_UNSIGNED_INT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

static inline void nemocook_polygon_update_callbacks(struct cookpoly *poly)
{
	if (poly->use_vbo != 0) {
		if (poly->texture != NULL) {
			if (poly->indices != NULL) {
				poly->draw = nemocook_polygon_draw_vbo_indices_texture;
			} else {
				poly->draw = nemocook_polygon_draw_vbo_texture;
			}
		} else {
			if (poly->indices != NULL) {
				poly->draw = nemocook_polygon_draw_vbo_indices;
			} else {
				poly->draw = nemocook_polygon_draw_vbo;
			}
		}

		poly->update_attrib = nemocook_polygon_update_attrib_vbo;
		poly->update_buffer = nemocook_polygon_update_buffer_vbo;
	} else {
		if (poly->texture != NULL) {
			if (poly->indices != NULL) {
				poly->draw = nemocook_polygon_draw_indices_texture;
			} else {
				poly->draw = nemocook_polygon_draw_texture;
			}
		} else {
			if (poly->indices != NULL) {
				poly->draw = nemocook_polygon_draw_indices;
			} else {
				poly->draw = nemocook_polygon_draw_simple;
			}
		}

		poly->update_attrib = nemocook_polygon_update_attrib_simple;
		poly->update_buffer = nemocook_polygon_update_buffer_simple;
	}
}

struct cookpoly *nemocook_polygon_create(void)
{
	struct cookpoly *poly;

	poly = (struct cookpoly *)malloc(sizeof(struct cookpoly));
	if (poly == NULL)
		return NULL;
	memset(poly, 0, sizeof(struct cookpoly));

	poly->update_attrib = nemocook_polygon_update_attrib_simple;
	poly->update_buffer = nemocook_polygon_update_buffer_simple;
	poly->draw = nemocook_polygon_draw_simple;

	nemomatrix_init_identity(&poly->matrix);
	nemomatrix_init_identity(&poly->inverse);

	nemolist_init(&poly->link);

	nemocook_one_prepare(&poly->one);

	return poly;
}

void nemocook_polygon_destroy(struct cookpoly *poly)
{
	int i;

	nemolist_remove(&poly->link);

	nemocook_one_finish(&poly->one);

	for (i = 0; i < NEMOCOOK_POLYGON_ATTRIBS_MAX; i++) {
		if (poly->buffers[i] != NULL)
			free(poly->buffers[i]);

		if (poly->vbuffers[i] > 0)
			glDeleteBuffers(1, &poly->vbuffers[i]);
	}

	if (poly->indices != NULL)
		free(poly->indices);

	if (poly->vindices > 0)
		glDeleteBuffers(1, &poly->vindices);

	if (poly->varray > 0)
		glDeleteVertexArrays(1, &poly->varray);

	free(poly);
}

void nemocook_polygon_set_count(struct cookpoly *poly, int count)
{
	int i;

	poly->count = count;

	for (i = 0; i < NEMOCOOK_POLYGON_ATTRIBS_MAX; i++) {
		if (poly->buffers[i] != NULL)
			poly->buffers[i] = (float *)realloc(poly->buffers[i], sizeof(float) * poly->count * poly->elements[i]);
	}

	if (poly->indices != NULL)
		poly->indices = (uint32_t *)realloc(poly->indices, sizeof(uint32_t) * poly->count);
}

void nemocook_polygon_set_type(struct cookpoly *poly, int type)
{
	poly->type = type;
}

void nemocook_polygon_use_vbo(struct cookpoly *poly)
{
	poly->use_vbo = 1;

	nemocook_polygon_update_callbacks(poly);
}

void nemocook_polygon_set_buffer(struct cookpoly *poly, int attrib, int element)
{
	poly->elements[attrib] = element;

	poly->buffers[attrib] = (float *)realloc(poly->buffers[attrib], sizeof(float) * poly->count * poly->elements[attrib]);
}

float *nemocook_polygon_get_buffer(struct cookpoly *poly, int attrib)
{
	return poly->buffers[attrib];
}

void nemocook_polygon_copy_buffer(struct cookpoly *poly, int attrib, float *buffer, int size)
{
	memcpy(poly->buffers[attrib], buffer, size);
}

void nemocook_polygon_set_indices(struct cookpoly *poly)
{
	poly->indices = (uint32_t *)realloc(poly->indices, sizeof(uint32_t) * poly->count);

	nemocook_polygon_update_callbacks(poly);
}

uint32_t *nemocook_polygon_get_indices(struct cookpoly *poly)
{
	return poly->indices;
}

void nemocook_polygon_copy_indices(struct cookpoly *poly, uint32_t *indices, int size)
{
	memcpy(poly->indices, indices, size);
}

void nemocook_polygon_set_texture(struct cookpoly *poly, struct cooktex *tex)
{
	poly->texture = tex;

	nemocook_polygon_update_callbacks(poly);
}

struct cooktex *nemocook_polygon_get_texture(struct cookpoly *poly)
{
	return poly->texture;
}

void nemocook_polygon_set_color(struct cookpoly *poly, float r, float g, float b, float a)
{
	poly->color[0] = r;
	poly->color[1] = g;
	poly->color[2] = b;
	poly->color[3] = a;
}

float *nemocook_polygon_get_color(struct cookpoly *poly)
{
	return poly->color;
}

void nemocook_polygon_set_transform(struct cookpoly *poly, struct cooktrans *trans)
{
	poly->transform = trans;
}

int nemocook_polygon_update_transform(struct cookpoly *poly)
{
	struct cooktrans *trans;

	nemomatrix_init_identity(&poly->matrix);

	for (trans = poly->transform; trans != NULL; trans = trans->parent) {
		nemomatrix_multiply(&poly->matrix, &trans->matrix);
	}

	if (nemomatrix_invert(&poly->inverse, &poly->matrix) < 0)
		return -1;

	return 0;
}

int nemocook_polygon_transform_to_global(struct cookpoly *poly, float sx, float sy, float sz, float *x, float *y, float *z)
{
	struct nemovector v = { { sx, sy, sz, 1.0f } };

	nemomatrix_transform_vector(&poly->matrix, &v);

	if (fabsf(v.f[3]) < 1e-6)
		return -1;

	*x = v.f[0] / v.f[3];
	*y = v.f[1] / v.f[3];
	*z = v.f[2] / v.f[3];

	return 0;
}

int nemocook_polygon_transform_from_global(struct cookpoly *poly, float x, float y, float z, float *sx, float *sy, float *sz)
{
	struct nemovector v = { { x, y, z, 1.0f } };

	nemomatrix_transform_vector(&poly->inverse, &v);

	if (fabsf(v.f[3]) < 1e-6)
		return -1;

	*sx = v.f[0] / v.f[3];
	*sy = v.f[1] / v.f[3];
	*sz = v.f[2] / v.f[3];

	return 0;
}

int nemocook_polygon_2d_transform_to_global(struct cookpoly *poly, float sx, float sy, float *x, float *y)
{
	struct nemovector v = { { sx, sy, 0.0f, 1.0f } };

	nemomatrix_transform_vector(&poly->matrix, &v);

	if (fabsf(v.f[3]) < 1e-6)
		return -1;

	*x = v.f[0] / v.f[3];
	*y = v.f[1] / v.f[3];

	return 0;
}

int nemocook_polygon_2d_transform_from_global(struct cookpoly *poly, float x, float y, float *sx, float *sy)
{
	struct nemovector v = { { x, y, 0.0f, 1.0f } };

	nemomatrix_transform_vector(&poly->inverse, &v);

	if (fabsf(v.f[3]) < 1e-6)
		return -1;

	*sx = v.f[0] / v.f[3];
	*sy = v.f[1] / v.f[3];

	return 0;
}
