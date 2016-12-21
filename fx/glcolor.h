#ifndef	__NEMOFX_GL_COLOR_H__
#define	__NEMOFX_GL_COLOR_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

extern struct glcolor *nemofx_glcolor_create(int32_t width, int32_t height);
extern void nemofx_glcolor_destroy(struct glcolor *color);

extern void nemofx_glcolor_use_fbo(struct glcolor *color);

extern void nemofx_glcolor_set_color(struct glcolor *color, float r, float g, float b, float a);

extern void nemofx_glcolor_resize(struct glcolor *color, int32_t width, int32_t height);
extern uint32_t nemofx_glcolor_dispatch(struct glcolor *color);

extern uint32_t nemofx_glcolor_get_texture(struct glcolor *color);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
