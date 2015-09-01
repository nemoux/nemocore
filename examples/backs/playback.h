#ifndef	__PLAYBACK_H__
#define	__PLAYBACK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <gsthelper.h>
#include <glibhelper.h>

struct playback {
	struct nemotool *tool;

	struct eglcontext *egl;
	struct eglcanvas *eglcanvas;

	struct nemocanvas *canvas;

	struct nemotale *tale;
	struct talenode *node;

	struct nemogst *gst;

	int32_t width, height;
};

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
