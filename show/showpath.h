#ifndef __NEMOSHOW_PATH_H__
#define __NEMOSHOW_PATH_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <showone.h>

typedef enum {
	NEMOSHOW_NONE_PATH = 0,
	NEMOSHOW_MOVETO_PATH = 1,
	NEMOSHOW_LINETO_PATH = 2,
	NEMOSHOW_CURVETO_PATH = 3,
	NEMOSHOW_CLOSE_PATH = 4,
	NEMOSHOW_CMD_PATH = 5,
	NEMOSHOW_TEXT_PATH = 6,
	NEMOSHOW_SVG_PATH = 7,
	NEMOSHOW_RECT_PATH = 8,
	NEMOSHOW_CIRCLE_PATH = 9,
	NEMOSHOW_LAST_PATH
} NemoShowPathType;

struct showpath {
	struct showone base;

	double x0, y0;
	double x1, y1;
	double x2, y2;
	double r;
};

#define NEMOSHOW_PATH(one)					((struct showpath *)container_of(one, struct showpath, base))
#define	NEMOSHOW_PATH_AT(one, at)		(NEMOSHOW_PATH(one)->at)

extern struct showone *nemoshow_path_create(int type);
extern void nemoshow_path_destroy(struct showone *one);

extern int nemoshow_path_arrange(struct nemoshow *show, struct showone *one);
extern int nemoshow_path_update(struct nemoshow *show, struct showone *one);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
