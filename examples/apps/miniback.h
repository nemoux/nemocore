#ifndef	__MINIBACK_H__
#define	__MINIBACK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct miniback {
	struct nemotool *tool;

	struct eglcontext *egl;
	struct eglcanvas *eglcanvas;

	struct nemocanvas *canvas;

	struct nemotale *tale;

	int32_t width, height;
};

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
