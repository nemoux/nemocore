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
	NEMOSHOW_TEXT_ITEM = 7,
	NEMOSHOW_PATH_ITEM = 8,
	NEMOSHOW_PATHGROUP_ITEM = 9,
	NEMOSHOW_IMAGE_ITEM = 10,
	NEMOSHOW_STYLE_ITEM = 11,
	NEMOSHOW_GROUP_ITEM = 12,
	NEMOSHOW_LAST_ITEM
} NemoShowItemType;

typedef enum {
	NEMOSHOW_NONE_TRANSFORM = 0,
	NEMOSHOW_INTERN_TRANSFORM = (1 << 0),
	NEMOSHOW_EXTERN_TRANSFORM = (1 << 1),
	NEMOSHOW_CHILDREN_TRANSFORM = (1 << 8) | (1 << 0),
	NEMOSHOW_DIRECT_TRANSFORM = (1 << 9) | (1 << 0),
	NEMOSHOW_TSR_TRANSFORM = (1 << 10) | (1 << 0),
	NEMOSHOW_LAST_TRANSFORM
} NemoShowItemTransform;

struct showitem {
	struct showone base;

	int32_t event;

	struct showone *canvas;

	double x, y;
	double rx, ry;
	double width, height;
	double r;
	double inner;

	double from, to;

	uint32_t stroke;
	double strokes[4];
	double stroke_width;
	uint32_t fill;
	double fills[4];

	double alpha;

	struct showone *style;
	struct showone *clip;
	struct showone *blur;
	struct showone *shader;
	struct showone *matrix;
	struct showone *path;
	double length;

	struct showone *font;
	double fontsize;
	double fontascent;
	double fontdescent;
	const char *text;
	double textwidth, textheight;

	int transform;

	double tx, ty;
	double ro;
	double sx, sy;
	double px, py;

	void *cc;
};

#define NEMOSHOW_ITEM(one)					((struct showitem *)container_of(one, struct showitem, base))
#define	NEMOSHOW_ITEM_AT(one, at)		(NEMOSHOW_ITEM(one)->at)

extern struct showone *nemoshow_item_create(int type);
extern void nemoshow_item_destroy(struct showone *one);

extern int nemoshow_item_arrange(struct nemoshow *show, struct showone *one);
extern int nemoshow_item_update(struct nemoshow *show, struct showone *one);

extern void nemoshow_item_update_style(struct nemoshow *show, struct showone *one);
extern void nemoshow_item_update_child(struct nemoshow *show, struct showone *one);
extern void nemoshow_item_update_matrix(struct nemoshow *show, struct showone *one);
extern void nemoshow_item_update_text(struct nemoshow *show, struct showone *one);
extern void nemoshow_item_update_boundingbox(struct nemoshow *show, struct showone *one);

extern void nemoshow_item_set_matrix(struct showone *one, double m[9]);
extern void nemoshow_item_set_tsr(struct showone *one);
extern void nemoshow_item_set_shader(struct showone *one, struct showone *shader);
extern void nemoshow_item_set_blur(struct showone *one, struct showone *blur);
extern void nemoshow_item_set_clip(struct showone *one, struct showone *clip);
extern void nemoshow_item_set_image(struct showone *one, const char *uri);

extern void nemoshow_item_attach_one(struct showone *parent, struct showone *one);
extern void nemoshow_item_detach_one(struct showone *parent, struct showone *one);

static inline void nemoshow_item_set_event(struct showone *one, int32_t event)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->event = event;
}

static inline void nemoshow_item_set_canvas(struct showone *one, struct showone *canvas)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->canvas = canvas;
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

static inline void nemoshow_item_rotate(struct showone *one, double ro)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->ro = ro;

	nemoshow_one_dirty(one, NEMOSHOW_MATRIX_DIRTY);
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
