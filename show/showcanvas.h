#ifndef	__NEMOSHOW_CANVAS_H__
#define	__NEMOSHOW_CANVAS_H__

#include <stdint.h>

#include <showone.h>

#define	NEMOSHOW_CANVAS_TYPE_MAX		(32)

struct showcanvas {
	struct showone base;

	char type[NEMOSHOW_CANVAS_TYPE_MAX];

	double width, height;
};

#define	NEMOSHOW_CANVAS(one)		((struct showcanvas *)container_of(one, struct showcanvas, base))

extern struct showone *nemoshow_canvas_create(void);
extern void nemoshow_canvas_destroy(struct showone *one);

extern void nemoshow_canvas_dump(struct showone *one, FILE *out);

#endif
