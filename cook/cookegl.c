#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <pixman.h>

#include <cookegl.h>
#include <nemocook.h>
#include <nemomisc.h>

#define NEMOCOOK_BUFFER_AGE_COUNT		(2)

struct cookegl {
	EGLDisplay display;
	EGLContext context;
	EGLConfig config;
	EGLSurface surface;

	PFNGLEGLIMAGETARGETTEXTURE2DOESPROC image_target_texture_2d;
	PFNEGLCREATEIMAGEKHRPROC create_image;
	PFNEGLDESTROYIMAGEKHRPROC destroy_image;
	PFNEGLSWAPBUFFERSWITHDAMAGEEXTPROC swap_buffers_with_damage;
	PFNEGLBINDWAYLANDDISPLAYWL bind_display;
	PFNEGLUNBINDWAYLANDDISPLAYWL unbind_display;
	PFNEGLQUERYWAYLANDBUFFERWL query_buffer;

	int has_bind_display;
	int has_configless_context;
	int has_egl_image_external;
};

static int nemocook_egl_prerender(struct nemocook *cook)
{
	struct cookegl *egl = (struct cookegl *)cook->backend;

	if (eglMakeCurrent(egl->display, egl->surface, egl->surface, egl->context) == EGL_FALSE)
		return -1;

	return 0;
}

static int nemocook_egl_postrender(struct nemocook *cook)
{
	struct cookegl *egl = (struct cookegl *)cook->backend;

	if (eglSwapBuffers(egl->display, egl->surface) == EGL_FALSE)
		return -1;

	return 0;
}

static int nemocook_egl_resize(struct nemocook *cook, int width, int height)
{
	cook->width = width;
	cook->height = height;

	return 0;
}

static void nemocook_egl_finish(struct nemocook *cook)
{
	struct cookegl *egl = (struct cookegl *)cook->backend;

	eglMakeCurrent(egl->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

	eglDestroySurface(egl->display, egl->surface);

	free(egl);
}

int nemocook_prepare_egl(struct nemocook *cook, EGLDisplay egl_display, EGLContext egl_context, EGLConfig egl_config, EGLNativeWindowType egl_window)
{
	struct cookegl *egl;
	const char *extensions;

	egl = (struct cookegl *)malloc(sizeof(struct cookegl));
	if (egl == NULL)
		return -1;
	memset(egl, 0, sizeof(struct cookegl));

	egl->display = egl_display;
	egl->context = egl_context;
	egl->config = egl_config;

	if (!eglMakeCurrent(egl->display, EGL_NO_SURFACE, EGL_NO_SURFACE, egl->context))
		goto err1;

	extensions = (const char *)eglQueryString(egl->display, EGL_EXTENSIONS);
	if (extensions == NULL)
		goto err1;

	if (strstr(extensions, "EGL_WL_bind_wayland_display"))
		egl->has_bind_display = 1;

	if (strstr(extensions, "EGL_EXT_swap_buffers_with_damage") && strstr(extensions, "EGL_EXT_buffer_age"))
		egl->swap_buffers_with_damage = (void *)eglGetProcAddress("eglSwapBuffersWithDamageEXT");

	if (strstr(extensions, "EGL_MESA_configless_context"))
		egl->has_configless_context = 1;

	if (strstr(extensions, "GL_OES_EGL_image_external"))
		egl->has_egl_image_external = 1;

	egl->create_image = (void *)eglGetProcAddress("eglCreateImageKHR");
	egl->destroy_image = (void *)eglGetProcAddress("eglDestroyImageKHR");
	egl->bind_display = (void *)eglGetProcAddress("eglBindWaylandDisplayWL");
	egl->unbind_display = (void *)eglGetProcAddress("eglUnbindWaylandDisplayWL");
	egl->query_buffer = (void *)eglGetProcAddress("eglQueryWaylandBufferWL");
	egl->image_target_texture_2d = (void *)eglGetProcAddress("glEGLImageTargetTexture2DOES");

	egl->surface = eglCreateWindowSurface(egl->display, egl->config, egl_window, NULL);
	if (egl->surface == EGL_NO_SURFACE)
		goto err1;

	cook->backend_prerender = nemocook_egl_prerender;
	cook->backend_postrender = nemocook_egl_postrender;
	cook->backend_resize = nemocook_egl_resize;
	cook->backend_finish = nemocook_egl_finish;
	cook->backend = (void *)egl;

	return 0;

err1:
	free(egl);

	return -1;
}
