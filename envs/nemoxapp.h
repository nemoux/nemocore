#ifndef __NEMOSHELL_XAPP_H__
#define __NEMOSHELL_XAPP_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct nemoenvs;

struct nemoxapp {
	int xdisplay;
	char *rendernode;

	char *path;
	char *args;
	char *envp;
	char *states;

	struct wl_list link;
};

struct nemoxclient {
	struct nemoxserver *xserver;

	char *name;
	pid_t pid;

	uint32_t stime;

	struct wl_list link;
};

extern int nemoenvs_launch_xserver(struct nemoenvs *envs, int xdisplay, const char *rendernode);
extern void nemoenvs_use_xserver(struct nemoenvs *envs, int xdisplay);

extern int nemoenvs_launch_xapp(struct nemoenvs *envs, const char *_path, const char *_args, const char *_envp, const char *_states);

extern int nemoenvs_attach_xclient(struct nemoenvs *envs, struct nemoxserver *xserver, pid_t pid, const char *name);
extern int nemoenvs_detach_xclient(struct nemoenvs *envs, pid_t pid);

extern int nemoenvs_terminate_xclient(struct nemoenvs *envs, pid_t pid);
extern int nemoenvs_terminate_xclients(struct nemoenvs *envs);
extern int nemoenvs_terminate_xservers(struct nemoenvs *envs);
extern int nemoenvs_terminate_xapps(struct nemoenvs *envs);

extern int nemoenvs_get_xclient_count(struct nemoenvs *envs);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
