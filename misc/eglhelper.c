#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <eglhelper.h>

int egl_prepare_context(EGLNativeDisplayType nativedisplay, EGLDisplay *egl_display, EGLContext *egl_context, EGLConfig *egl_config, int buffer_size, int use_alpha)
{
#ifdef NEMOUX_WITH_OPENGL_ES3
	static const EGLint opaque_attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 1,
		EGL_GREEN_SIZE, 1,
		EGL_BLUE_SIZE, 1,
		EGL_ALPHA_SIZE, 0,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
		EGL_SAMPLE_BUFFERS, 1,
		EGL_SAMPLES, 4,
		EGL_NONE
	};
	static const EGLint alpha_attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 1,
		EGL_GREEN_SIZE, 1,
		EGL_BLUE_SIZE, 1,
		EGL_ALPHA_SIZE, 1,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
		EGL_SAMPLE_BUFFERS, 1,
		EGL_SAMPLES, 4,
		EGL_NONE
	};
	static const EGLint client_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 3,
		EGL_NONE
	};
#else
	static const EGLint opaque_attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 1,
		EGL_GREEN_SIZE, 1,
		EGL_BLUE_SIZE, 1,
		EGL_ALPHA_SIZE, 0,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_SAMPLE_BUFFERS, 1,
		EGL_SAMPLES, 4,
		EGL_NONE
	};
	static const EGLint alpha_attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 1,
		EGL_GREEN_SIZE, 1,
		EGL_BLUE_SIZE, 1,
		EGL_ALPHA_SIZE, 1,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_SAMPLE_BUFFERS, 1,
		EGL_SAMPLES, 4,
		EGL_NONE
	};
	static const EGLint client_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
#endif
	EGLConfig configs[16];
	EGLint major, minor;
	EGLint count, n, i, s;

	*egl_display = eglGetDisplay(nativedisplay);
	if (*egl_display == EGL_NO_DISPLAY)
		return -1;

	if (!eglInitialize(*egl_display, &major, &minor))
		return -1;

	if (!eglBindAPI(EGL_OPENGL_ES_API))
		return -1;

	if (!eglGetConfigs(*egl_display, NULL, 0, &count) || count < 1)
		return -1;

	if (!eglChooseConfig(*egl_display, use_alpha == 0 ? opaque_attribs : alpha_attribs, configs, count, &n))
		return -1;

	for (i = 0; i < n; i++) {
		eglGetConfigAttrib(*egl_display, configs[i], EGL_BUFFER_SIZE, &s);

		if (s == buffer_size) {
			*egl_config = configs[i];
			break;
		}
	}

	*egl_context = eglCreateContext(*egl_display, *egl_config, EGL_NO_CONTEXT, client_attribs);
	if (*egl_context == NULL)
		return -1;

	return 0;
}
