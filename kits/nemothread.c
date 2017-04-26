#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <pthread.h>

#include <nemothread.h>
#include <nemomisc.h>

struct nemothread {
	pthread_t thread;

	int (*dispatch)(void *);
	int value;
	void *data;
};

static void *nemothread_handle_routine(void *arg)
{
	struct nemothread *thread = (struct nemothread *)arg;

	thread->value = thread->dispatch(thread->data);

	return thread;
}

struct nemothread *nemothread_create(int (*dispatch)(void *), void *data)
{
	struct nemothread *thread;

	thread = (struct nemothread *)malloc(sizeof(struct nemothread));
	if (thread == NULL)
		return NULL;
	memset(thread, 0, sizeof(struct nemothread));

	thread->dispatch = dispatch;
	thread->data = data;

	pthread_create(&thread->thread, NULL, nemothread_handle_routine, (void *)thread);

	return thread;
}

void nemothread_destroy(struct nemothread *thread)
{
	free(thread);
}

int nemothread_join(struct nemothread *thread)
{
	if (pthread_join(thread->thread, NULL) < 0)
		return errno;

	return thread->value;
}
