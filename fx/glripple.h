#ifndef __NEMOFX_GL_RIPPLE_H__
#define __NEMOFX_GL_RIPPLE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

extern struct glripple *nemofx_glripple_create(int32_t width, int32_t height);
extern void nemofx_glripple_destroy(struct glripple *ripple);

extern void nemofx_glripple_use_vectors(struct glripple *ripple, float *vectors, int rows, int columns, int width, int height);
extern void nemofx_glripple_use_amplitudes(struct glripple *ripple, float *amplitudes, int length, int cycles, float amplitude);

extern void nemofx_glripple_layout(struct glripple *ripple, int32_t rows, int32_t columns, int32_t length);
extern void nemofx_glripple_resize(struct glripple *ripple, int32_t width, int32_t height);
extern void nemofx_glripple_update(struct glripple *ripple);
extern uint32_t nemofx_glripple_dispatch(struct glripple *ripple, uint32_t texture);

extern void nemofx_glripple_shoot(struct glripple *ripple, float x, float y, int step);

extern void nemofx_glripple_build_vectors(float *vectors, int rows, int columns, int width, int height);
extern void nemofx_glripple_build_amplitudes(float *amplitudes, int length, int cycles, float amplitude);

extern int32_t nemofx_glripple_get_width(struct glripple *ripple);
extern int32_t nemofx_glripple_get_height(struct glripple *ripple);
extern int32_t nemofx_glripple_get_rows(struct glripple *ripple);
extern int32_t nemofx_glripple_get_columns(struct glripple *ripple);
extern uint32_t nemofx_glripple_get_texture(struct glripple *ripple);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
