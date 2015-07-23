#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <eglhelper.h>

int egl_choose_config(EGLDisplay egl_display, const EGLint *attribs, const EGLint *visualid, EGLConfig *config)
{
	EGLint count = 0;
	EGLint matched = 0;
	EGLint id;
	EGLConfig *configs;
	int i;

	if (!eglGetConfigs(egl_display, NULL, 0, &count) || count < 1)
		return -1;

	configs = (EGLConfig *)malloc(sizeof(EGLConfig) * count);
	if (configs == NULL)
		return -1;
	memset(configs, 0, sizeof(EGLConfig) * count);

	if (!eglChooseConfig(egl_display, attribs, configs, count, &matched))
		goto err1;

	for (i = 0; i < matched; i++) {
		if (visualid != NULL) {
			if (!eglGetConfigAttrib(egl_display, configs[i], EGL_NATIVE_VISUAL_ID, &id))
				continue;

			if (id != 0 && id != *visualid)
				continue;
		}

		*config = configs[i];

		free(configs);

		return 0;
	}

err1:
	free(configs);

	return -1;
}

int egl_prepare_context(EGLNativeDisplayType nativedisplay, EGLDisplay *egl_display, EGLContext *egl_context, EGLConfig *egl_config, int use_alpha, const EGLint *visualid)
{
	static const EGLint opaque_attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 1,
		EGL_GREEN_SIZE, 1,
		EGL_BLUE_SIZE, 1,
		EGL_ALPHA_SIZE, 0,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};
	static const EGLint alpha_attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 1,
		EGL_GREEN_SIZE, 1,
		EGL_BLUE_SIZE, 1,
		EGL_ALPHA_SIZE, 1,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};
	static const EGLint client_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	EGLConfig config;
	EGLint major, minor;
	const char *extensions;

	*egl_display = eglGetDisplay(nativedisplay);
	if (*egl_display == EGL_NO_DISPLAY)
		return -1;

	if (!eglInitialize(*egl_display, &major, &minor))
		return -1;

	if (!eglBindAPI(EGL_OPENGL_ES_API))
		return -1;

	if (egl_choose_config(*egl_display, use_alpha == 0 ? opaque_attribs : alpha_attribs, visualid, egl_config) < 0)
		return -1;

	config = *egl_config;

	extensions = (const char *)eglQueryString(*egl_display, EGL_EXTENSIONS);
	if (extensions == NULL)
		return -1;

#ifdef EGL_MESA_configless_context
	if (strstr(extensions, "EGL_MESA_configless_context"))
		config = EGL_NO_CONFIG_MESA;
#endif

	*egl_context = eglCreateContext(*egl_display, config, EGL_NO_CONTEXT, client_attribs);
	if (*egl_context == NULL)
		return -1;

	return 0;
}
