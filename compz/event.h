#ifndef	__NEMO_EVENT_H__
#define	__NEMO_EVENT_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct nemocompz;

struct nemoevent {
	struct nemocompz *compz;

	struct wl_event_source *source;
	int eventfd;
};

extern struct nemoevent *nemoevent_create(struct nemocompz *compz);
extern void nemoevent_destroy(struct nemoevent *event);

extern void nemoevent_write(struct nemoevent *event, uint64_t v);
extern uint64_t nemoevent_read(struct nemoevent *event);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
