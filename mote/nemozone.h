#ifndef	__NEMOMOTE_ZONE_H__
#define	__NEMOMOTE_ZONE_H__

#define	NEMOMOTE_POINT_ZONE					(0)
#define	NEMOMOTE_CUBE_ZONE					(1)
#define	NEMOMOTE_DISC_ZONE					(2)

struct nemozone;

typedef void (*nemomote_zone_locates_t)(struct nemozone *zone, double *x, double *y);
typedef int (*nemomote_zone_contains_t)(struct nemozone *zone, double x, double y);

struct nemozone {
	int type;

	double left, right;
	double top, bottom;

	double x, y;

	double radius;

	nemomote_zone_locates_t locates;
	nemomote_zone_contains_t contains;
};

extern void nemozone_set_cube(struct nemozone *zone, double left, double right, double top, double bottom);
extern void nemozone_set_disc(struct nemozone *zone, double x, double y, double radius);
extern void nemozone_set_point(struct nemozone *zone, double x, double y);

static inline int nemozone_get_type(struct nemozone *zone)
{
	return zone->type;
}

static inline void nemozone_locates(struct nemozone *zone, double *x, double *y)
{
	zone->locates(zone, x, y);
}

static inline int nemozone_contains(struct nemozone *zone, double x, double y)
{
	return zone->contains(zone, x, y);
}

#endif
