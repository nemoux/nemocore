#ifndef	__NEMOSHOW_LOOP_H__
#define	__NEMOSHOW_LOOP_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <showone.h>

struct showloop {
	struct showone base;

	struct showone *canvas;

	int32_t begin, end;
};

#define	NEMOSHOW_LOOP(one)					((struct showloop *)container_of(one, struct showloop, base))
#define	NEMOSHOW_LOOP_AT(one, at)		(NEMOSHOW_LOOP(one)->at)

extern struct showone *nemoshow_loop_create(void);
extern void nemoshow_loop_destroy(struct showone *one);

extern int nemoshow_loop_arrange(struct nemoshow *show, struct showone *one);
extern int nemoshow_loop_update(struct nemoshow *show, struct showone *one);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
