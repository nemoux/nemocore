#ifndef __NEMOUX_MIRO_BACK_H__
#define __NEMOUX_MIRO_BACK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemolist.h>
#include <nemolistener.h>

#include <showhelper.h>

struct miroback {
	struct nemotool *tool;

	struct nemotimer *timer;

	int32_t width, height;

	int32_t columns, rows;

	int32_t nmices, mmices;

	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;
	struct showone *canvas;
	struct showone *inner, *outer, *solid;
	struct showone *ease0;
	struct showone *ease1;
	struct showone *ease2;

	struct showone **cones;
	struct showone **rones;

	struct showone **bones;
	int32_t *nodes0;
	int32_t *nodes1;

	char *snddev;

	struct nemotimer *ptimer;
	pthread_mutex_t plock;

	int is_sleeping;

	struct nemolist tap_list;
	float tapsize;
};

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
