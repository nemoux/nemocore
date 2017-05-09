#ifndef	__GL_RENDERER_PRIVATE_H__
#define	__GL_RENDERER_PRIVATE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <wayland-server.h>

#include <canvas.h>
#include <renderer.h>
#include <compzhelper.h>

#ifndef EGL_TEXTURE_EXTERNAL_WL
#define	EGL_TEXTURE_EXTERNAL_WL		(0x31da)
#endif

#define	GLRENDERER_BUFFER_AGE_COUNT		(2)

typedef enum {
	GL_BUFFER_NULL_TYPE = 0,
	GL_BUFFER_SHM_TYPE = 1,
	GL_BUFFER_EGL_TYPE = 2,
	GL_BUFFER_LAST_TYPE
} GLBufferType;

struct glrenderer {
	struct nemorenderer base;

	EGLDisplay egl_display;
	EGLContext egl_context;
	EGLConfig egl_config;

	struct wl_array vertices;
	struct wl_array vtxcnt;

	const EGLint *attribs;

	PFNGLEGLIMAGETARGETTEXTURE2DOESPROC image_target_texture_2d;
	PFNEGLCREATEIMAGEKHRPROC create_image;
	PFNEGLDESTROYIMAGEKHRPROC destroy_image;
	PFNEGLSWAPBUFFERSWITHDAMAGEEXTPROC swap_buffers_with_damage;
	PFNEGLBINDWAYLANDDISPLAYWL bind_display;
	PFNEGLUNBINDWAYLANDDISPLAYWL unbind_display;
	PFNEGLQUERYWAYLANDBUFFERWL query_buffer;

	int has_bind_display;
	int has_configless_context;
	int has_surfaceless_context;
	int has_dmabuf_import;
	int has_egl_image_external;

	struct wl_signal destroy_signal;

	struct glcompz texture_shader_rgba;
	struct glcompz texture_shader_rgbx;
	struct glcompz texture_shader_egl_external;
	struct glcompz texture_shader_y_uv;
	struct glcompz texture_shader_y_u_v;
	struct glcompz texture_shader_y_xuxv;
	struct glcompz invert_color_shader;
	struct glcompz solid_shader;
	struct glcompz *current_shader;
};

struct glsurface {
	EGLSurface egl_surface;
	pixman_region32_t damages[GLRENDERER_BUFFER_AGE_COUNT];
};

struct glcontent {
	GLfloat colors[4];
	struct glcompz *shader;

	GLuint textures[3];
	int ntextures;
	int needs_full_upload;
	pixman_region32_t damage;

	GLenum format, pixeltype;

	EGLImageKHR images[3];
	GLenum target;
	int nimages;

	struct nemobuffer_reference buffer_reference;
	int buffer_type;
	int pitch, height;
	int y_inverted;

	struct glrenderer *renderer;

	struct nemocontent *content;

	pixman_image_t *image;

	struct wl_listener canvas_destroy_listener;
	struct wl_listener renderer_destroy_listener;
};

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
