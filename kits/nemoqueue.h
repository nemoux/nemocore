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

extern struct eventone *nemoqueue_one_create(void);
extern void nemoqueue_one_destroy(struct eventone *one);

extern void nemoqueue_one_set_max_integer(struct eventone *one, int count);
extern void nemoqueue_one_set_max_float(struct eventone *one, int count);
extern void nemoqueue_one_set_max_string(struct eventone *one, int count);
extern void nemoqueue_one_set_max_pointer(struct eventone *one, int count);

extern int nemoqueue_one_get_max_integer(struct eventone *one);
extern int nemoqueue_one_get_max_float(struct eventone *one);
extern int nemoqueue_one_get_max_string(struct eventone *one);
extern int nemoqueue_one_get_max_pointer(struct eventone *one);

extern void nemoqueue_one_set_integer(struct eventone *one, int index, int attr);
extern int nemoqueue_one_get_integer(struct eventone *one, int index);
extern void nemoqueue_one_set_float(struct eventone *one, int index, float attr);
extern float nemoqueue_one_get_float(struct eventone *one, int index);
extern void nemoqueue_one_set_string(struct eventone *one, int index, const char *attr);
extern const char *nemoqueue_one_get_string(struct eventone *one, int index);
extern void nemoqueue_one_set_pointer(struct eventone *one, int index, void *attr);
extern void *nemoqueue_one_get_pointer(struct eventone *one, int index);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
