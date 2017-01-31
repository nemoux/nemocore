#ifndef	__NEMOTOYZ_H__
#define	__NEMOTOYZ_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

struct nemotoyz {
};

extern struct nemotoyz *nemotoyz_create(void);
extern void nemotoyz_destroy(struct nemotoyz *toyz);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
