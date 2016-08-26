#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemopool.h>
#include <nemohelper.h>
#include <nemolog.h>
#include <nemomisc.h>

static struct poolnode *nemopool_create_node(struct nemopool *pool, uint32_t index);
static void nemopool_destroy_node(struct nemopool *pool, struct poolnode *node);

static void *nemopool_handle_node_thread(void *arg)
{
	struct poolnode *node = (struct poolnode *)arg;
	struct nemopool *pool = node->pool;
	struct pooltask *task;
	uint32_t stime, etime;

	pthread_mutex_lock(&pool->lock);

	while (pool->state != NEMOPOOL_DONE_STATE) {
		if (nemolist_empty(&pool->task_list) == 0) {
			task = nemolist_node0(&pool->task_list, struct pooltask, link);
			nemolist_remove(&task->link);

			pthread_mutex_unlock(&pool->lock);

			stime = time_current_msecs();

			task->dispatch(task->data);

			etime = time_current_msecs();

			pthread_mutex_lock(&pool->lock);

			nemolist_insert_tail(&pool->done_list, &task->link);
			pool->remains--;

			node->dones += 1;
			node->times += (etime - stime);

			pthread_cond_signal(&pool->signal);
		} else {
			pthread_cond_wait(&pool->task_signal, &pool->lock);
		}
	}

	pthread_mutex_unlock(&pool->lock);

	nemopool_destroy_node(pool, node);

	return NULL;
}

static struct poolnode *nemopool_create_node(struct nemopool *pool, uint32_t index)
{
	struct poolnode *node;

	node = (struct poolnode *)malloc(sizeof(struct poolnode));
	if (node == NULL)
		return NULL;
	memset(node, 0, sizeof(struct poolnode));

	node->pool = pool;
	node->index = index;

	pthread_mutex_lock(&pool->lock);

	nemolist_insert_tail(&pool->node_list, &node->link);

	pthread_cond_signal(&pool->signal);

	pthread_mutex_unlock(&pool->lock);

	pthread_create(&node->thread, NULL, nemopool_handle_node_thread, (void *)node);

	return node;
}

static void nemopool_destroy_node(struct nemopool *pool, struct poolnode *node)
{
	pthread_mutex_lock(&pool->lock);

	nemolist_remove(&node->link);

	pthread_cond_signal(&pool->signal);

	pthread_mutex_unlock(&pool->lock);

	free(node);
}

struct nemopool *nemopool_create(int nodes)
{
	struct nemopool *pool;
	int i;

	pool = (struct nemopool *)malloc(sizeof(struct nemopool));
	if (pool == NULL)
		return NULL;
	memset(pool, 0, sizeof(struct nemopool));

	if (pthread_mutex_init(&pool->lock, NULL) != 0)
		goto err1;

	if (pthread_cond_init(&pool->signal, NULL) != 0)
		goto err2;

	if (pthread_cond_init(&pool->task_signal, NULL) != 0)
		goto err3;

	pool->state = NEMOPOOL_NORMAL_STATE;

	nemolist_init(&pool->node_list);
	nemolist_init(&pool->task_list);
	nemolist_init(&pool->done_list);

	for (i = 0; i < nodes; i++) {
		nemopool_create_node(pool, i);
	}

	return pool;

err3:
	pthread_cond_destroy(&pool->signal);

err2:
	pthread_mutex_destroy(&pool->lock);

err1:
	free(pool);

	return NULL;
}

void nemopool_destroy(struct nemopool *pool)
{
	pthread_mutex_lock(&pool->lock);

	pool->state = NEMOPOOL_DONE_STATE;

	pthread_cond_broadcast(&pool->task_signal);

	while (nemolist_empty(&pool->node_list) == 0)
		pthread_cond_wait(&pool->signal, &pool->lock);

	pthread_mutex_unlock(&pool->lock);

	pthread_cond_destroy(&pool->task_signal);
	pthread_cond_destroy(&pool->signal);
	pthread_mutex_destroy(&pool->lock);

	free(pool);
}

int nemopool_dispatch_task(struct nemopool *pool, nemopool_dispatch_t dispatch, void *data)
{
	struct pooltask *task;

	task = (struct pooltask *)malloc(sizeof(struct pooltask));
	task->dispatch = dispatch;
	task->data = data;

	pthread_mutex_lock(&pool->lock);

	nemolist_insert_tail(&pool->task_list, &task->link);

	pool->remains++;

	pthread_cond_broadcast(&pool->task_signal);

	pthread_mutex_unlock(&pool->lock);

	return 0;
}

int nemopool_dispatch_done(struct nemopool *pool, nemopool_dispatch_t dispatch)
{
	struct pooltask *task;
	void *data = NULL;
	uint32_t stime, etime;
	int done;

	stime = time_current_msecs();

	pthread_mutex_lock(&pool->lock);

	while (pool->remains > 0 && nemolist_empty(&pool->done_list) != 0)
		pthread_cond_wait(&pool->signal, &pool->lock);

	if (nemolist_empty(&pool->done_list) == 0) {
		task = nemolist_node0(&pool->done_list, struct pooltask, link);
		nemolist_remove(&task->link);

		data = task->data;

		free(task);
	}

	done = pool->remains <= 0 && nemolist_empty(&pool->done_list) != 0;

	pthread_mutex_unlock(&pool->lock);

	if (data != NULL && dispatch != NULL)
		dispatch(data);

	etime = time_current_msecs();

	pool->times += (etime - stime);

	return done;
}

void nemopool_reset(struct nemopool *pool)
{
	struct poolnode *node;

	pool->times = 0;

	nemolist_for_each(node, &pool->node_list, link) {
		node->dones = 0;
		node->times = 0;
	}
}

void nemopool_dump(struct nemopool *pool)
{
	struct poolnode *node;

	nemolog_message("POOL", "----- total times(%d)\n", pool->times);

	nemolist_for_each(node, &pool->node_list, link) {
		nemolog_message("POOL", "[%02d node] dones(%d) times(%d-%d)\n", node->index, node->dones, node->times, node->dones > 0 ? node->times / node->dones : 0);
	}
}
