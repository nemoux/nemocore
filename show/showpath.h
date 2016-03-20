#ifndef __NEMOSHOW_PATH_H__
#define __NEMOSHOW_PATH_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <showone.h>

#define NEMOSHOW_PATH_DASH_MAX			(32)

typedef enum {
	NEMOSHOW_NONE_PATH = 0,
	NEMOSHOW_NORMAL_PATH = 1,
	NEMOSHOW_ARRAY_PATH = 2,
	NEMOSHOW_LIST_PATH = 3,
	NEMOSHOW_LAST_PATH
} NemoShowPathType;

struct showpath {
	struct showone base;

	double strokes[4];
	double stroke_width;
	double fills[4];

	double alpha;

	double _strokes[4];
	double _fills[4];
	double _alpha;

	double pathlength;
	double pathsegment;
	double pathdeviation;
	uint32_t pathseed;

	double *pathdashes;
	int pathdashcount;

	uint32_t *cmds;
	int ncmds, scmds;

	double *points;
	int npoints, spoints;

	void *cc;
};

#define NEMOSHOW_PATH(one)					((struct showpath *)container_of(one, struct showpath, base))
#define	NEMOSHOW_PATH_AT(one, at)		(NEMOSHOW_PATH(one)->at)

extern struct showone *nemoshow_path_create(int type);
extern void nemoshow_path_destroy(struct showone *one);

extern int nemoshow_path_update(struct showone *one);

extern void nemoshow_path_set_stroke_cap(struct showone *one, int cap);
extern void nemoshow_path_set_stroke_join(struct showone *one, int join);

extern void nemoshow_path_clear(struct showone *one);
extern int nemoshow_path_moveto(struct showone *one, double x, double y);
extern int nemoshow_path_lineto(struct showone *one, double x, double y);
extern int nemoshow_path_cubicto(struct showone *one, double x0, double y0, double x1, double y1, double x2, double y2);
extern void nemoshow_path_arcto(struct showone *one, double x, double y, double width, double height, double from, double to, int needs_moveto);
extern int nemoshow_path_close(struct showone *one);
extern void nemoshow_path_cmd(struct showone *one, const char *cmd);
extern void nemoshow_path_arc(struct showone *one, double x, double y, double width, double height, double from, double to);
extern void nemoshow_path_text(struct showone *one, const char *font, int fontsize, const char *text, int textlength, double x, double y);
extern void nemoshow_path_append(struct showone *one, struct showone *src);

extern void nemoshow_path_translate(struct showone *one, double x, double y);
extern void nemoshow_path_scale(struct showone *one, double sx, double sy);
extern void nemoshow_path_rotate(struct showone *one, double ro);

extern void nemoshow_path_path_set_discrete_effect(struct showone *one, double segment, double deviation, uint32_t seed);
extern void nemoshow_path_path_set_dash_effect(struct showone *one, double *dashes, int dashcount);

extern void nemoshow_path_set_filter(struct showone *one, struct showone *filter);

static inline void nemoshow_path_set_alpha(struct showone *one, double alpha)
{
	struct showpath *path = NEMOSHOW_PATH(one);

	path->_alpha = alpha;

	nemoshow_one_dirty(one, NEMOSHOW_STYLE_DIRTY);
}

static inline void nemoshow_path_set_fill_color(struct showone *one, double r, double g, double b, double a)
{
	struct showpath *path = NEMOSHOW_PATH(one);

	path->_fills[NEMOSHOW_RED_COLOR] = r;
	path->_fills[NEMOSHOW_GREEN_COLOR] = g;
	path->_fills[NEMOSHOW_BLUE_COLOR] = b;
	path->_fills[NEMOSHOW_ALPHA_COLOR] = a;

	nemoshow_one_set_state(one, NEMOSHOW_FILL_STATE);

	nemoshow_one_dirty(one, NEMOSHOW_STYLE_DIRTY);
}

static inline void nemoshow_path_set_stroke_color(struct showone *one, double r, double g, double b, double a)
{
	struct showpath *path = NEMOSHOW_PATH(one);

	path->_strokes[NEMOSHOW_RED_COLOR] = r;
	path->_strokes[NEMOSHOW_GREEN_COLOR] = g;
	path->_strokes[NEMOSHOW_BLUE_COLOR] = b;
	path->_strokes[NEMOSHOW_ALPHA_COLOR] = a;

	nemoshow_one_set_state(one, NEMOSHOW_STROKE_STATE);

	nemoshow_one_dirty(one, NEMOSHOW_STYLE_DIRTY);
}

static inline void nemoshow_path_set_stroke_width(struct showone *one, double width)
{
	struct showpath *path = NEMOSHOW_PATH(one);

	path->stroke_width = width;

	nemoshow_one_dirty(one, NEMOSHOW_STYLE_DIRTY);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
