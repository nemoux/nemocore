#ifndef	__NEMOSHELL_ENVS_H__
#define	__NEMOSHELL_ENVS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct nemoshell;

extern void nemoenvs_load_background(struct nemoshell *shell);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
