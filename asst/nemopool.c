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

static struct poolnode *nemopool_create_node(struct nemopool *pool);
static void nemopool_destroy_node(struct nemopool *pool, struct poolnode *node);

static void *nemopool_handle_node_thread(void *arg)
{
	struct poolnode *node = (struct poolnode *)arg;
	struct nemopool *pool = node->pool;
	struct pooltask *task;

	pthread_mutex_lock(&pool->lock);

	while (pool->state != NEMOPOOL_DONE_STATE) {
		if (nemolist_empty(&pool->task_list) == 0) {
			task = nemolist_node0(&pool->task_list, struct pooltask, link);
			nemolist_remove(&task->link);

			pthread_mutex_unlock(&pool->lock);

			task->dispatch(task->data);
			free(task);

			pthread_mutex_lock(&pool->lock);

			pool->task_remains--;

			pthread_cond_signal(&pool->signal);
		} else {
			pthread_cond_wait(&pool->task_signal, &pool->lock);
		}
	}

	pthread_mutex_unlock(&pool->lock);

	nemopool_destroy_node(pool, node);

	return NULL;
}

static struct poolnode *nemopool_create_node(struct nemopool *pool)
{
	struct poolnode *node;

	node = (struct poolnode *)malloc(sizeof(struct poolnode));
	if (node == NULL)
		return NULL;
	memset(node, 0, sizeof(struct poolnode));

	node->pool = pool;

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

	for (i = 0; i < nodes; i++) {
		nemopool_create_node(pool);
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

	pool->task_remains++;

	pthread_cond_broadcast(&pool->task_signal);

	pthread_mutex_unlock(&pool->lock);

	return 0;
}

int nemopool_finish_task(struct nemopool *pool)
{
	pthread_mutex_lock(&pool->lock);

	while (pool->task_remains > 0)
		pthread_cond_wait(&pool->signal, &pool->lock);

	pthread_mutex_unlock(&pool->lock);

	return 0;
}
