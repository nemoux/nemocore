#ifndef	__MOSIBACK_H__
#define	__MOSIBACK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <pixman.h>

struct mosiback {
	struct nemotool *tool;

	struct eglcontext *egl;
	struct eglcanvas *eglcanvas;

	struct nemocanvas *canvas;

	struct nemotale *tale;
	struct talenode *node;

	int32_t width, height;

	pixman_image_t *img0;
	int32_t col, row;
	double radius;
	uint32_t msecs;
};

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
