#ifndef	__NEMOFX_GL_MASK_H__
#define	__NEMOFX_GL_MASK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

extern struct glmask *nemofx_glmask_create(int32_t width, int32_t height);
extern void nemofx_glmask_destroy(struct glmask *mask);

extern void nemofx_glmask_resize(struct glmask *mask, int32_t width, int32_t height);
extern void nemofx_glmask_dispatch(struct glmask *mask, uint32_t texture, uint32_t overlay);

extern int32_t nemofx_glmask_get_width(struct glmask *mask);
extern int32_t nemofx_glmask_get_height(struct glmask *mask);
extern uint32_t nemofx_glmask_get_texture(struct glmask *mask);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
