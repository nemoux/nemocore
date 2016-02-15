#ifndef __NEMOUX_ATOM_BACK_H__
#define __NEMOUX_ATOM_BACK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemolist.h>
#include <nemolistener.h>
#include <nemomatrix.h>

#include <showhelper.h>

struct atomback {
	struct nemotool *tool;

	struct nemotimer *timer;

	int32_t width, height;

	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;
	struct showone *canvasp;
	struct showone *inner, *outer, *solid;
	struct showone *ease0;
	struct showone *ease1;
	struct showone *ease2;

	struct showone *pipe;
	struct showone *one0;
	struct showone *one1;

	struct showone *canvasb;
	struct showone *canvast;
	struct showone *onet;

	int is_sleeping;
};

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
