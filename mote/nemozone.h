#ifndef	__NEMOMOTE_ZONE_H__
#define	__NEMOMOTE_ZONE_H__

#define	NEMOMOTE_POINT_ZONE					(0)
#define	NEMOMOTE_CUBE_ZONE					(1)
#define	NEMOMOTE_DISC_ZONE					(2)

struct nemozone;

typedef void (*nemomote_zone_locates_t)(struct nemozone *zone, double *x, double *y, double *z);
typedef int (*nemomote_zone_contains_t)(struct nemozone *zone, double x, double y, double z);

struct nemozone {
	int type;

	double left, right;
	double top, bottom;
	double back, front;

	double x, y, z;

	double radius;

	nemomote_zone_locates_t locates;
	nemomote_zone_contains_t contains;
};

extern void nemozone_set_cube(struct nemozone *zone, double left, double right, double top, double bottom, double back, double front);
extern void nemozone_set_disc(struct nemozone *zone, double x, double y, double z, double radius);
extern void nemozone_set_point(struct nemozone *zone, double x, double y, double z);

static inline int nemozone_get_type(struct nemozone *zone)
{
	return zone->type;
}

static inline void nemozone_locates(struct nemozone *zone, double *x, double *y, double *z)
{
	zone->locates(zone, x, y, z);
}

static inline int nemozone_contains(struct nemozone *zone, double x, double y, double z)
{
	return zone->contains(zone, x, y, z);
}

#endif
