#ifndef __NEMOCOOK_EGL_H__
#define __NEMOCOOK_EGL_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <pixman.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

struct cookegl;
struct cookstate;
struct cookshader;

extern struct cookegl *nemocook_egl_create(EGLDisplay egl_display, EGLContext egl_context, EGLConfig egl_config, EGLNativeWindowType egl_window);
extern void nemocook_egl_destroy(struct cookegl *egl);

extern int nemocook_egl_resize(struct cookegl *egl, int width, int height);

extern int nemocook_egl_make_current(struct cookegl *egl);
extern int nemocook_egl_swap_buffers(struct cookegl *egl);

extern int nemocook_egl_has_swap_buffers_with_damage(struct cookegl *egl);

extern void nemocook_egl_fetch_damage(struct cookegl *egl, pixman_region32_t *damage);
extern void nemocook_egl_rotate_damage(struct cookegl *egl, pixman_region32_t *damage);
extern int nemocook_egl_swap_buffers_with_damage(struct cookegl *egl, pixman_region32_t *damage);

extern struct cookshader *nemocook_egl_use_shader(struct cookegl *egl, struct cookshader *shader);

extern void nemocook_egl_attach_state(struct cookegl *egl, struct cookstate *state);
extern void nemocook_egl_detach_state(struct cookegl *egl, int tag);
extern void nemocook_egl_update_state(struct cookegl *egl);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
