#ifndef	__NEMOTALE_GL_H__
#define	__NEMOTALE_GL_H__

#include <stdint.h>
#include <pixman.h>
#include <cairo.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <EGL/egl.h>

#include <nemotale.h>
#include <talenode.h>
#include <nemolistener.h>
#include <nemomatrix.h>

#ifndef EGL_TEXTURE_EXTERNAL_WL
#define	EGL_TEXTURE_EXTERNAL_WL		(0x31da)
#endif

#define NEMOTALE_BUFFER_AGE_COUNT		(2)

struct taleglnode {
	GLuint texture;

	struct nemolistener destroy_listener;
};

extern struct nemotale *nemotale_create_egl(EGLDisplay egl_display, EGLContext egl_context, EGLConfig egl_config);
extern void nemotale_destroy_egl(struct nemotale *tale);

extern int nemotale_attach_egl(struct nemotale *tale, EGLNativeWindowType window);
extern void nemotale_detach_egl(struct nemotale *tale);
extern int nemotale_resize_egl(struct nemotale *tale, int32_t width, int32_t height);
extern int nemotale_transform_egl(struct nemotale *tale, float d[9]);
extern int nemotale_composite_egl(struct nemotale *tale);

extern struct nemotale *nemotale_create_fbo(void);
extern void nemotale_destroy_fbo(struct nemotale *tale);

extern int nemotale_attach_fbo(struct nemotale *tale, GLuint texture, int32_t width, int32_t height);
extern void nemotale_detach_fbo(struct nemotale *tale);
extern int nemotale_composite_fbo(struct nemotale *tale);

extern struct talenode *nemotale_node_create_gl(int32_t width, int32_t height);
extern int nemotale_node_resize_gl(struct talenode *node, int32_t width, int32_t height);

static inline GLuint nemotale_node_get_texture(struct talenode *node)
{
	struct taleglnode *context = (struct taleglnode *)node->glcontext;

	return context->texture;
}

#endif
