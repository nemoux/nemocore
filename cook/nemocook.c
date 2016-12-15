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

	return cook;
}

void nemocook_destroy(struct nemocook *cook)
{
	free(cook);
}

void nemocook_set_size(struct nemocook *cook, uint32_t width, uint32_t height)
{
	cook->width = width;
	cook->height = height;
}
