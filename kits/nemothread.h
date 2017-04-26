#ifndef __NEMO_THREAD_H__
#define	__NEMO_THREAD_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

struct nemothread;

extern struct nemothread *nemothread_create(int (*dispatch)(void *), void *data);
extern void nemothread_destroy(struct nemothread *thread);

extern int nemothread_join(struct nemothread *thread);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
