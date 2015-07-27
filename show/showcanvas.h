#ifndef	__NEMOSHOW_CANVAS_H__
#define	__NEMOSHOW_CANVAS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

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

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
