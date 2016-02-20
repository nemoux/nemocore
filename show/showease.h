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

extern struct showone *nemoeases[NEMOEASE_LAST_TYPE];

#define NEMOSHOW_LINEAR_EASE							(nemoeases[NEMOEASE_LINEAR_TYPE])
#define NEMOSHOW_QUADRATIC_IN_EASE				(nemoeases[NEMOEASE_QUADRATIC_IN_TYPE])
#define NEMOSHOW_QUADRATIC_OUT_EASE				(nemoeases[NEMOEASE_QUADRATIC_OUT_TYPE])
#define NEMOSHOW_QUADRATIC_INOUT_EASE			(nemoeases[NEMOEASE_QUADRATIC_INOUT_TYPE])
#define NEMOSHOW_CUBIC_IN_EASE						(nemoeases[NEMOEASE_CUBIC_IN_TYPE])
#define NEMOSHOW_CUBIC_OUT_EASE						(nemoeases[NEMOEASE_CUBIC_OUT_TYPE])
#define NEMOSHOW_CUBIC_INOUT_EASE					(nemoeases[NEMOEASE_CUBIC_INOUT_TYPE])
#define NEMOSHOW_QUARTIC_IN_EASE					(nemoeases[NEMOEASE_QUARTIC_IN_TYPE])
#define NEMOSHOW_QUARTIC_OUT_EASE					(nemoeases[NEMOEASE_QUARTIC_OUT_TYPE])
#define NEMOSHOW_QUARTIC_INOUT_EASE				(nemoeases[NEMOEASE_QUARTIC_INOUT_TYPE])
#define NEMOSHOW_QUINTIC_IN_EASE					(nemoeases[NEMOEASE_QUINTIC_IN_TYPE])
#define NEMOSHOW_QUINTIC_OUT_EASE					(nemoeases[NEMOEASE_QUINTIC_OUT_TYPE])
#define NEMOSHOW_QUINTIC_INOUT_EASE				(nemoeases[NEMOEASE_QUINTIC_INOUT_TYPE])
#define NEMOSHOW_SINUSOIDAL_IN_EASE				(nemoeases[NEMOEASE_SINUSOIDAL_IN_TYPE])
#define NEMOSHOW_SINUSOIDAL_OUT_EASE			(nemoeases[NEMOEASE_SINUSOIDAL_OUT_TYPE])
#define NEMOSHOW_SINUSOIDAL_INOUT_EASE		(nemoeases[NEMOEASE_SINUSOIDAL_INOUT_TYPE])
#define NEMOSHOW_EXPONENTIAL_IN_EASE			(nemoeases[NEMOEASE_EXPONENTIAL_IN_TYPE])
#define NEMOSHOW_EXPONENTIAL_OUT_EASE			(nemoeases[NEMOEASE_EXPONENTIAL_OUT_TYPE])
#define NEMOSHOW_EXPONENTIAL_INOUT_EASE		(nemoeases[NEMOEASE_EXPONENTIAL_INOUT_TYPE])
#define NEMOSHOW_CIRCULAR_IN_EASE					(nemoeases[NEMOEASE_CIRCULAR_IN_TYPE])
#define NEMOSHOW_CIRCULAR_OUT_EASE				(nemoeases[NEMOEASE_CIRCULAR_OUT_TYPE])
#define NEMOSHOW_CIRCULAR_INOUT_EASE			(nemoeases[NEMOEASE_CIRCULAR_INOUT_TYPE])

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
