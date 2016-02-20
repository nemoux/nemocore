#ifndef __NEMOUX_MOTE_BACK_H__
#define __NEMOUX_MOTE_BACK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemolist.h>
#include <nemolistener.h>
#include <nemomatrix.h>

#include <nemomote.h>
#include <showhelper.h>

struct moteback {
	struct nemotool *tool;

	struct nemotimer *timer;

	int32_t width, height;

	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;
	struct showone *canvasb;
	struct showone *canvasp;
	struct showone *canvast;
	
	struct showone *pipe;
	struct showone *quad;
	struct showone *mesh;

	struct nemomote *mote;
	struct moterandom random;
	struct nemozone box;
	struct nemozone disc;
	struct nemozone speed;

	struct nemoease ease;

	int type;
	char *logo;
	char *font;
	double fontsize;
	double textsize;
	double pixelsize;
	double speedmax;
	double mutualgravity;

	double colors0[4];
	double colors1[4];
	double tcolors0[4];
	double tcolors1[4];

	double secs;

	double ratio;

	int is_sleeping;
};

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
