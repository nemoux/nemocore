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
	double width, height;
	double r;
};

#define NEMOSHOW_PATH(one)					((struct showpath *)container_of(one, struct showpath, base))
#define	NEMOSHOW_PATH_AT(one, at)		(NEMOSHOW_PATH(one)->at)

extern struct showone *nemoshow_path_create(int type);
extern void nemoshow_path_destroy(struct showone *one);

extern int nemoshow_path_arrange(struct showone *one);
extern int nemoshow_path_update(struct showone *one);

static inline void nemoshow_path_set_x0(struct showone *one, double x0)
{
	NEMOSHOW_PATH_AT(one, x0) = x0;
}

static inline void nemoshow_path_set_y0(struct showone *one, double y0)
{
	NEMOSHOW_PATH_AT(one, y0) = y0;
}

static inline void nemoshow_path_set_x1(struct showone *one, double x1)
{
	NEMOSHOW_PATH_AT(one, x1) = x1;
}

static inline void nemoshow_path_set_y1(struct showone *one, double y1)
{
	NEMOSHOW_PATH_AT(one, y1) = y1;
}

static inline void nemoshow_path_set_x2(struct showone *one, double x2)
{
	NEMOSHOW_PATH_AT(one, x2) = x2;
}

static inline void nemoshow_path_set_y2(struct showone *one, double y2)
{
	NEMOSHOW_PATH_AT(one, y2) = y2;
}

static inline void nemoshow_path_set_width(struct showone *one, double width)
{
	NEMOSHOW_PATH_AT(one, width) = width;
}

static inline void nemoshow_path_set_height(struct showone *one, double height)
{
	NEMOSHOW_PATH_AT(one, height) = height;
}

static inline void nemoshow_path_set_r(struct showone *one, double r)
{
	NEMOSHOW_PATH_AT(one, r) = r;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
