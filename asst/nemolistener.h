#ifndef	__NEMO_LISTENER_H__
#define	__NEMO_LISTENER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemolist.h>

struct nemolistener;

typedef void (*nemosignal_notify_t)(struct nemolistener *listener, void *data);

struct nemolistener {
	struct nemolist link;
	nemosignal_notify_t notify;
};

struct nemosignal {
	struct nemolist listener_list;
};

static inline void nemosignal_init(struct nemosignal *signal)
{
	nemolist_init(&signal->listener_list);
}

static inline void nemosignal_add(struct nemosignal *signal, struct nemolistener *listener)
{
	nemolist_insert(signal->listener_list.prev, &listener->link);
}

static inline struct nemolistener *nemosignal_get(struct nemosignal *signal, nemosignal_notify_t notify)
{
	struct nemolistener *listener;

	nemolist_for_each(listener, &signal->listener_list, link) {
		if (listener->notify == notify)
			return listener;
	}

	return NULL;
}

static inline void nemosignal_emit(struct nemosignal *signal, void *data)
{
	struct nemolistener *listener, *next;

	nemolist_for_each_safe(listener, next, &signal->listener_list, link) {
		listener->notify(listener, data);
	}
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
