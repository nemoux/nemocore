#ifndef __NEMOMSG_H__
#define __NEMOMSG_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <msgqueue.h>
#include <msgconn.h>

struct nemomsg {
	struct msgqueue *queue;
	struct msgconn *conn;

	int soc;
};

extern struct nemomsg *nemomsg_create(const char *ip, int port);
extern void nemomsg_destroy(struct nemomsg *msg);

extern int nemomsg_dispatch_event(void *data);

static inline void nemomsg_set_socket(struct nemomsg *msg, int soc)
{
	msg->soc = soc;
}

static inline int nemomsg_get_socket(struct nemomsg *msg)
{
	return msg->soc;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
