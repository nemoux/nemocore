#ifndef	__GL_RENDERER_H__
#define	__GL_RENDERER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <EGL/egl.h>

#include <nemomisc.h>

struct nemocompz;
struct nemorenderer;
struct nemoscreen;
struct rendernode;

extern struct nemorenderer *glrenderer_create(struct rendernode *node, EGLNativeDisplayType display, const EGLint *visualid);
extern void glrenderer_destroy(struct nemorenderer *renderer);

extern void glrenderer_make_current(struct nemorenderer *base);

extern int glrenderer_prepare_screen(struct nemorenderer *base, struct nemoscreen *screen, EGLNativeWindowType window, const EGLint *visualid);
extern void glrenderer_finish_screen(struct nemorenderer *base, struct nemoscreen *screen);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
