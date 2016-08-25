#ifndef __NEMO_POOL_H__
#define __NEMO_POOL_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>
#include <pthread.h>

#include <nemolist.h>

typedef void (*nemopool_dispatch_t)(void *data);

typedef enum {
	NEMOPOOL_NORMAL_STATE = 0,
	NEMOPOOL_DONE_STATE = 1,
	NEMOPOOL_LAST_STATE
} NemoPoolState;

struct nemopool {
	int state;

	pthread_mutex_t lock;
	pthread_cond_t signal;

	struct nemolist node_list;

	pthread_cond_t task_signal;

	struct nemolist task_list;
	struct nemolist done_list;
	uint32_t remains;
	uint32_t times;
};

struct pooltask {
	struct nemolist link;

	nemopool_dispatch_t dispatch;
	void *data;
};

struct poolnode {
	struct nemopool *pool;

	uint32_t index;
	uint32_t dones;
	uint32_t times;

	pthread_t thread;

	struct nemolist link;
};

extern struct nemopool *nemopool_create(int nodes);
extern void nemopool_destroy(struct nemopool *pool);

extern int nemopool_dispatch_task(struct nemopool *pool, nemopool_dispatch_t dispatch, void *data);
extern int nemopool_dispatch_done(struct nemopool *pool, nemopool_dispatch_t dispatch);

extern void nemopool_reset(struct nemopool *pool);
extern void nemopool_dump(struct nemopool *pool);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
