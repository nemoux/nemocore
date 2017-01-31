#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemotoyz.h>
#include <nemomisc.h>

struct nemotoyz *nemotoyz_create(void)
{
	struct nemotoyz *toyz;

	toyz = (struct nemotoyz *)malloc(sizeof(struct nemotoyz));
	if (toyz == NULL)
		return NULL;
	memset(toyz, 0, sizeof(struct nemotoyz));

	return toyz;
}

void nemotoyz_destroy(struct nemotoyz *toyz)
{
	free(toyz);
}
