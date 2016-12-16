#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemocook.h>
#include <nemomisc.h>

struct nemocook *nemocook_create(void)
{
	struct nemocook *cook;

	cook = (struct nemocook *)malloc(sizeof(struct nemocook));
	if (cook == NULL)
		return NULL;
	memset(cook, 0, sizeof(struct nemocook));

	nemolist_init(&cook->poly_list);

	return cook;
}

void nemocook_destroy(struct nemocook *cook)
{
	nemolist_remove(&cook->poly_list);

	free(cook);
}

void nemocook_set_size(struct nemocook *cook, uint32_t width, uint32_t height)
{
	cook->width = width;
	cook->height = height;
}

void nemocook_attach_polygon(struct nemocook *cook, struct cookpoly *poly)
{
	nemolist_insert_tail(&cook->poly_list, &poly->link);
}

void nemocook_detach_polygon(struct nemocook *cook, struct cookpoly *poly)
{
	nemolist_remove(&poly->link);
	nemolist_init(&poly->link);
}
