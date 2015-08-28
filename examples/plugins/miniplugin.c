#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <pthread.h>

#include <nemocompz.h>
#include <nemomisc.h>

static void miniplugin_dispatch(struct nemocompz *compz, void *data)
{
}

static void *miniplugin_thread(void *arg)
{
	struct nemocompz *compz = (struct nemocompz *)arg;
	struct nemoevent *event = nemocompz_get_main_event(compz);
	struct eventone *one;

	while (nemocompz_is_running(compz)) {
		one = nemoevent_create_one(miniplugin_dispatch, "nemo");
		nemoevent_enqueue_one(event, one);
		nemoevent_trigger(event, 1);

		usleep(100000);
	}

	return NULL;
}

int nemoplugin_init(struct nemocompz *compz)
{
	pthread_t thread;

	pthread_create(&thread, NULL, miniplugin_thread, (void *)compz);

	return 0;
}
