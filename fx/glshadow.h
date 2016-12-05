#ifndef	__NEMOFX_GL_SHADOW_H__
#define	__NEMOFX_GL_SHADOW_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#define GLSHADOW_POINTLIGHTS_MAX				(16)

extern struct glshadow *nemofx_glshadow_create(int32_t width, int32_t height, int32_t lightscope);
extern void nemofx_glshadow_destroy(struct glshadow *shadow);

extern void nemofx_glshadow_set_pointlight_position(struct glshadow *shadow, int index, float x, float y);
extern void nemofx_glshadow_set_pointlight_color(struct glshadow *shadow, int index, float r, float g, float b);
extern void nemofx_glshadow_set_pointlight_size(struct glshadow *shadow, int index, float size);
extern void nemofx_glshadow_clear_pointlights(struct glshadow *shadow);

extern void nemofx_glshadow_resize(struct glshadow *shadow, int32_t width, int32_t height);
extern void nemofx_glshadow_dispatch(struct glshadow *shadow, uint32_t texture);

extern int32_t nemofx_glshadow_get_width(struct glshadow *shadow);
extern int32_t nemofx_glshadow_get_height(struct glshadow *shadow);
extern uint32_t nemofx_glshadow_get_texture(struct glshadow *shadow);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
