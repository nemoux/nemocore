#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <pthread.h>

#include <nemocompz.h>
#include <nemomisc.h>

static void *miniplugin_thread(void *arg)
{
	struct nemocompz *compz = (struct nemocompz *)arg;
	struct nemoevent *event = nemocompz_get_main_event(compz);

	while (nemocompz_is_running(compz)) {
		nemoevent_write(event, 777);

		sleep(1);
	}

	return NULL;
}

int nemoplugin_init(struct nemocompz *compz)
{
	pthread_t thread;

	pthread_create(&thread, NULL, miniplugin_thread, (void *)compz);

	return 0;
}
