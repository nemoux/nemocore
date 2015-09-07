#ifndef	__MOTEBACK_H__
#define	__MOTEBACK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <pixman.h>

#include <nemomote.h>

#include <nemotool.h>
#include <nemotimer.h>

struct moteback {
	struct nemotool *tool;

	struct eglcontext *egl;
	struct eglcanvas *eglcanvas;

	struct nemocanvas *canvas;

	struct nemotale *tale;
	struct talenode *node;

	int32_t width, height;

	struct nemomote *mote;
	struct moterandom random;
	struct nemozone box;
	struct nemozone disc;
	struct nemozone speed;

	struct motetween tween;

	double secs;

	pixman_image_t *numbers[10];

	struct nemotimer *timer;
};

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
