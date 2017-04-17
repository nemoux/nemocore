#ifndef __NEMOSHELL_APPS_H__
#define __NEMOSHELL_APPS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemolist.h>

struct nemoenvs;
struct nemotimer;

struct nemoapp {
	struct nemoenvs *envs;

	pid_t pid;

	char *id;

	struct nemotimer *timer;

	struct nemolist link;
};

struct nemoclient {
	char *name;

	pid_t pid;

	uint32_t stime;

	struct nemolist link;
};

extern struct nemoapp *nemoenvs_create_app(void);
extern void nemoenvs_destroy_app(struct nemoapp *app);

extern int nemoenvs_attach_app(struct nemoenvs *envs, const char *id, pid_t pid);
extern void nemoenvs_detach_app(struct nemoenvs *envs, pid_t pid);

extern void nemoenvs_alive_app(struct nemoenvs *envs, pid_t pid, uint32_t timeout);

extern int nemoenvs_respawn_app(struct nemoenvs *envs, pid_t pid);

extern void nemoenvs_execute_backgrounds(struct nemoenvs *envs);
extern void nemoenvs_execute_daemons(struct nemoenvs *envs);
extern void nemoenvs_execute_screensavers(struct nemoenvs *envs);

extern int nemoenvs_attach_client(struct nemoenvs *envs, pid_t pid, const char *name);
extern int nemoenvs_detach_client(struct nemoenvs *envs, pid_t pid);

extern int nemoenvs_terminate_client(struct nemoenvs *envs, pid_t pid);
extern int nemoenvs_terminate_clients(struct nemoenvs *envs);

extern int nemoenvs_get_client_count(struct nemoenvs *envs);

extern int nemoenvs_launch_app(struct nemoenvs *envs, const char *_path, const char *_args, const char *_states);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
