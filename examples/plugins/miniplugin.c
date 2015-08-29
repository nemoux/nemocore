#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <pthread.h>

#include <nemocompz.h>
#include <nemomisc.h>

struct miniplugin {
	struct nemocompz *compz;

	char *devnode;
};

static void miniplugin_dispatch_touch(struct nemocompz *compz, void *context, void *data)
{
	struct touchnode *node = (struct touchnode *)context;
	struct touchtaps *taps = (struct touchtaps *)data;

	nemotouch_flush_taps(node, taps);

	nemotouch_destroy_taps(taps);
}

static void *miniplugin_dispatch_thread(void *arg)
{
	struct miniplugin *mini = (struct miniplugin *)arg;
	struct nemocompz *compz = mini->compz;
	struct nemoevent *event = nemocompz_get_main_event(compz);
	struct eventone *one;
	struct touchnode *node;
	struct touchtaps *taps;

	node = nemotouch_create_node(compz, mini->devnode);

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

	free(mini->devnode);
	free(mini);

	return NULL;
}

int nemoplugin_init(struct nemocompz *compz, const char *args)
{
	struct miniplugin *mini;
	pthread_t thread;

	mini = (struct miniplugin *)malloc(sizeof(struct miniplugin));
	if (mini == NULL)
		return -1;

	mini->compz = compz;
	mini->devnode = strdup(args);

	pthread_create(&thread, NULL, miniplugin_dispatch_thread, (void *)mini);

	return 0;
}
