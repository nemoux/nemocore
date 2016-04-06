#ifndef	__NEMOSHOW_ITEM_H__
#define	__NEMOSHOW_ITEM_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <showone.h>

#include <nemolist.h>
#include <nemolistener.h>

#define NEMOSHOW_ITEM_PATH_DASH_MAX			(32)

typedef enum {
	NEMOSHOW_NONE_ITEM = 0,
	NEMOSHOW_LINE_ITEM = 1,
	NEMOSHOW_RECT_ITEM = 2,
	NEMOSHOW_RRECT_ITEM = 3,
	NEMOSHOW_CIRCLE_ITEM = 4,
	NEMOSHOW_ARC_ITEM = 5,
	NEMOSHOW_TEXT_ITEM = 6,
	NEMOSHOW_TEXTBOX_ITEM = 7,
	NEMOSHOW_PATH_ITEM = 8,
	NEMOSHOW_PATHTWICE_ITEM = 9,
	NEMOSHOW_PATHARRAY_ITEM = 10,
	NEMOSHOW_PATHLIST_ITEM = 11,
	NEMOSHOW_PATHGROUP_ITEM = 12,
	NEMOSHOW_POINTS_ITEM = 13,
	NEMOSHOW_POLYLINE_ITEM = 14,
	NEMOSHOW_POLYGON_ITEM = 15,
	NEMOSHOW_IMAGE_ITEM = 16,
	NEMOSHOW_SVG_ITEM = 17,
	NEMOSHOW_GROUP_ITEM = 18,
	NEMOSHOW_CONTAINER_ITEM = 19,
	NEMOSHOW_LAST_ITEM
} NemoShowItemType;

typedef enum {
	NEMOSHOW_ITEM_NORMAL_PICK = 0,
	NEMOSHOW_ITEM_PATH_PICK = 1,
	NEMOSHOW_ITEM_LAST_PICK
} NemoShowItemPick;

typedef enum {
	NEMOSHOW_ITEM_STROKE_PATH = (1 << 0),
	NEMOSHOW_ITEM_FILL_PATH = (1 << 1),
	NEMOSHOW_ITEM_BOTH_PATH = NEMOSHOW_ITEM_STROKE_PATH | NEMOSHOW_ITEM_FILL_PATH
} NemoShowItemPathIndex;

struct showitem {
	struct showone base;

	double x, y;
	double width, height;

	double ox, oy;

	double cx, cy;
	double r;

	double width0, height0;

	double from, to;

	double strokes[4];
	double stroke_width;
	double fills[4];

	double alpha;

	double _strokes[4];
	double _fills[4];
	double _alpha;

	double pathsegment;
	double pathdeviation;
	uint32_t pathseed;

	double *pathdashes;
	int pathdashcount;

	uint32_t pathselect;

	double fontsize;
	double fontascent;
	double fontdescent;
	double textwidth, textheight;
	double spacingmul, spacingadd;
	char *text;

	uint32_t *cmds;
	int ncmds, scmds;

	double *points;
	int npoints, spoints;

	char *uri;

	int transform;
	int pick;

	double matrix[9];

	double tx, ty;
	double ro;
	double sx, sy;
	double px, py;

	double ax, ay;

	void *cc;
};

#define NEMOSHOW_ITEM(one)					((struct showitem *)container_of(one, struct showitem, base))
#define NEMOSHOW_ITEM_AT(one, at)		(NEMOSHOW_ITEM(one)->at)
#define NEMOSHOW_ITEM_ONE(item)			(&(item)->base)

extern struct showone *nemoshow_item_create(int type);
extern void nemoshow_item_destroy(struct showone *one);

extern int nemoshow_item_update(struct showone *one);

extern void nemoshow_item_set_stroke_cap(struct showone *one, int cap);
extern void nemoshow_item_set_stroke_join(struct showone *one, int join);

extern void nemoshow_item_set_matrix(struct showone *one, double m[9]);
extern void nemoshow_item_set_shader(struct showone *one, struct showone *shader);
extern void nemoshow_item_set_filter(struct showone *one, struct showone *filter);
extern void nemoshow_item_set_clip(struct showone *one, struct showone *clip);
extern void nemoshow_item_set_font(struct showone *one, struct showone *font);
extern void nemoshow_item_set_path(struct showone *one, struct showone *path);
extern void nemoshow_item_set_uri(struct showone *one, const char *uri);
extern void nemoshow_item_set_text(struct showone *one, const char *text);

extern void nemoshow_item_attach_one(struct showone *parent, struct showone *one);
extern void nemoshow_item_detach_one(struct showone *one);
extern int nemoshow_item_above_one(struct showone *one, struct showone *above);
extern int nemoshow_item_below_one(struct showone *one, struct showone *below);

extern void nemoshow_item_path_clear(struct showone *one);
extern int nemoshow_item_path_moveto(struct showone *one, double x, double y);
extern int nemoshow_item_path_lineto(struct showone *one, double x, double y);
extern int nemoshow_item_path_cubicto(struct showone *one, double x0, double y0, double x1, double y1, double x2, double y2);
extern void nemoshow_item_path_arcto(struct showone *one, double x, double y, double width, double height, double from, double to, int needs_moveto);
extern int nemoshow_item_path_close(struct showone *one);
extern void nemoshow_item_path_cmd(struct showone *one, const char *cmd);
extern void nemoshow_item_path_arc(struct showone *one, double x, double y, double width, double height, double from, double to);
extern void nemoshow_item_path_text(struct showone *one, const char *font, int fontsize, const char *text, int textlength, double x, double y);
extern void nemoshow_item_path_append(struct showone *one, struct showone *src);
extern int nemoshow_item_path_load_svg(struct showone *one, const char *uri, double x, double y, double width, double height);
extern void nemoshow_item_path_use(struct showone *one, uint32_t paths);

extern void nemoshow_item_path_translate(struct showone *one, double x, double y);
extern void nemoshow_item_path_scale(struct showone *one, double sx, double sy);
extern void nemoshow_item_path_rotate(struct showone *one, double ro);

extern void nemoshow_item_path_set_discrete_effect(struct showone *one, double segment, double deviation, uint32_t seed);
extern void nemoshow_item_path_set_dash_effect(struct showone *one, double *dashes, int dashcount);

extern int nemoshow_item_path_contain_point(struct showone *one, double x, double y);

extern void nemoshow_item_clear_points(struct showone *one);
extern int nemoshow_item_append_point(struct showone *one, double x, double y);

extern int nemoshow_item_set_buffer(struct showone *one, char *buffer, uint32_t width, uint32_t height);
extern void nemoshow_item_put_buffer(struct showone *one);
extern int nemoshow_item_copy_buffer(struct showone *one, char *buffer, uint32_t width, uint32_t height);
extern int nemoshow_item_fill_buffer(struct showone *one, double r, double g, double b, double a);

extern int nemoshow_item_contain_one(struct showone *one, float x, float y);
extern struct showone *nemoshow_item_pick_one(struct showone *one, float x, float y);

extern void nemoshow_item_transform_to_global(struct showone *one, float sx, float sy, float *x, float *y);
extern void nemoshow_item_transform_from_global(struct showone *one, float x, float y, float *sx, float *sy);

static inline void nemoshow_item_set_x(struct showone *one, double x)
{
	NEMOSHOW_ITEM_AT(one, x) = x;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

static inline double nemoshow_item_get_x(struct showone *one)
{
	return NEMOSHOW_ITEM_AT(one, x);
}

static inline void nemoshow_item_set_y(struct showone *one, double y)
{
	NEMOSHOW_ITEM_AT(one, y) = y;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

static inline double nemoshow_item_get_y(struct showone *one)
{
	return NEMOSHOW_ITEM_AT(one, y);
}

static inline void nemoshow_item_set_roundx(struct showone *one, double ox)
{
	NEMOSHOW_ITEM_AT(one, ox) = ox;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

static inline void nemoshow_item_set_roundy(struct showone *one, double oy)
{
	NEMOSHOW_ITEM_AT(one, oy) = oy;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

static inline void nemoshow_item_set_width(struct showone *one, double width)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->width = width;

	nemoshow_one_set_state(one, NEMOSHOW_SIZE_STATE);

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY | NEMOSHOW_SIZE_DIRTY);
}

static inline double nemoshow_item_get_width(struct showone *one)
{
	return NEMOSHOW_ITEM_AT(one, width);
}

static inline void nemoshow_item_set_height(struct showone *one, double height)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->height = height;

	nemoshow_one_set_state(one, NEMOSHOW_SIZE_STATE);

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY | NEMOSHOW_SIZE_DIRTY);
}

static inline double nemoshow_item_get_height(struct showone *one)
{
	return NEMOSHOW_ITEM_AT(one, height);
}

static inline void nemoshow_item_set_base_width(struct showone *one, double width)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->width0 = width;

	nemoshow_one_dirty(one, NEMOSHOW_REDRAW_DIRTY);
}

static inline double nemoshow_item_get_base_width(struct showone *one)
{
	return NEMOSHOW_ITEM_AT(one, width0);
}

static inline void nemoshow_item_set_base_height(struct showone *one, double height)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->height0 = height;

	nemoshow_one_dirty(one, NEMOSHOW_REDRAW_DIRTY);
}

static inline double nemoshow_item_get_base_height(struct showone *one)
{
	return NEMOSHOW_ITEM_AT(one, height0);
}

static inline void nemoshow_item_set_cx(struct showone *one, double cx)
{
	NEMOSHOW_ITEM_AT(one, cx) = cx;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

static inline void nemoshow_item_set_cy(struct showone *one, double cy)
{
	NEMOSHOW_ITEM_AT(one, cy) = cy;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

static inline void nemoshow_item_set_r(struct showone *one, double r)
{
	NEMOSHOW_ITEM_AT(one, r) = r;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

static inline void nemoshow_item_set_from(struct showone *one, double from)
{
	NEMOSHOW_ITEM_AT(one, from) = from;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

static inline void nemoshow_item_set_to(struct showone *one, double to)
{
	NEMOSHOW_ITEM_AT(one, to) = to;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

static inline void nemoshow_item_set_fontsize(struct showone *one, double fontsize)
{
	NEMOSHOW_ITEM_AT(one, fontsize) = fontsize;

	nemoshow_one_dirty(one, NEMOSHOW_TEXT_DIRTY);
}

static inline void nemoshow_item_set_spacing(struct showone *one, double spacingmul, double spacingadd)
{
	NEMOSHOW_ITEM_AT(one, spacingmul) = spacingmul;
	NEMOSHOW_ITEM_AT(one, spacingadd) = spacingadd;

	nemoshow_one_dirty(one, NEMOSHOW_TEXT_DIRTY);
}

static inline void nemoshow_item_set_alpha(struct showone *one, double alpha)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->_alpha = alpha;

	nemoshow_one_dirty(one, NEMOSHOW_STYLE_DIRTY);
}

static inline void nemoshow_item_set_fill_color(struct showone *one, double r, double g, double b, double a)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->_fills[NEMOSHOW_RED_COLOR] = r;
	item->_fills[NEMOSHOW_GREEN_COLOR] = g;
	item->_fills[NEMOSHOW_BLUE_COLOR] = b;
	item->_fills[NEMOSHOW_ALPHA_COLOR] = a;

	nemoshow_one_set_state(one, NEMOSHOW_FILL_STATE);

	nemoshow_one_dirty(one, NEMOSHOW_STYLE_DIRTY);
}

static inline void nemoshow_item_set_stroke_color(struct showone *one, double r, double g, double b, double a)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->_strokes[NEMOSHOW_RED_COLOR] = r;
	item->_strokes[NEMOSHOW_GREEN_COLOR] = g;
	item->_strokes[NEMOSHOW_BLUE_COLOR] = b;
	item->_strokes[NEMOSHOW_ALPHA_COLOR] = a;

	nemoshow_one_set_state(one, NEMOSHOW_STROKE_STATE);

	nemoshow_one_dirty(one, NEMOSHOW_STYLE_DIRTY);
}

static inline void nemoshow_item_set_stroke_width(struct showone *one, double width)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->stroke_width = width;

	nemoshow_one_dirty(one, NEMOSHOW_STYLE_DIRTY | NEMOSHOW_SHAPE_DIRTY);
}

static inline void nemoshow_item_set_anchor(struct showone *one, double ax, double ay)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->ax = ax;
	item->ay = ay;

	nemoshow_one_set_state(one, NEMOSHOW_ANCHOR_STATE | NEMOSHOW_TRANSFORM_STATE);

	nemoshow_one_dirty(one, NEMOSHOW_MATRIX_DIRTY);
}

static inline const char *nemoshow_item_get_text(struct showone *one)
{
	return NEMOSHOW_ITEM_AT(one, text);
}

static inline void nemoshow_item_translate(struct showone *one, double tx, double ty)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->tx = tx;
	item->ty = ty;

	nemoshow_one_set_state(one, NEMOSHOW_TRANSFORM_STATE);

	nemoshow_one_dirty(one, NEMOSHOW_MATRIX_DIRTY);
}

static inline double nemoshow_item_get_tx(struct showone *one)
{
	return NEMOSHOW_ITEM_AT(one, tx);
}

static inline double nemoshow_item_get_ty(struct showone *one)
{
	return NEMOSHOW_ITEM_AT(one, ty);
}

static inline void nemoshow_item_rotate(struct showone *one, double ro)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->ro = ro;

	nemoshow_one_set_state(one, NEMOSHOW_TRANSFORM_STATE);

	nemoshow_one_dirty(one, NEMOSHOW_MATRIX_DIRTY);
}

static inline double nemoshow_item_get_rotate(struct showone *one)
{
	return NEMOSHOW_ITEM_AT(one, ro);
}

static inline void nemoshow_item_scale(struct showone *one, double sx, double sy)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->sx = sx;
	item->sy = sy;

	nemoshow_one_set_state(one, NEMOSHOW_TRANSFORM_STATE);

	nemoshow_one_dirty(one, NEMOSHOW_MATRIX_DIRTY);
}

static inline void nemoshow_item_pivot(struct showone *one, double px, double py)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->px = px;
	item->py = py;

	nemoshow_one_set_state(one, NEMOSHOW_TRANSFORM_STATE);

	nemoshow_one_dirty(one, NEMOSHOW_MATRIX_DIRTY);
}

static inline void nemoshow_item_set_pick(struct showone *one, int pick)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->pick = pick;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
