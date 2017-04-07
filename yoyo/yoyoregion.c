#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <yoyoregion.h>
#include <nemomisc.h>

struct yoyoregion *nemoyoyo_region_create(struct nemoyoyo *yoyo)
{
	struct yoyoregion *region;

	region = (struct yoyoregion *)malloc(sizeof(struct yoyoregion));
	if (region == NULL)
		return NULL;
	memset(region, 0, sizeof(struct yoyoregion));

	nemolist_init(&region->link);

	return region;
}

void nemoyoyo_region_destroy(struct yoyoregion *region)
{
	nemolist_remove(&region->link);

	free(region);
}
