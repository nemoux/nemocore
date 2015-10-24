#ifndef	__NEMOSHOW_ITEM_H__
#define	__NEMOSHOW_ITEM_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <showone.h>

typedef enum {
	NEMOSHOW_NONE_ITEM = 0,
	NEMOSHOW_RECT_ITEM = 1,
	NEMOSHOW_RRECT_ITEM = 2,
	NEMOSHOW_CIRCLE_ITEM = 3,
	NEMOSHOW_ARC_ITEM = 4,
	NEMOSHOW_PIE_ITEM = 5,
	NEMOSHOW_DONUT_ITEM = 6,
	NEMOSHOW_RING_ITEM = 7,
	NEMOSHOW_TEXT_ITEM = 8,
	NEMOSHOW_PATH_ITEM = 9,
	NEMOSHOW_PATHGROUP_ITEM = 10,
	NEMOSHOW_BITMAP_ITEM = 11,
	NEMOSHOW_IMAGE_ITEM = 12,
	NEMOSHOW_SVG_ITEM = 13,
	NEMOSHOW_GROUP_ITEM = 14,
	NEMOSHOW_STYLE_ITEM = 15,
	NEMOSHOW_LAST_ITEM
} NemoShowItemType;

typedef enum {
	NEMOSHOW_NONE_TRANSFORM = (0 << 0),
	NEMOSHOW_EXTERN_TRANSFORM = (1 << 0),
	NEMOSHOW_INTERN_TRANSFORM = (1 << 1),
	NEMOSHOW_CHILDREN_TRANSFORM = (1 << 7) | (1 << 1),
	NEMOSHOW_DIRECT_TRANSFORM = (1 << 8) | (1 << 1),
	NEMOSHOW_TSR_TRANSFORM = (1 << 9) | (1 << 1),
	NEMOSHOW_LAST_TRANSFORM
} NemoShowItemTransform;

struct showitem {
	struct showone base;

	struct showone *canvas;

	double x, y;
	double rx, ry;
	double width, height;
	double r;
	double inner;

	int has_size;

	double from, to;

	uint32_t stroke;
	double strokes[4];
	double stroke_width;
	uint32_t fill;
	double fills[4];

	double alpha;

	double pathlength;

	double fontsize;
	double fontascent;
	double fontdescent;
	const char *text;
	double textwidth, textheight;

	char *uri;

	int transform;

	double matrix[9];

	double tx, ty;
	double ro;
	double sx, sy;
	double px, py;

	double ax, ay;
	int has_anchor;

	void *cc;
};

#define NEMOSHOW_ITEM(one)					((struct showitem *)container_of(one, struct showitem, base))
#define	NEMOSHOW_ITEM_AT(one, at)		(NEMOSHOW_ITEM(one)->at)

extern struct showone *nemoshow_item_create(int type);
extern void nemoshow_item_destroy(struct showone *one);

extern int nemoshow_item_arrange(struct nemoshow *show, struct showone *one);
extern int nemoshow_item_update(struct nemoshow *show, struct showone *one);

extern void nemoshow_item_update_boundingbox(struct nemoshow *show, struct showone *one);

extern void nemoshow_item_set_matrix(struct showone *one, double m[9]);
extern void nemoshow_item_set_tsr(struct showone *one);
extern void nemoshow_item_set_shader(struct showone *one, struct showone *shader);
extern void nemoshow_item_set_filter(struct showone *one, struct showone *filter);
extern void nemoshow_item_set_clip(struct showone *one, struct showone *clip);
extern void nemoshow_item_set_font(struct showone *one, struct showone *font);
extern void nemoshow_item_set_path(struct showone *one, struct showone *path);
extern void nemoshow_item_set_src(struct showone *one, struct showone *src);
extern void nemoshow_item_set_uri(struct showone *one, const char *uri);
extern void nemoshow_item_set_text(struct showone *one, const char *text);

extern void nemoshow_item_attach_one(struct showone *parent, struct showone *one);
extern void nemoshow_item_detach_one(struct showone *parent, struct showone *one);

extern void nemoshow_item_path_clear(struct showone *one);
extern void nemoshow_item_path_moveto(struct showone *one, double x, double y);
extern void nemoshow_item_path_lineto(struct showone *one, double x, double y);
extern void nemoshow_item_path_cubicto(struct showone *one, double x0, double y0, double x1, double y1, double x2, double y2);
extern void nemoshow_item_path_close(struct showone *one);
extern void nemoshow_item_path_cmd(struct showone *one, const char *cmd);
extern void nemoshow_item_path_arc(struct showone *one, double x, double y, double width, double height, double from, double to);

extern int nemoshow_item_load_svg(struct showone *one, const char *uri);

static inline void nemoshow_item_set_canvas(struct showone *one, struct showone *canvas)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->canvas = canvas;
}

static inline struct showone *nemoshow_item_get_canvas(struct showone *one)
{
	return NEMOSHOW_ITEM_AT(one, canvas);
}

static inline struct showone *nemoshow_item_get_style(struct showone *one)
{
	struct showone *style = NEMOSHOW_REF(one, NEMOSHOW_STYLE_REF);

	return style != NULL ? style : one;
}

static inline void nemoshow_item_set_x(struct showone *one, double x)
{
	NEMOSHOW_ITEM_AT(one, x) = x;
}

static inline double nemoshow_item_get_x(struct showone *one)
{
	return NEMOSHOW_ITEM_AT(one, x);
}

static inline void nemoshow_item_set_y(struct showone *one, double y)
{
	NEMOSHOW_ITEM_AT(one, y) = y;
}

static inline double nemoshow_item_get_y(struct showone *one)
{
	return NEMOSHOW_ITEM_AT(one, y);
}

static inline void nemoshow_item_set_rx(struct showone *one, double rx)
{
	NEMOSHOW_ITEM_AT(one, rx) = rx;
}

static inline void nemoshow_item_set_ry(struct showone *one, double ry)
{
	NEMOSHOW_ITEM_AT(one, ry) = ry;
}

static inline void nemoshow_item_set_width(struct showone *one, double width)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->width = width;
	item->has_size = 1;
}

static inline void nemoshow_item_set_height(struct showone *one, double height)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->height = height;
	item->has_size = 1;
}

static inline void nemoshow_item_set_r(struct showone *one, double r)
{
	NEMOSHOW_ITEM_AT(one, r) = r;
}

static inline void nemoshow_item_set_inner(struct showone *one, double inner)
{
	NEMOSHOW_ITEM_AT(one, inner) = inner;
}

static inline void nemoshow_item_set_from(struct showone *one, double from)
{
	NEMOSHOW_ITEM_AT(one, from) = from;
}

static inline void nemoshow_item_set_to(struct showone *one, double to)
{
	NEMOSHOW_ITEM_AT(one, to) = to;
}

static inline void nemoshow_item_set_fontsize(struct showone *one, double fontsize)
{
	NEMOSHOW_ITEM_AT(one, fontsize) = fontsize;
}

static inline void nemoshow_item_set_alpha(struct showone *one, double alpha)
{
	NEMOSHOW_ITEM_AT(one, alpha) = alpha;
}

static inline void nemoshow_item_set_fill_color(struct showone *one, double r, double g, double b, double a)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->fills[2] = r;
	item->fills[1] = g;
	item->fills[0] = b;
	item->fills[3] = a;

	item->fill = 1;
}

static inline void nemoshow_item_set_stroke_color(struct showone *one, double r, double g, double b, double a)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->strokes[2] = r;
	item->strokes[1] = g;
	item->strokes[0] = b;
	item->strokes[3] = a;

	item->stroke = 1;
}

static inline void nemoshow_item_set_stroke_width(struct showone *one, double width)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->stroke_width = width;
}

static inline void nemoshow_item_set_anchor(struct showone *one, double ax, double ay)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->ax = ax;
	item->ay = ay;

	item->has_anchor = 1;
}

static inline double nemoshow_item_get_outer(struct showone *one)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	return one->outer + (item->stroke != 0 ? item->stroke_width : 0.0f);
}

static inline void nemoshow_item_translate(struct showone *one, double tx, double ty)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->tx = tx;
	item->ty = ty;

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

	nemoshow_one_dirty(one, NEMOSHOW_MATRIX_DIRTY);
}

static inline void nemoshow_item_pivot(struct showone *one, double px, double py)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->px = px;
	item->py = py;

	nemoshow_one_dirty(one, NEMOSHOW_MATRIX_DIRTY);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
