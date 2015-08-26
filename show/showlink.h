#ifndef	__NEMOSHOW_LINK_H__
#define	__NEMOSHOW_LINK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <showone.h>

struct showlink {
	struct showone base;

	struct showone *canvas;

	struct showone *head;
	struct showone *tail;

	uint32_t stroke;
	double strokes[4];
	double stroke_width;

	double alpha;

	struct showone *filter;

	void *cc;
};

#define NEMOSHOW_LINK(one)					((struct showlink *)container_of(one, struct showlink, base))
#define NEMOSHOW_LINK_AT(one, at)		(NEMOSHOW_LINK(one)->at)

extern struct showone *nemoshow_link_create(void);
extern void nemoshow_link_destroy(struct showone *one);

extern int nemoshow_link_arrange(struct nemoshow *show, struct showone *one);
extern int nemoshow_link_update(struct nemoshow *show, struct showone *one);

extern void nemoshow_link_set_filter(struct showone *one, struct showone *filter);

static inline void nemoshow_link_set_canvas(struct showone *one, struct showone *canvas)
{
	struct showlink *link = NEMOSHOW_LINK(one);

	link->canvas = canvas;
}

static inline void nemoshow_link_set_head(struct showone *one, struct showone *head)
{
	struct showlink *link = NEMOSHOW_LINK(one);

	link->head = head;

	nemoshow_one_reference_one(one, head);
}

static inline void nemoshow_link_set_tail(struct showone *one, struct showone *tail)
{
	struct showlink *link = NEMOSHOW_LINK(one);

	link->tail = tail;

	nemoshow_one_reference_one(one, tail);
}

static inline void nemoshow_link_set_stroke_color(struct showone *one, double r, double g, double b, double a)
{
	struct showlink *link = NEMOSHOW_LINK(one);

	link->strokes[2] = r;
	link->strokes[1] = g;
	link->strokes[0] = b;
	link->strokes[3] = a;
}

static inline void nemoshow_link_set_stroke_width(struct showone *one, double width)
{
	struct showlink *link = NEMOSHOW_LINK(one);

	link->stroke_width = width;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
