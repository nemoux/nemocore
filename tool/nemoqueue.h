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
	uint32_t type;
	uint32_t tag;

	float *attrs;
	int nattrs;

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

extern struct eventone *nemoqueue_one_create(uint32_t type, uint32_t tag, int size);
extern void nemoqueue_one_destroy(struct eventone *one);

static inline void nemoqueue_one_set(struct eventone *one, int index, float attr)
{
	one->attrs[index] = attr;
}

static inline float nemoqueue_one_get(struct eventone *one, int index)
{
	return one->attrs[index];
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
