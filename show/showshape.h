#ifndef	__NEMOSHOW_SHAPE_H__
#define	__NEMOSHOW_SHAPE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <showone.h>

struct showrect {
	struct showone base;

	double x, y;
	double width, height;
	double alpha;
};

#define	NEMOSHOW_RECT(one)		((struct showrect *)container_of(one, struct showrect, base))

extern struct showone *nemoshow_rect_create(void);
extern void nemoshow_rect_destroy(struct showone *one);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
