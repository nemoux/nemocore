#ifndef	__MOSIBACK_H__
#define	__MOSIBACK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct mosiback {
	struct nemotool *tool;

	struct eglcontext *egl;
	struct eglcanvas *eglcanvas;

	struct nemocanvas *canvas;

	struct nemotale *tale;
	struct talenode *node;

	int32_t width, height;

	uint32_t msecs;
};

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
