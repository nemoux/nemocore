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

	void *(*dispatch)(void *);
	void *data;
};

struct nemothread *nemothread_create(void *(*dispatch)(void *), void *data)
{
	struct nemothread *thread;

	thread = (struct nemothread *)malloc(sizeof(struct nemothread));
	if (thread == NULL)
		return NULL;
	memset(thread, 0, sizeof(struct nemothread));

	thread->dispatch = dispatch;
	thread->data = data;

	pthread_create(&thread->thread, NULL, dispatch, (void *)thread);

	return thread;
}

void nemothread_destroy(struct nemothread *thread)
{
	free(thread);
}

void *nemothread_join(struct nemothread *thread)
{
	void *r;

	if (pthread_join(thread->thread, &r) < 0)
		return NULL;

	return r;
}
