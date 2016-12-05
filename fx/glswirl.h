#ifndef	__NEMOFX_GL_SWIRL_H__
#define	__NEMOFX_GL_SWIRL_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

extern struct glswirl *nemofx_glswirl_create(int32_t width, int32_t height);
extern void nemofx_glswirl_destroy(struct glswirl *swirl);

extern void nemofx_glswirl_set_radius(struct glswirl *swirl, float radius);
extern void nemofx_glswirl_set_angle(struct glswirl *swirl, float angle);
extern void nemofx_glswirl_set_center(struct glswirl *swirl, float cx, float cy);

extern void nemofx_glswirl_resize(struct glswirl *swirl, int32_t width, int32_t height);
extern void nemofx_glswirl_dispatch(struct glswirl *swirl, uint32_t texture);

extern float nemofx_glswirl_get_radius(struct glswirl *swirl);
extern float nemofx_glswirl_get_angle(struct glswirl *swirl);
extern float nemofx_glswirl_get_center_x(struct glswirl *swirl);
extern float nemofx_glswirl_get_center_y(struct glswirl *swirl);
extern uint32_t nemofx_glswirl_get_texture(struct glswirl *swirl);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
