#ifndef __NEMOSHELL_BEAT_H__
#define __NEMOSHELL_BEAT_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct nemoenvs;

extern int nemoenvs_check_beats(struct nemoenvs *envs, int port, int timeout);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
