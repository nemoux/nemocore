#ifndef	__NEMO_COMPZ_H__
#define	__NEMO_COMPZ_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

struct nemocompz;
struct nemoevent;

extern struct nemoevent *nemocompz_get_main_event(struct nemocompz *compz);
extern int nemocompz_is_running(struct nemocompz *compz);

extern void nemoevent_write(struct nemoevent *event, uint64_t v);
extern uint64_t nemoevent_read(struct nemoevent *event);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
