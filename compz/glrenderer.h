#ifndef	__GL_RENDERER_H__
#define	__GL_RENDERER_H__

#include <EGL/egl.h>

#include <nemomisc.h>

struct nemocompz;
struct nemorenderer;
struct nemoscreen;
struct rendernode;

extern struct nemorenderer *glrenderer_create(struct rendernode *node, EGLNativeDisplayType display, int use_alpha, const EGLint *visualid);
extern void glrenderer_destroy(struct nemorenderer *renderer);

extern void glrenderer_make_current(struct nemorenderer *base);

extern int glrenderer_prepare_screen(struct nemorenderer *base, struct nemoscreen *screen, EGLNativeWindowType window, int use_alpha, const EGLint *visualid);
extern void glrenderer_finish_screen(struct nemorenderer *base, struct nemoscreen *screen);

#ifdef NEMOUX_WITH_MESA_RENDERNODE
extern void glrenderer_set_render_nodes(struct nemocompz *compz);
#endif

#endif
