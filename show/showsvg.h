#ifndef	__NEMOSHOW_SVG_H__
#define	__NEMOSHOW_SVG_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <showone.h>

struct showsvg {
	struct showone base;

	int32_t event;

	struct showone *canvas;

	double width, height;

	struct showone *matrix;
	struct showone *clip;

	char *uri;

	int transform;

	double tx, ty;
	double ro;
	double sx, sy;
	double px, py;

	void *cc;
};

#define	NEMOSHOW_SVG(one)						((struct showsvg *)container_of(one, struct showsvg, base))
#define	NEMOSHOW_SVG_AT(one, at)		(NEMOSHOW_SVG(one)->at)

extern struct showone *nemoshow_svg_create(void);
extern void nemoshow_svg_destroy(struct showone *one);

extern int nemoshow_svg_arrange(struct nemoshow *show, struct showone *one);
extern int nemoshow_svg_update(struct nemoshow *show, struct showone *one);

extern void nemoshow_svg_set_tsr(struct showone *one);
extern void nemoshow_svg_set_uri(struct showone *one, const char *uri);

static inline void nemoshow_svg_set_event(struct showone *one, int32_t event)
{
	struct showsvg *svg = NEMOSHOW_SVG(one);

	svg->event = event;
}

static inline void nemoshow_svg_set_canvas(struct showone *one, struct showone *canvas)
{
	struct showsvg *svg = NEMOSHOW_SVG(one);

	svg->canvas = canvas;
}

static inline void nemoshow_svg_translate(struct showone *one, double tx, double ty)
{
	struct showsvg *svg = NEMOSHOW_SVG(one);

	svg->tx = tx;
	svg->ty = ty;

	nemoshow_one_dirty(one, NEMOSHOW_MATRIX_DIRTY);
}

static inline void nemoshow_svg_rotate(struct showone *one, double ro)
{
	struct showsvg *svg = NEMOSHOW_SVG(one);

	svg->ro = ro;

	nemoshow_one_dirty(one, NEMOSHOW_MATRIX_DIRTY);
}

static inline void nemoshow_svg_scale(struct showone *one, double sx, double sy)
{
	struct showsvg *svg = NEMOSHOW_SVG(one);

	svg->sx = sx;
	svg->sy = sy;

	nemoshow_one_dirty(one, NEMOSHOW_MATRIX_DIRTY);
}

static inline void nemoshow_svg_pivot(struct showone *one, double px, double py)
{
	struct showsvg *svg = NEMOSHOW_SVG(one);

	svg->px = px;
	svg->py = py;

	nemoshow_one_dirty(one, NEMOSHOW_MATRIX_DIRTY);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
