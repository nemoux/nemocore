#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <pthread.h>

#include <nemocompz.h>
#include <nemomisc.h>

static void miniplugin_dispatch_touch(struct nemocompz *compz, void *context, void *data)
{
	struct touchnode *node = (struct touchnode *)context;
	struct touchtaps *taps = (struct touchtaps *)data;

	nemotouch_flush_taps(node, taps);

	nemotouch_destroy_taps(taps);
}

static void *miniplugin_dispatch_thread(void *arg)
{
	struct nemocompz *compz = (struct nemocompz *)arg;
	struct nemoevent *event = nemocompz_get_main_event(compz);
	struct eventone *one;
	struct touchnode *node;
	struct touchtaps *taps;

	node = nemotouch_create_node(compz, "vtouch");

	while (nemocompz_is_running(compz)) {
		taps = nemotouch_create_taps(64);
		nemotouch_attach_tap(taps, 1, 0.1f, 0.1f);
		nemotouch_attach_tap(taps, 2, 0.5f, 0.5f);
		nemotouch_attach_tap(taps, 3, 0.9f, 0.9f);

		one = nemoevent_create_one(miniplugin_dispatch_touch, node, taps);
		nemoevent_enqueue_one(event, one);
		nemoevent_trigger(event, 1);

		usleep(100000);
	}

	nemotouch_destroy_node(node);

	return NULL;
}

int nemoplugin_init(struct nemocompz *compz)
{
	pthread_t thread;

	pthread_create(&thread, NULL, miniplugin_dispatch_thread, (void *)compz);

	return 0;
}
