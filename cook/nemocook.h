#ifndef __NEMOCOOK_H__
#define __NEMOCOOK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <cookshader.h>

struct nemocook {
};

extern struct nemocook *nemocook_create(void);
extern void nemocook_destroy(struct nemocook *cook);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
