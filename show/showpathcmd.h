#ifndef __NEMOSHOW_PATHCMD_H__
#define __NEMOSHOW_PATHCMD_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <showone.h>

typedef enum {
	NEMOSHOW_PATH_NONE_CMD = 0,
	NEMOSHOW_PATH_MOVETO_CMD = 1,
	NEMOSHOW_PATH_LINETO_CMD = 2,
	NEMOSHOW_PATH_CURVETO_CMD = 3,
	NEMOSHOW_PATH_CLOSE_CMD = 4,
	NEMOSHOW_PATH_LAST_CMD
} NemoShowPathCmd;

struct showpathcmd {
	struct showone base;

	double x0, y0;
	double x1, y1;
	double x2, y2;
};

#define NEMOSHOW_PATHCMD(one)						((struct showpathcmd *)container_of(one, struct showpathcmd, base))
#define NEMOSHOW_PATHCMD_AT(one, at)		(NEMOSHOW_PATHCMD(one)->at)

extern struct showone *nemoshow_pathcmd_create(int type);
extern void nemoshow_pathcmd_destroy(struct showone *one);

extern int nemoshow_pathcmd_update(struct showone *one);

static inline void nemoshow_pathcmd_set_x0(struct showone *one, double x0)
{
	NEMOSHOW_PATHCMD_AT(one, x0) = x0;

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

static inline void nemoshow_pathcmd_set_y0(struct showone *one, double y0)
{
	NEMOSHOW_PATHCMD_AT(one, y0) = y0;

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

static inline void nemoshow_pathcmd_set_x1(struct showone *one, double x1)
{
	NEMOSHOW_PATHCMD_AT(one, x1) = x1;

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

static inline void nemoshow_pathcmd_set_y1(struct showone *one, double y1)
{
	NEMOSHOW_PATHCMD_AT(one, y1) = y1;

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

static inline void nemoshow_pathcmd_set_x2(struct showone *one, double x2)
{
	NEMOSHOW_PATHCMD_AT(one, x2) = x2;

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

static inline void nemoshow_pathcmd_set_y2(struct showone *one, double y2)
{
	NEMOSHOW_PATHCMD_AT(one, y2) = y2;

	nemoshow_one_dirty(one, NEMOSHOW_PATH_DIRTY);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
