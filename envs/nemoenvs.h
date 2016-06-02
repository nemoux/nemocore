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

struct nemoenvs;

typedef int (*nemoenvs_callback_t)(struct nemoenvs *envs, const char *src, const char *dst, const char *cmd, const char *path, struct itemone *one, void *data);

struct nemoshell;

struct envscallback {
	nemoenvs_callback_t callback;
	void *data;

	struct nemolist link;
};

struct nemoenvs {
	struct nemoshell *shell;

	struct nemoitem *configs;

	struct nemolist app_list;

	struct nemolist callback_list;

	struct nemomsg *msg;
	struct nemomonitor *monitor;
};

extern struct nemoenvs *nemoenvs_create(struct nemoshell *shell);
extern void nemoenvs_destroy(struct nemoenvs *envs);

extern int nemoenvs_listen(struct nemoenvs *envs, const char *ip, int port);
extern int nemoenvs_send(struct nemoenvs *envs, const char *src, const char *dst, const char *cmd, const char *path, const char *content);

extern int nemoenvs_set_callback(struct nemoenvs *envs, nemoenvs_callback_t callback, void *data);
extern int nemoenvs_put_callback(struct nemoenvs *envs, nemoenvs_callback_t callback, void *data);

extern int nemoenvs_dispatch(struct nemoenvs *envs, const char *src, const char *dst, const char *cmd, const char *path, struct itemone *one);

extern void nemoenvs_load_configs(struct nemoenvs *envs, const char *configpath);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
