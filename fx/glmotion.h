#ifndef	__NEMOFX_GL_MOTION_H__
#define	__NEMOFX_GL_MOTION_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

extern struct glmotion *nemofx_glmotion_create(int32_t width, int32_t height);
extern void nemofx_glmotion_destroy(struct glmotion *motion);

extern void nemofx_glmotion_set_step(struct glmotion *motion, float step);

extern void nemofx_glmotion_resize(struct glmotion *motion, int32_t width, int32_t height);
extern uint32_t nemofx_glmotion_dispatch(struct glmotion *motion, uint32_t texture);
extern void nemofx_glmotion_clear(struct glmotion *motion);

extern float nemofx_glmotion_get_step(struct glmotion *motion);
extern uint32_t nemofx_glmotion_get_texture(struct glmotion *motion);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
