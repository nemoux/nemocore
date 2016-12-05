#ifndef	__NEMOFX_SPH_H__
#define	__NEMOFX_SPH_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

extern struct fxsph *nemofx_sph_create(int particles, float width, float height, float kernel, float mass);
extern void nemofx_sph_destroy(struct fxsph *sph);

extern void nemofx_sph_build_cell(struct fxsph *sph);
extern void nemofx_sph_compute_timestep(struct fxsph *sph);
extern void nemofx_sph_compute_density_pressure(struct fxsph *sph);
extern void nemofx_sph_compute_force(struct fxsph *sph);
extern void nemofx_sph_advect(struct fxsph *sph, float t);

extern void nemofx_sph_set_particle(struct fxsph *sph, int index, float x, float y, float vx, float vy);
extern int nemofx_sph_get_particles(struct fxsph *sph);
extern float nemofx_sph_get_particle_x(struct fxsph *sph, int index);
extern float nemofx_sph_get_particle_y(struct fxsph *sph, int index);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
