#ifndef __NEMOTOOL_QUEUE_H__
#define	__NEMOTOOL_QUEUE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <pthread.h>

#include <nemolist.h>

struct eventone {
	int *iattrs;
	int niattrs;

	float *fattrs;
	int nfattrs;

	struct nemolist link;
};

struct nemoqueue {
	struct nemolist list;
	pthread_mutex_t lock;
};

extern struct nemoqueue *nemoqueue_create(void);
extern void nemoqueue_destroy(struct nemoqueue *queue);

extern void nemoqueue_enqueue_one(struct nemoqueue *queue, struct eventone *one);
extern struct eventone *nemoqueue_dequeue_one(struct nemoqueue *queue);

extern struct eventone *nemoqueue_one_create(int isize, int fsize);
extern void nemoqueue_one_destroy(struct eventone *one);

static inline void nemoqueue_one_seti(struct eventone *one, int index, int attr)
{
	one->iattrs[index] = attr;
}

static inline int nemoqueue_one_geti(struct eventone *one, int index)
{
	return one->iattrs[index];
}

static inline void nemoqueue_one_setf(struct eventone *one, int index, float attr)
{
	one->fattrs[index] = attr;
}

static inline float nemoqueue_one_getf(struct eventone *one, int index)
{
	return one->fattrs[index];
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
