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
	struct cookone one;

	struct cookshader *shader;

	int width, height;

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

int nemocook_egl_make_current(struct cookegl *egl)
{
	egl->shader = NULL;

	if (eglMakeCurrent(egl->display, egl->surface, egl->surface, egl->context) == EGL_FALSE)
		return -1;

	glViewport(0, 0, egl->width, egl->height);

	return 0;
}

int nemocook_egl_swap_buffers(struct cookegl *egl)
{
	if (eglSwapBuffers(egl->display, egl->surface) == EGL_FALSE)
		return -1;

	return 0;
}

struct cookshader *nemocook_egl_use_shader(struct cookegl *egl, struct cookshader *shader)
{
	if (egl->shader != shader) {
		nemocook_shader_use_program(shader);

		egl->shader = shader;
	}

	return egl->shader;
}

int nemocook_egl_resize(struct cookegl *egl, int width, int height)
{
	egl->width = width;
	egl->height = height;

	return 0;
}

struct cookegl *nemocook_egl_create(EGLDisplay egl_display, EGLContext egl_context, EGLConfig egl_config, EGLNativeWindowType egl_window)
{
	struct cookegl *egl;
	const char *extensions;

	egl = (struct cookegl *)malloc(sizeof(struct cookegl));
	if (egl == NULL)
		return NULL;
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

	nemocook_one_prepare(&egl->one);

	return egl;

err1:
	free(egl);

	return NULL;
}

void nemocook_egl_destroy(struct cookegl *egl)
{
	nemocook_one_finish(&egl->one);

	eglMakeCurrent(egl->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

	eglDestroySurface(egl->display, egl->surface);

	free(egl);
}

void nemocook_egl_attach_state(struct cookegl *egl, struct cookstate *state)
{
	nemocook_one_attach_state(&egl->one, state);
}

void nemocook_egl_detach_state(struct cookegl *egl, int tag)
{
	nemocook_one_detach_state(&egl->one, tag);
}

void nemocook_egl_update_state(struct cookegl *egl)
{
	nemocook_one_update_state(&egl->one);
}
