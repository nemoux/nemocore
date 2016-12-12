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
	NEMOPLAY_QUEUE_NORMAL_STATE = 0,
	NEMOPLAY_QUEUE_STOP_STATE = 1,
	NEMOPLAY_QUEUE_DONE_STATE = 2,
	NEMOPLAY_QUEUE_LAST_STATE
} NemoPlayQueueState;

typedef enum {
	NEMOPLAY_QUEUE_NORMAL_COMMAND = 0,
	NEMOPLAY_QUEUE_ERROR_COMMAND = 2,
	NEMOPLAY_QUEUE_LAST_COMMAND
} NemoPlayQueueCmd;

struct playone {
	struct nemolist link;

	int cmd;

	double pts;

	void *data[4];
	int linesize[4];
	int height;

	uint32_t serial;
};

struct playqueue {
	int state;

	pthread_mutex_t lock;
	pthread_cond_t signal;

	struct nemolist list;
	int count;

	uint32_t serial;
};

extern struct playqueue *nemoplay_queue_create(void);
extern void nemoplay_queue_destroy(struct playqueue *queue);

extern void nemoplay_queue_enqueue(struct playqueue *queue, struct playone *one);
extern void nemoplay_queue_enqueue_tail(struct playqueue *queue, struct playone *one);
extern struct playone *nemoplay_queue_dequeue(struct playqueue *queue);
extern struct playone *nemoplay_queue_peek(struct playqueue *queue);
extern int nemoplay_queue_peek_pts(struct playqueue *queue, double *pts);
extern void nemoplay_queue_wait(struct playqueue *queue);

extern void nemoplay_queue_flush(struct playqueue *queue);

extern void nemoplay_queue_set_state(struct playqueue *queue, int state);

static inline int nemoplay_queue_get_state(struct playqueue *queue)
{
	return queue->state;
}

static inline int nemoplay_queue_get_count(struct playqueue *queue)
{
	return queue->count;
}

static inline uint32_t nemoplay_queue_get_serial(struct playqueue *queue)
{
	return queue->serial;
}

extern struct playone *nemoplay_one_create(void);
extern void nemoplay_one_destroy(struct playone *one);

static inline int nemoplay_one_get_cmd(struct playone *one)
{
	return one->cmd;
}

static inline double nemoplay_one_get_pts(struct playone *one)
{
	return one->pts;
}

static inline uint8_t *nemoplay_one_get_y(struct playone *one)
{
	return (uint8_t *)one->data[0];
}

static inline uint8_t *nemoplay_one_get_u(struct playone *one)
{
	return (uint8_t *)one->data[1];
}

static inline uint8_t *nemoplay_one_get_v(struct playone *one)
{
	return (uint8_t *)one->data[2];
}

static inline void *nemoplay_one_get_data(struct playone *one, int index)
{
	return one->data[index];
}

static inline uint32_t nemoplay_one_get_linesize(struct playone *one, int index)
{
	return one->linesize[index];
}

static inline int nemoplay_one_get_height(struct playone *one)
{
	return one->height;
}

static inline uint32_t nemoplay_one_get_serial(struct playone *one)
{
	return one->serial;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
