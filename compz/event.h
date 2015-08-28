#ifndef	__NEMO_EVENT_H__
#define	__NEMO_EVENT_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <pthread.h>

#include <nemolist.h>
#include <nemolistener.h>

struct nemocompz;

typedef void (*nemoevent_dispatch_t)(struct nemocompz *compz, void *data);

struct eventone {
	nemoevent_dispatch_t dispatch;
	void *data;

	struct nemolist link;
};

struct nemoevent {
	struct nemocompz *compz;

	struct wl_event_source *source;
	int eventfd;

	struct nemolist event_list;
	pthread_mutex_t lock;
};

extern struct nemoevent *nemoevent_create(struct nemocompz *compz);
extern void nemoevent_destroy(struct nemoevent *event);

extern void nemoevent_trigger(struct nemoevent *event, uint64_t v);

extern struct eventone *nemoevent_create_one(nemoevent_dispatch_t dispatch, void *data);
extern void nemoevent_destroy_one(struct eventone *one);

extern void nemoevent_enqueue_one(struct nemoevent *event, struct eventone *one);
extern struct eventone *nemoevent_dequeue_one(struct nemoevent *event);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
