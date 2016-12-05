#ifndef	__NEMOFX_GL_FILTER_H__
#define	__NEMOFX_GL_FILTER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

extern struct glfilter *nemofx_glfilter_create(int32_t width, int32_t height);
extern void nemofx_glfilter_destroy(struct glfilter *filter);

extern void nemofx_glfilter_set_program(struct glfilter *filter, const char *shaderpath);

extern void nemofx_glfilter_resize(struct glfilter *filter, int32_t width, int32_t height);
extern void nemofx_glfilter_dispatch(struct glfilter *filter, uint32_t texture);

extern int32_t nemofx_glfilter_get_width(struct glfilter *filter);
extern int32_t nemofx_glfilter_get_height(struct glfilter *filter);
extern uint32_t nemofx_glfilter_get_texture(struct glfilter *filter);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
