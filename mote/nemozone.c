#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <math.h>

#include <nemozone.h>

static inline void nemomote_cubezone_locates(struct nemozone *zone, double *x, double *y)
{
	*x = zone->left + ((double)rand() / RAND_MAX) * (zone->right - zone->left);
	*y = zone->bottom + ((double)rand() / RAND_MAX) * (zone->top - zone->bottom);
}

static inline int nemomote_cubezone_contains(struct nemozone *zone, double x, double y)
{
	return x >= zone->left &&
		x <= zone->right &&
		y >= zone->top &&
		y <= zone->bottom;
}

static inline void nemomote_disczone_locates(struct nemozone *zone, double *x, double *y)
{
	double radius = ((double)rand() / RAND_MAX) * zone->radius;
	double degree = ((double)rand() / RAND_MAX) * M_PI * 2.0f;

	*x = zone->x + cos(degree) * radius;
	*y = zone->y + sin(degree) * radius;
}

static inline int nemomote_disczone_contains(struct nemozone *zone, double x, double y)
{
	double dx = zone->x - x;
	double dy = zone->y - y;

	return sqrtf(dx * dx + dy * dy) <= zone->radius;
}

static inline void nemomote_pointzone_locates(struct nemozone *zone, double *x, double *y)
{
	*x = zone->x;
	*y = zone->y;
}

static inline int nemomote_pointzone_contains(struct nemozone *zone, double x, double y)
{
	return zone->x == x && zone->y == y;
}

void nemozone_set_cube(struct nemozone *zone, double left, double right, double top, double bottom)
{
	zone->type = NEMOMOTE_CUBE_ZONE;

	zone->left = left;
	zone->right = right;
	zone->top = top;
	zone->bottom = bottom;

	zone->locates = nemomote_cubezone_locates;
	zone->contains = nemomote_cubezone_contains;
}

void nemozone_set_disc(struct nemozone *zone, double x, double y, double radius)
{
	zone->type = NEMOMOTE_DISC_ZONE;

	zone->x = x;
	zone->y = y;
	zone->radius = radius;

	zone->locates = nemomote_disczone_locates;
	zone->contains = nemomote_disczone_contains;
}

void nemozone_set_point(struct nemozone *zone, double x, double y)
{
	zone->type = NEMOMOTE_POINT_ZONE;

	zone->x = x;
	zone->y = y;

	zone->locates = nemomote_pointzone_locates;
	zone->contains = nemomote_pointzone_contains;
}
