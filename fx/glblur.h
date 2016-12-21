#ifndef	__NEMOFX_GL_BLUR_H__
#define	__NEMOFX_GL_BLUR_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

extern struct glblur *nemofx_glblur_create(int32_t width, int32_t height);
extern void nemofx_glblur_destroy(struct glblur *blur);

extern void nemofx_glblur_use_fbo(struct glblur *blur);

extern void nemofx_glblur_set_radius(struct glblur *blur, int32_t rx, int32_t ry);

extern void nemofx_glblur_resize(struct glblur *blur, int32_t width, int32_t height);
extern uint32_t nemofx_glblur_dispatch(struct glblur *blur, uint32_t texture);

extern int32_t nemofx_glblur_get_width(struct glblur *blur);
extern int32_t nemofx_glblur_get_height(struct glblur *blur);
extern uint32_t nemofx_glblur_get_texture(struct glblur *blur);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
