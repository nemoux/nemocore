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

struct showtext {
	struct showone base;

	double x, y;
};

#define	NEMOSHOW_RECT(one)		((struct showrect *)container_of(one, struct showrect, base))
#define	NEMOSHOW_TEXT(one)		((struct showtext *)container_of(one, struct showtext, base))

extern struct showone *nemoshow_rect_create(void);
extern void nemoshow_rect_destroy(struct showone *one);

extern struct showone *nemoshow_text_create(void);
extern void nemoshow_text_destroy(struct showone *one);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
