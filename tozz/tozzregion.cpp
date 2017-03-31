#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemotozz.h>
#include <tozzregion.hpp>
#include <nemomisc.h>

struct tozzregion *nemotozz_region_create(void)
{
	struct tozzregion *region;

	region = new tozzregion;
	region->region = new SkRegion;

	return region;
}

void nemotozz_region_destroy(struct tozzregion *region)
{
	delete region->region;
	delete region;
}

void nemotozz_region_clear(struct tozzregion *region)
{
	region->region->setEmpty();
}

void nemotozz_region_set_rectangle(struct tozzregion *region, float x, float y, float w, float h)
{
	region->region->setRect(SkIRect::MakeXYWH(x, y, w, h));
}

void nemotozz_region_intersect(struct tozzregion *region, float x, float y, float w, float h)
{
	region->region->op(*region->region, SkIRect::MakeXYWH(x, y, w, h), SkRegion::kIntersect_Op);
}

void nemotozz_region_union(struct tozzregion *region, float x, float y, float w, float h)
{
	region->region->op(*region->region, SkIRect::MakeXYWH(x, y, w, h), SkRegion::kUnion_Op);
}

int nemotozz_region_check_intersection(struct tozzregion *region, float x, float y, float w, float h)
{
	return region->region->intersects(SkIRect::MakeXYWH(x, y, w, h)) == true;
}
