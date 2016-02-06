#ifndef __NEMOUX_EDGE_BACK_H__
#define __NEMOUX_EDGE_BACK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemolist.h>
#include <nemolistener.h>

#include <nemoenvs.h>
#include <showhelper.h>

struct edgeback {
	struct nemotool *tool;

	struct nemoenvs *envs;

	int32_t width, height;

	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;
	struct showone *canvas;
	struct showone *inner, *outer, *solid;
	struct showone *ease0;
	struct showone *ease1;
	struct showone *ease2;

	uint32_t roll_serial;

	float rollsize;
	float rollrange;
	uint32_t rolltimeout;
};

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
