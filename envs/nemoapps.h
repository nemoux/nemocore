#ifndef __NEMOSHELL_APPS_H__
#define __NEMOSHELL_APPS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemolist.h>

struct nemoenvs;

struct nemoapp {
	pid_t pid;

	char *id;

	struct nemolist link;
};

struct nemoclient {
	pid_t pid;

	struct nemolist link;
};

extern struct nemoapp *nemoenvs_create_app(void);
extern void nemoenvs_destroy_app(struct nemoapp *app);

extern int nemoenvs_attach_app(struct nemoenvs *envs, const char *id, pid_t pid);
extern void nemoenvs_detach_app(struct nemoenvs *envs, pid_t pid);

extern int nemoenvs_respawn_app(struct nemoenvs *envs, pid_t pid);

extern void nemoenvs_execute_backgrounds(struct nemoenvs *envs);
extern void nemoenvs_execute_daemons(struct nemoenvs *envs);

extern int nemoenvs_attach_client(struct nemoenvs *envs, pid_t pid);
extern void nemoenvs_detach_client(struct nemoenvs *envs, pid_t pid);

extern int nemoenvs_terminate_client(struct nemoenvs *envs, pid_t pid);
extern int nemoenvs_terminate_clients(struct nemoenvs *envs);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
