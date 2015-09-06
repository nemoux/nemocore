#ifndef	__MOSIBACK_H__
#define	__MOSIBACK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <pixman.h>

#include <nemomosi.h>

struct mosiback {
	struct nemotool *tool;

	struct eglcontext *egl;
	struct eglcanvas *eglcanvas;

	struct nemocanvas *canvas;

	struct nemotale *tale;
	struct talenode *node;

	int32_t width, height;

	pixman_image_t *imgs[8];
	int nimgs, iimgs;

	int type;
	double r;
	struct nemomosi *mosi;
};

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
