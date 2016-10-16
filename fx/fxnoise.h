#ifndef	__NEMOFX_NOISE_H__
#define	__NEMOFX_NOISE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

struct fxnoise;

extern struct fxnoise *nemofx_noise_create(void);
extern void nemofx_noise_destroy(struct fxnoise *noise);

extern void nemofx_noise_set_seed(struct fxnoise *noise, int seed);
extern int nemofx_noise_get_seed(struct fxnoise *noise);
extern void nemofx_noise_set_frequency(struct fxnoise *noise, float frequency);
extern void nemofx_noise_set_interpolation(struct fxnoise *noise, const char *interp);
extern void nemofx_noise_set_type(struct fxnoise *noise, const char *type);
extern void nemofx_noise_set_fractal_octaves(struct fxnoise *noise, unsigned int octaves);
extern void nemofx_noise_set_fractal_lacunarity(struct fxnoise *noise, float lacunarity);
extern void nemofx_noise_set_fractal_gain(struct fxnoise *noise, float gain);
extern void nemofx_noise_set_fractal_type(struct fxnoise *noise, const char *type);
extern void nemofx_noise_set_cellular_distance_function(struct fxnoise *noise, const char *func);
extern void nemofx_noise_set_cellular_return_type(struct fxnoise *noise, const char *type);
extern void nemofx_noise_set_cellular_noise_lookup(struct fxnoise *noise, struct fxnoise *lookup);
extern void nemofx_noise_set_position_warp(struct fxnoise *noise, float amp);

extern void nemofx_noise_dispatch(struct fxnoise *noise, void *buffer, int32_t width, int32_t height);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
