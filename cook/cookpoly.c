#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <cookpoly.h>
#include <cookone.h>
#include <nemomisc.h>

static void nemocook_polygon_draw_simple(struct cookpoly *poly)
{
	glDrawArrays(poly->type, 0, poly->count);
}

static void nemocook_polygon_draw_texture(struct cookpoly *poly)
{
	glBindTexture(GL_TEXTURE_2D, poly->texture->texture);
	glDrawArrays(poly->type, 0, poly->count);
	glBindTexture(GL_TEXTURE_2D, 0);
}

struct cookpoly *nemocook_polygon_create(void)
{
	struct cookpoly *poly;

	poly = (struct cookpoly *)malloc(sizeof(struct cookpoly));
	if (poly == NULL)
		return NULL;
	memset(poly, 0, sizeof(struct cookpoly));

	poly->draw = nemocook_polygon_draw_simple;

	nemomatrix_init_identity(&poly->matrix);

	nemolist_init(&poly->link);

	nemocook_one_prepare(&poly->one);

	return poly;
}

void nemocook_polygon_destroy(struct cookpoly *poly)
{
	int i;

	nemolist_remove(&poly->link);

	nemocook_one_finish(&poly->one);

	for (i = 0; i < NEMOCOOK_SHADER_ATTRIBS_MAX; i++) {
		if (poly->buffers[i] != NULL)
			free(poly->buffers[i]);
	}

	free(poly);
}

void nemocook_polygon_set_count(struct cookpoly *poly, int count)
{
	int i;

	poly->count = count;

	for (i = 0; i < NEMOCOOK_SHADER_ATTRIBS_MAX; i++) {
		if (poly->buffers[i] != NULL)
			poly->buffers[i] = (float *)realloc(poly->buffers[i], sizeof(float) * poly->count * poly->elements[i]);
	}
}

void nemocook_polygon_set_type(struct cookpoly *poly, int type)
{
	poly->type = type;
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

void nemocook_polygon_set_texture(struct cookpoly *poly, struct cooktex *tex)
{
	poly->texture = tex;

	if (tex != NULL)
		poly->draw = nemocook_polygon_draw_texture;
	else
		poly->draw = nemocook_polygon_draw_simple;
}

struct cooktex *nemocook_polygon_get_texture(struct cookpoly *poly)
{
	return poly->texture;
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

	return 0;
}

void nemocook_polygon_set_shader(struct cookpoly *poly, struct cookshader *shader)
{
	poly->shader = shader;
}
