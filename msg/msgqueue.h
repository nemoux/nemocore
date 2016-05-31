#ifndef	__NEMOMSG_QUEUE_H__
#define	__NEMOMSG_QUEUE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemolist.h>
#include <nemotoken.h>

typedef int (*nemomsg_callback_t)(void *data, const char *name, struct nemotoken *msg);

struct msgcallback {
	nemomsg_callback_t callback;
	char *name;

	struct nemolist link;
	struct nemolist dlink;
};

struct msgqueue {
	struct nemolist callback_list;

	struct nemolist source_list;
	struct nemolist destination_list;
	struct nemolist command_list;

	struct nemolist delete_list;

	void *data;
};

extern struct msgqueue *nemomsg_queue_create(void);
extern void nemomsg_queue_destroy(struct msgqueue *queue);

extern int nemomsg_queue_set_callback(struct msgqueue *queue, nemomsg_callback_t callback);
extern int nemomsg_queue_set_source_callback(struct msgqueue *queue, const char *name, nemomsg_callback_t callback);
extern int nemomsg_queue_set_destination_callback(struct msgqueue *queue, const char *name, nemomsg_callback_t callback);
extern int nemomsg_queue_set_command_callback(struct msgqueue *queue, const char *name, nemomsg_callback_t callback);

extern int nemomsg_queue_put_callback(struct msgqueue *queue, nemomsg_callback_t callback);
extern int nemomsg_queue_put_source_callback(struct msgqueue *queue, const char *name, nemomsg_callback_t callback);
extern int nemomsg_queue_put_destination_callback(struct msgqueue *queue, const char *name, nemomsg_callback_t callback);
extern int nemomsg_queue_put_command_callback(struct msgqueue *queue, const char *name, nemomsg_callback_t callback);

extern int nemomsg_queue_dispatch(struct msgqueue *queue, struct nemotoken *msg);
extern int nemomsg_queue_clean(struct msgqueue *queue);

static inline void nemomsg_queue_set_data(struct msgqueue *queue, void *data)
{
	queue->data = data;
}

static inline void *nemomsg_queue_get_data(struct msgqueue *queue)
{
	return queue->data;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
