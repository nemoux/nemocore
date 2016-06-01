#ifndef __NEMOSHELL_ENVS_H__
#define __NEMOSHELL_ENVS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemoapps.h>
#include <nemomsg.h>

#include <nemobox.h>
#include <nemoitem.h>

struct nemoshell;

struct nemoenvs {
	struct nemoshell *shell;

	struct nemoitem *configs;

	struct nemolist app_list;

	struct nemomsg *msg;
	struct nemomonitor *monitor;
};

extern struct nemoenvs *nemoenvs_create(struct nemoshell *shell);
extern void nemoenvs_destroy(struct nemoenvs *envs);

extern int nemoenvs_listen(struct nemoenvs *envs, const char *ip, int port);

extern void nemoenvs_load_configs(struct nemoenvs *envs, const char *configpath);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
