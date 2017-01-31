#ifndef	__NEMOMOTZ_H__
#define	__NEMOMOTZ_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

struct nemomotz {
};

extern struct nemomotz *nemomotz_create(void);
extern void nemomotz_destroy(struct nemomotz *motz);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
