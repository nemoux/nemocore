#ifndef __NEMOMSG_CONN_H__
#define __NEMOMSG_CONN_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemolist.h>

struct msgclient {
	char *name;

	char *ip;
	int port;

	struct nemolist link;
};

struct msgconn {
	struct nemolist client_list;

	int soc;
};

extern struct msgconn *nemomsg_conn_create(void);
extern void nemomsg_conn_destroy(struct msgconn *conn);

extern int nemomsg_conn_set_client(struct msgconn *conn, const char *name, const char *ip, int port);
extern int nemomsg_conn_put_client(struct msgconn *conn, const char *name, const char *ip, int port);

extern int nemomsg_conn_send_msg(struct msgconn *conn, const char *name, const char *msg, int size);

static inline void nemomsg_conn_set_socket(struct msgconn *conn, int soc)
{
	conn->soc = soc;
}

static inline int nemomsg_conn_get_socket(struct msgconn *conn)
{
	return conn->soc;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
