#ifndef __NEMO_QUEUE_H__
#define	__NEMO_QUEUE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

struct nemoqueue;
struct eventone;

extern struct nemoqueue *nemoqueue_create(void);
extern void nemoqueue_destroy(struct nemoqueue *queue);

extern void nemoqueue_enqueue_one(struct nemoqueue *queue, struct eventone *one);
extern void nemoqueue_enqueue_one_tail(struct nemoqueue *queue, struct eventone *one);
extern struct eventone *nemoqueue_dequeue_one(struct nemoqueue *queue);

extern struct eventone *nemoqueue_one_create(int isize, int fsize);
extern void nemoqueue_one_destroy(struct eventone *one);

extern int nemoqueue_one_get_icount(struct eventone *one);
extern int nemoqueue_one_get_fcount(struct eventone *one);
extern void nemoqueue_one_seti(struct eventone *one, int index, int attr);
extern int nemoqueue_one_geti(struct eventone *one, int index);
extern void nemoqueue_one_setf(struct eventone *one, int index, float attr);
extern float nemoqueue_one_getf(struct eventone *one, int index);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
