#ifndef	__NEMOFX_SPH_H__
#define	__NEMOFX_SPH_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemolist.h>

struct sphone {
	float x, y;
	float vx, vy;
	float ax, ay;
	float ex, ey;

	float density;
	float pressure;

	struct nemolist link;
};

struct sphcell {
	struct nemolist list;
};

struct fxsph {
	struct sphone *ones;
	int nones;

	float worldwidth, worldheight;
	float cellsize;
	uint32_t cellwidth, cellheight;

	struct sphcell *cells;
	int ncells;

	float kernel;
	float mass;

	float stiffness;
	float density;
	float walldamping;
	float viscosity;

	float gx, gy;
};

extern struct fxsph *nemofx_sph_create(int particles, float width, float height, float kernel, float mass);
extern void nemofx_sph_destroy(struct fxsph *sph);

extern void nemofx_sph_build_cell(struct fxsph *sph);
extern void nemofx_sph_compute_timestep(struct fxsph *sph);
extern void nemofx_sph_compute_density_pressure(struct fxsph *sph);
extern void nemofx_sph_compute_force(struct fxsph *sph);
extern void nemofx_sph_advect(struct fxsph *sph, float t);

static inline void nemofx_sph_set_particle(struct fxsph *sph, int index, float x, float y, float vx, float vy)
{
	struct sphone *one = &sph->ones[index];

	one->x = x;
	one->y = y;
	one->vx = vx;
	one->vy = vy;
	one->ax = 0.0f;
	one->ay = 0.0f;
	one->ex = vx;
	one->ey = vy;
	one->density = sph->density;

	nemolist_init(&one->link);
}

static inline int nemofx_sph_get_particles(struct fxsph *sph)
{
	return sph->nones;
}

static inline float nemofx_sph_get_particle_x(struct fxsph *sph, int index)
{
	return sph->ones[index].x;
}

static inline float nemofx_sph_get_particle_y(struct fxsph *sph, int index)
{
	return sph->ones[index].y;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
