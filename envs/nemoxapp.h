#ifndef __NEMOSHELL_XAPP_H__
#define __NEMOSHELL_XAPP_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct nemoenvs;

struct nemoxapp {
	char *cmds;

	double x, y;
	double r;

	int has_state;

	struct wl_list link;
};

struct nemoxclient {
	struct nemoxserver *xserver;

	pid_t pid;

	struct wl_list link;
};

extern int nemoenvs_launch_xserver0(struct nemoenvs *envs);

extern int nemoenvs_launch_xapp(struct nemoenvs *envs, const char *cmds, double x, double y, double r, int has_state);

extern int nemoenvs_terminate_xclient(struct nemoenvs *envs, pid_t pid);
extern int nemoenvs_terminate_xclients(struct nemoenvs *envs);
extern int nemoenvs_terminate_xservers(struct nemoenvs *envs);
extern int nemoenvs_terminate_xapps(struct nemoenvs *envs);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
