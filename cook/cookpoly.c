#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <cookpoly.h>
#include <nemomisc.h>

struct cookpoly *nemocook_polygon_create(void)
{
	struct cookpoly *poly;

	poly = (struct cookpoly *)malloc(sizeof(struct cookpoly));
	if (poly == NULL)
		return NULL;
	memset(poly, 0, sizeof(struct cookpoly));

	return poly;
}

void nemocook_polygon_destroy(struct cookpoly *poly)
{
	int i;

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
}

struct cooktex *nemocook_polygon_get_texture(struct cookpoly *poly)
{
	return poly->texture;
}
