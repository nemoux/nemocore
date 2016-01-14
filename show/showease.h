#ifndef	__NEMOSHOW_EASE_H__
#define	__NEMOSHOW_EASE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemoease.h>

#include <showone.h>

#define	NEMOSHOW_EASE_TYPE_MAX		(32)

struct showease {
	struct showone base;

	char type[NEMOSHOW_EASE_TYPE_MAX];

	double x0, y0;
	double x1, y1;

	struct nemoease ease;
};

#define	NEMOSHOW_EASE(one)					((struct showease *)container_of(one, struct showease, base))
#define	NEMOSHOW_EASE_AT(one, at)		(NEMOSHOW_EASE(one)->at)

extern struct showone *nemoshow_ease_create(void);
extern void nemoshow_ease_destroy(struct showone *one);

extern int nemoshow_ease_arrange(struct showone *one);
extern int nemoshow_ease_update(struct showone *one);

extern void nemoshow_ease_set_type(struct showone *one, int type);
extern void nemoshow_ease_set_bezier(struct showone *one, double x0, double y0, double x1, double y1);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
