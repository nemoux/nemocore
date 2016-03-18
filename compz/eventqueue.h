#ifndef	__NEMO_EVENTQUEUE_H__
#define	__NEMO_EVENTQUEUE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <pthread.h>

#include <nemolist.h>
#include <nemolistener.h>

struct nemocompz;

typedef void (*nemoeventqueue_dispatch_t)(struct nemocompz *compz, void *context, void *data);

struct eventone {
	nemoeventqueue_dispatch_t dispatch;
	void *context;
	void *data;

	struct nemolist link;
};

struct nemoeventqueue {
	struct nemocompz *compz;

	struct wl_event_source *source;
	int eventfd;

	struct nemolist event_list;
	pthread_mutex_t lock;
};

extern struct nemoeventqueue *nemoeventqueue_create(struct nemocompz *compz);
extern void nemoeventqueue_destroy(struct nemoeventqueue *queue);

extern void nemoeventqueue_trigger(struct nemoeventqueue *queue, uint64_t v);

extern struct eventone *nemoeventqueue_create_one(nemoeventqueue_dispatch_t dispatch, void *context, void *data);
extern void nemoeventqueue_destroy_one(struct eventone *one);

extern void nemoeventqueue_enqueue_one(struct nemoeventqueue *queue, struct eventone *one);
extern struct eventone *nemoeventqueue_dequeue_one(struct nemoeventqueue *queue);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
