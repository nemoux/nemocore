#ifndef	__MOSIBACK_H__
#define	__MOSIBACK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <pixman.h>

struct mosione {
	double x, y;
	double r;

	uint8_t c[4];
	uint8_t c0[4];
	uint8_t c1[4];

	uint32_t stime, etime;
};

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

	int32_t col, row;
	double radius;

	struct mosione *ones;
	int nones;
};

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
