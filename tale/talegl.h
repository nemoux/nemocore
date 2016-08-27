#ifndef	__NEMOTALE_GL_H__
#define	__NEMOTALE_GL_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>
#include <pixman.h>
#include <cairo.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
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
	GLuint pbo;
	GLuint pwidth;
	GLuint pheight;
	GLubyte *pbuffer;

	GLuint ftexture;
	GLuint fbo, dbo;
	GLuint fprogram;
	GLuint utexture;
	GLuint uwidth;
	GLuint uheight;
	GLuint utime;

	struct nemolistener destroy_listener;
};

struct taleegl;
struct talefbo;

extern struct nemotale *nemotale_create_gl(void);
extern void nemotale_destroy_gl(struct nemotale *tale);

extern int nemotale_make_current(struct nemotale *tale);

extern struct taleegl *nemotale_create_egl(EGLDisplay egl_display, EGLContext egl_context, EGLConfig egl_config, EGLNativeWindowType egl_window);
extern void nemotale_destroy_egl(struct taleegl *egl);
extern int nemotale_composite_egl(struct nemotale *tale, pixman_region32_t *region);
extern int nemotale_composite_egl_full(struct nemotale *tale);

extern struct talefbo *nemotale_create_fbo(GLuint texture, int32_t width, int32_t height);
extern void nemotale_destroy_fbo(struct talefbo *fbo);
extern int nemotale_resize_fbo(struct talefbo *fbo, int32_t width, int32_t height);
extern int nemotale_composite_fbo(struct nemotale *tale, pixman_region32_t *region);
extern int nemotale_composite_fbo_full(struct nemotale *tale);

extern int nemotale_node_resize_gl(struct talenode *node, int32_t width, int32_t height);
extern int nemotale_node_viewport_gl(struct talenode *node, int32_t width, int32_t height);
extern int nemotale_node_flush_gl(struct talenode *node);
extern int nemotale_node_flush_gl_pbo(struct talenode *node);
extern int nemotale_node_map_pbo(struct talenode *node);
extern int nemotale_node_unmap_pbo(struct talenode *node);
extern int nemotale_node_copy_pbo(struct talenode *node, int32_t x, int32_t y, int32_t width, int32_t height);
extern int nemotale_node_flush_gl_subimage(struct talenode *node);
extern int nemotale_node_map_subimage(struct talenode *node);
extern int nemotale_node_unmap_subimage(struct talenode *node);
extern int nemotale_node_copy_subimage(struct talenode *node, int32_t x, int32_t y, int32_t width, int32_t height);
extern int nemotale_node_filter_gl(struct talenode *node);

extern int nemotale_node_set_filter(struct talenode *node, const char *shader);
extern int nemotale_node_set_pbo(struct talenode *node, int has_pbo);

extern struct talenode *nemotale_node_create_gl(int32_t width, int32_t height);
extern int nemotale_node_prepare_gl(struct talenode *node);

static inline GLuint nemotale_node_get_texture(struct talenode *node)
{
	struct taleglnode *context = (struct taleglnode *)node->glcontext;

	if (node->has_filter != 0)
		return context->ftexture;

	return context->texture;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
