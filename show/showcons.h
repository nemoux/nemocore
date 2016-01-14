#ifndef __NEMOSHOW_CONS_H__
#define __NEMOSHOW_CONS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <showone.h>
#include <showexpr.h>

struct showcons {
	struct showone base;

	char name[NEMOSHOW_EXPR_SYMBOL_NAME_MAX];
	double value;
};

#define NEMOSHOW_CONS(one)					((struct showcons *)container_of(one, struct showcons, base))
#define	NEMOSHOW_CONS_AT(one, at)		(NEMOSHOW_CONS(one)->at)

extern struct showone *nemoshow_cons_create(void);
extern void nemoshow_cons_destroy(struct showone *one);

extern int nemoshow_cons_arrange(struct showone *one);
extern int nemoshow_cons_update(struct showone *one);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
