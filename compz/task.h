#ifndef	__NEMO_TASK_H__
#define	__NEMO_TASK_H__

struct nemocompz;
struct nemotask;

typedef void (*nemotask_cleanup_t)(struct nemotask *task, int status);

struct nemotask {
	pid_t pid;
	nemotask_cleanup_t cleanup;
	struct wl_list link;
};

extern struct wl_client *nemotask_launch(struct nemocompz *compz, struct nemotask *task, const char *path, nemotask_cleanup_t cleanup);
extern void nemotask_watch(struct nemocompz *compz, struct nemotask *task);

#endif
