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

	int32_t begin, end;
};

#define	NEMOSHOW_LOOP(one)		((struct showloop *)container_of(one, struct showloop, base))

extern struct showone *nemoshow_loop_create(void);
extern void nemoshow_loop_destroy(struct showone *one);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
