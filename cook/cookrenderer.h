#ifndef __NEMOCOOK_RENDERER_H__
#define __NEMOCOOK_RENDERER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

struct nemocook;

extern int nemocook_renderer_prepare(struct nemocook *cook);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
