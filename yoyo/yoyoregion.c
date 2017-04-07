#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <yoyoregion.h>
#include <nemomisc.h>

static int nemoyoyo_region_contain_rectangle(struct yoyoregion *region, float x, float y)
{
	if (region->x0 <= x && x < region->x1 && region->y0 <= y && y < region->y1)
		return 1;

	return 0;
}

static int nemoyoyo_region_contain_triangle(struct yoyoregion *region, float x, float y)
{
	float x0 = x - region->x0;
	float y0 = y - region->y0;
	int xy = ((region->x1 - region->x0) * y0 - (region->y1 - region->y0) * x0 > 0);

	if ((((region->x2 - region->x0) * y0 - (region->y2 - region->y0) * x0) > 0) == xy)
		return 0;

	if ((((region->x2 - region->x1) * (y - region->y1) - (region->y2 - region->y1) * (x - region->x1)) > 0) != xy)
		return 0;

	return 1;
}

void nemoyoyo_region_set_type(struct yoyoregion *region, int type)
{
	if (type == NEMOYOYO_REGION_TRIANGLE_TYPE)
		region->contain = nemoyoyo_region_contain_triangle;
	else
		region->contain = nemoyoyo_region_contain_rectangle;
}

struct yoyoregion *nemoyoyo_region_create(struct nemoyoyo *yoyo)
{
	struct yoyoregion *region;

	region = (struct yoyoregion *)malloc(sizeof(struct yoyoregion));
	if (region == NULL)
		return NULL;
	memset(region, 0, sizeof(struct yoyoregion));

	nemolist_init(&region->link);

	region->contain = nemoyoyo_region_contain_rectangle;

	return region;
}

void nemoyoyo_region_destroy(struct yoyoregion *region)
{
	nemolist_remove(&region->link);

	free(region);
}
