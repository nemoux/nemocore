#ifndef	__NEMOFX_GL_LIGHT_H__
#define	__NEMOFX_GL_LIGHT_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#define GLLIGHT_POINTLIGHTS_MAX				(16)

extern struct gllight *nemofx_gllight_create(int32_t width, int32_t height);
extern void nemofx_gllight_destroy(struct gllight *light);

extern void nemofx_gllight_use_fbo(struct gllight *light);

extern void nemofx_gllight_set_ambientlight_color(struct gllight *light, float r, float g, float b);

extern void nemofx_gllight_set_pointlight_position(struct gllight *light, int index, float x, float y);
extern void nemofx_gllight_set_pointlight_color(struct gllight *light, int index, float r, float g, float b);
extern void nemofx_gllight_set_pointlight_size(struct gllight *light, int index, float size);
extern void nemofx_gllight_set_pointlight_scope(struct gllight *light, int index, float scope);
extern void nemofx_gllight_clear_pointlights(struct gllight *light);

extern void nemofx_gllight_resize(struct gllight *light, int32_t width, int32_t height);
extern uint32_t nemofx_gllight_dispatch(struct gllight *light, uint32_t texture);

extern int32_t nemofx_gllight_get_width(struct gllight *light);
extern int32_t nemofx_gllight_get_height(struct gllight *light);
extern uint32_t nemofx_gllight_get_texture(struct gllight *light);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
