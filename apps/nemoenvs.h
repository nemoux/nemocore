#ifndef	__NEMOTOOL_ENVS_H__
#define	__NEMOTOOL_ENVS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemotool.h>
#include <nemomonitor.h>
#include <nemomsg.h>

#include <nemobox.h>
#include <nemoitem.h>
#include <nemolist.h>

struct nemoenvs {
	struct nemotool *tool;

	struct nemoitem *configs;

	struct nemomsg *msg;
	struct nemomonitor *monitor;
};

extern struct nemoenvs *nemoenvs_create(struct nemotool *tool);
extern void nemoenvs_destroy(struct nemoenvs *envs);

extern int nemoenvs_connect(struct nemoenvs *envs, const char *ip, int port);

extern void nemoenvs_load_configs(struct nemoenvs *envs, const char *configpath);

static inline int nemoenvs_set_callback(struct nemoenvs *envs, nemomsg_callback_t callback, void *data)
{
	return nemomsg_set_callback(envs->msg, callback, data);
}

static inline int nemoenvs_set_source_callback(struct nemoenvs *envs, const char *name, nemomsg_callback_t callback, void *data)
{
	return nemomsg_set_source_callback(envs->msg, name, callback, data);
}

static inline int nemoenvs_set_destination_callback(struct nemoenvs *envs, const char *name, nemomsg_callback_t callback, void *data)
{
	return nemomsg_set_destination_callback(envs->msg, name, callback, data);
}

static inline int nemoenvs_set_command_callback(struct nemoenvs *envs, const char *name, nemomsg_callback_t callback, void *data)
{
	return nemomsg_set_command_callback(envs->msg, name, callback, data);
}

static inline int nemoenvs_put_callback(struct nemoenvs *envs, nemomsg_callback_t callback)
{
	return nemomsg_put_callback(envs->msg, callback);
}

static inline int nemoenvs_put_source_callback(struct nemoenvs *envs, const char *name, nemomsg_callback_t callback)
{
	return nemomsg_put_source_callback(envs->msg, name, callback);
}

static inline int nemoenvs_put_destination_callback(struct nemoenvs *envs, const char *name, nemomsg_callback_t callback)
{
	return nemomsg_put_destination_callback(envs->msg, name, callback);
}

static inline int nemoenvs_put_command_callback(struct nemoenvs *envs, const char *name, nemomsg_callback_t callback)
{
	return nemomsg_put_command_callback(envs->msg, name, callback);
}

static inline void nemoenvs_set_data(struct nemoenvs *envs, void *data)
{
	nemomsg_set_data(envs->msg, data);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
