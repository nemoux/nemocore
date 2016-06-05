#ifndef __NEMO_MSG_H__
#define __NEMO_MSG_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemolist.h>
#include <nemotoken.h>

typedef int (*nemomsg_callback_t)(void *data, const char *src, const char *dst, struct nemotoken *content);

struct msgcallback {
	nemomsg_callback_t callback;
	void *data;

	char *name;

	struct nemolist link;
	struct nemolist dlink;
};

struct msgclient {
	char *name;

	char *ip;
	int port;

	int liveness;

	struct nemolist link;
};

struct nemomsg {
	int soc;

	struct nemolist client_list;

	struct nemolist callback_list;

	struct nemolist source_list;
	struct nemolist destination_list;

	struct nemolist delete_list;

	struct {
		char ip[128];
		int port;
	} source;

	void *data;
};

extern struct nemomsg *nemomsg_create(const char *ip, int port);
extern void nemomsg_destroy(struct nemomsg *msg);

extern int nemomsg_set_callback(struct nemomsg *msg, nemomsg_callback_t callback, void *data);
extern int nemomsg_set_source_callback(struct nemomsg *msg, const char *name, nemomsg_callback_t callback, void *data);
extern int nemomsg_set_destination_callback(struct nemomsg *msg, const char *name, nemomsg_callback_t callback, void *data);

extern int nemomsg_put_callback(struct nemomsg *msg, nemomsg_callback_t callback);
extern int nemomsg_put_source_callback(struct nemomsg *msg, const char *name, nemomsg_callback_t callback);
extern int nemomsg_put_destination_callback(struct nemomsg *msg, const char *name, nemomsg_callback_t callback);

extern int nemomsg_dispatch(struct nemomsg *msg, const char *ip, int port, struct nemotoken *content);
extern int nemomsg_clean(struct nemomsg *msg);

extern int nemomsg_set_client(struct nemomsg *msg, const char *name, const char *ip, int port);
extern int nemomsg_put_client(struct nemomsg *msg, const char *name, const char *ip, int port);

extern int nemomsg_check_clients(struct nemomsg *msg);
extern int nemomsg_clean_clients(struct nemomsg *msg);

extern int nemomsg_recv_message(struct nemomsg *msg, char *content, int size);
extern int nemomsg_send_message(struct nemomsg *msg, const char *name, const char *contents, int size);
extern int nemomsg_send_format(struct nemomsg *msg, const char *name, const char *fmt, ...);
extern int nemomsg_send_vargs(struct nemomsg *msg, const char *name, const char *fmt, va_list vargs);

static inline void nemomsg_set_data(struct nemomsg *msg, void *data)
{
	msg->data = data;
}

static inline void *nemomsg_get_data(struct nemomsg *msg)
{
	return msg->data;
}

static inline void nemomsg_set_socket(struct nemomsg *msg, int soc)
{
	msg->soc = soc;
}

static inline int nemomsg_get_socket(struct nemomsg *msg)
{
	return msg->soc;
}

static inline const char *nemomsg_get_source_ip(struct nemomsg *msg)
{
	return msg->source.ip;
}

static inline int nemomsg_get_source_port(struct nemomsg *msg)
{
	return msg->source.port;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
