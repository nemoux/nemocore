#ifndef __NEMOPLAY_QUEUE_H__
#define __NEMOPLAY_QUEUE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <pthread.h>

#include <nemolist.h>

typedef enum {
	NEMOPLAY_QUEUE_NORMAL_COMMAND = 0,
	NEMOPLAY_QUEUE_FLUSH_COMMAND = 1,
	NEMOPLAY_QUEUE_ERROR_COMMAND = 2,
	NEMOPLAY_QUEUE_DONE_COMMAND = 3,
	NEMOPLAY_QUEUE_LAST_COMMAND
} NemoPlayQueueCmd;

struct playone {
	struct nemolist link;

	int cmd;

	double pts;

	void *data;
	uint32_t size;

	uint8_t *y;
	uint8_t *u;
	uint8_t *v;
};

struct playqueue {
	pthread_mutex_t lock;
	pthread_cond_t signal;

	struct nemolist list;
	int count;
};

extern struct playqueue *nemoplay_queue_create(void);
extern void nemoplay_queue_destroy(struct playqueue *queue);

extern struct playone *nemoplay_queue_create_one(void);
extern void nemoplay_queue_destroy_one(struct playone *one);

extern void nemoplay_queue_enqueue(struct playqueue *queue, struct playone *one);
extern void nemoplay_queue_enqueue_tail(struct playqueue *queue, struct playone *one);
extern struct playone *nemoplay_queue_dequeue(struct playqueue *queue);
extern struct playone *nemoplay_queue_peek(struct playqueue *queue);
extern void nemoplay_queue_wait(struct playqueue *queue);

static inline int nemoplay_queue_get_count(struct playqueue *queue)
{
	return queue->count;
}

static inline int nemoplay_queue_get_one_cmd(struct playone *one)
{
	return one->cmd;
}

static inline double nemoplay_queue_get_one_pts(struct playone *one)
{
	return one->pts;
}

static inline void *nemoplay_queue_get_one_data(struct playone *one)
{
	return one->data;
}

static inline uint8_t *nemoplay_queue_get_one_y(struct playone *one)
{
	return one->y;
}

static inline uint8_t *nemoplay_queue_get_one_u(struct playone *one)
{
	return one->u;
}

static inline uint8_t *nemoplay_queue_get_one_v(struct playone *one)
{
	return one->v;
}

static inline uint32_t nemoplay_queue_get_one_size(struct playone *one)
{
	return one->size;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
