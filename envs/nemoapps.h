#ifndef __NEMOSHELL_APPS_H__
#define __NEMOSHELL_APPS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemolist.h>

struct nemoenvs;
struct nemotimer;

struct nemoservice {
	struct nemoenvs *envs;

	pid_t pid;

	char *group;
	char *path;
	char *args;
	char *envp;
	char *states;

	struct nemotimer *timer;

	struct nemolist link;
};

struct nemoclient {
	char *name;

	pid_t pid;

	uint32_t stime;

	struct nemolist link;
};

extern struct nemoservice *nemoenvs_attach_service(struct nemoenvs *envs, const char *group, const char *path, const char *args, const char *envp, const char *states);
extern void nemoenvs_detach_service(struct nemoservice *service);

extern int nemoenvs_alive_service(struct nemoenvs *envs, pid_t pid, uint32_t timeout);
extern int nemoenvs_respawn_service(struct nemoenvs *envs, pid_t pid);

extern void nemoenvs_start_services(struct nemoenvs *envs, const char *group);
extern void nemoenvs_stop_services(struct nemoenvs *envs, const char *group);

extern int nemoenvs_attach_client(struct nemoenvs *envs, pid_t pid, const char *name);
extern int nemoenvs_detach_client(struct nemoenvs *envs, pid_t pid);

extern int nemoenvs_terminate_client(struct nemoenvs *envs, pid_t pid);
extern int nemoenvs_terminate_clients(struct nemoenvs *envs);

extern int nemoenvs_get_client_count(struct nemoenvs *envs);

extern int nemoenvs_launch_service(struct nemoenvs *envs, const char *_path, const char *_args, const char *_envp, const char *_states);
extern int nemoenvs_launch_app(struct nemoenvs *envs, const char *_path, const char *_args, const char *_envp, const char *_states);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
