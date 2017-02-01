#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <toyzregion.hpp>
#include <nemomisc.h>

struct toyzregion *nemotoyz_region_create(void)
{
	struct toyzregion *region;

	region = new toyzregion;
	region->region = new SkRegion;

	return region;
}

void nemotoyz_region_destroy(struct toyzregion *region)
{
	delete region->region;
	delete region;
}

void nemotoyz_region_clear(struct toyzregion *region)
{
	region->region->setEmpty();
}

void nemotoyz_region_set_rectangle(struct toyzregion *region, float x, float y, float w, float h)
{
	region->region->setRect(SkIRect::MakeXYWH(x, y, w, h));
}

void nemotoyz_region_intersect(struct toyzregion *region, float x, float y, float w, float h)
{
	region->region->op(*region->region, SkIRect::MakeXYWH(x, y, w, h), SkRegion::kIntersect_Op);
}

void nemotoyz_region_union(struct toyzregion *region, float x, float y, float w, float h)
{
	region->region->op(*region->region, SkIRect::MakeXYWH(x, y, w, h), SkRegion::kUnion_Op);
}
