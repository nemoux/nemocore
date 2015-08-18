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

	struct showone *canvas;
	struct showone *group;

	double width, height;

	struct showone *matrix;

	int transform;

	void *cc;
};

#define	NEMOSHOW_SVG(one)						((struct showsvg *)container_of(one, struct showsvg, base))
#define	NEMOSHOW_SVG_AT(one, at)		(NEMOSHOW_SVG(one)->at)

extern struct showone *nemoshow_svg_create(void);
extern void nemoshow_svg_destroy(struct showone *one);

extern int nemoshow_svg_arrange(struct nemoshow *show, struct showone *one);
extern int nemoshow_svg_update(struct nemoshow *show, struct showone *one);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
