#ifndef	__NEMO_COMPZ_H__
#define	__NEMO_COMPZ_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

struct nemocompz;
struct nemoevent;

typedef void (*nemoevent_dispatch_t)(struct nemocompz *compz, void *data);

extern struct nemoevent *nemocompz_get_main_event(struct nemocompz *compz);
extern int nemocompz_is_running(struct nemocompz *compz);

extern void nemoevent_trigger(struct nemoevent *event, uint64_t v);

extern struct eventone *nemoevent_create_one(nemoevent_dispatch_t dispatch, void *data);
extern void nemoevent_destroy_one(struct eventone *one);

extern void nemoevent_enqueue_one(struct nemoevent *event, struct eventone *one);
extern struct eventone *nemoevent_dequeue_one(struct nemoevent *event);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
