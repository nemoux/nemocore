#ifndef	__NEMOSHELL_ENVS_H__
#define	__NEMOSHELL_ENVS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemobox.h>

typedef enum {
	NEMOENVS_ACTION_APP_TYPE = 0,
	NEMOENVS_ACTION_CHROME_TYPE = 1,
	NEMOENVS_ACTION_KEYPAD_TYPE = 2,
	NEMOENVS_ACTION_SPEAKER_TYPE = 3,
	NEMOENVS_ACTION_NONE_TYPE = 4,
	NEMOENVS_ACTION_LAST_TYPE
} NemoEnvsActionType;

typedef enum {
	NEMOENVS_APP_BACKGROUND_TYPE = 0,
	NEMOENVS_APP_SOUNDMANAGER_TYPE = 1,
	NEMOENVS_APP_LAST_TYPE
} NemoEnvsAppType;

typedef enum {
	NEMOENVS_GROUP_NORMAL_TYPE = 0,
	NEMOENVS_GROUP_CONTENTS_TYPE = 1,
	NEMOENVS_GROUP_LAST_TYPE
} NemoEnvsGroupType;

typedef enum {
	NEMOENVS_NETWORK_NORMAL_STATE = 0,
	NEMOENVS_NETWORK_BLOCK_STATE = 1,
	NEMOENVS_NETWORK_LAST_STATE
} NemoEnvsNetworkState;

struct nemoshell;

struct nemoaction {
	char *path;
	char *icon;
	char *ring;
	char *user;
	char *args[16];

	uint32_t flags;

	int type;
	int input;
	int network;

	uint32_t max_width, max_height;
	uint32_t min_width, min_height;
	int has_min_size, has_max_size;

	uint32_t fadein_type;
	uint32_t fadein_ease;
	uint32_t fadein_delay;
	uint32_t fadein_duration;
};

struct nemogroup {
	struct nemoaction **actions;
	int sactions, nactions;

	char *path;
	char *icon;
	char *ring;

	int type;
};

struct nemoapp {
	pid_t pid;

	int type;
	int index;

	struct nemolist link;
};

struct nemoenvs {
	struct nemoshell *shell;

	struct nemogroup **groups;
	int sgroups, ngroups;

	struct nemolist app_list;
};

extern struct nemoenvs *nemoenvs_create(struct nemoshell *shell);
extern void nemoenvs_destroy(struct nemoenvs *envs);

extern struct nemoaction *nemoenvs_create_action(void);
extern void nemoenvs_destroy_action(struct nemoaction *action);

extern struct nemogroup *nemoenvs_create_group(void);
extern void nemoenvs_destroy_group(struct nemogroup *group);

extern struct nemoapp *nemoenvs_create_app(void);
extern void nemoenvs_destroy_app(struct nemoapp *app);

extern int nemoenvs_attach_app(struct nemoenvs *envs, int type, int index, pid_t pid);
extern void nemoenvs_detach_app(struct nemoenvs *envs, pid_t pid);

extern int nemoenvs_respawn_app(struct nemoenvs *envs, pid_t pid);

extern void nemoenvs_load_actions(struct nemoenvs *envs);

extern void nemoenvs_execute_background(struct nemoenvs *envs, int index);
extern void nemoenvs_execute_backgrounds(struct nemoenvs *envs);
extern void nemoenvs_execute_soundmanager(struct nemoenvs *envs);

static inline int nemoenvs_get_groups_count(struct nemoenvs *envs)
{
	return envs->ngroups;
}

static inline int nemoenvs_get_actions_count(struct nemoenvs *envs, int group)
{
	return envs->groups[group]->nactions;
}

static inline struct nemogroup *nemoenvs_get_group(struct nemoenvs *envs, int group)
{
	return envs->groups[group];
}

static inline struct nemoaction *nemoenvs_get_action(struct nemoenvs *envs, int group, int action)
{
	return envs->groups[group]->actions[action];
}

static inline const char *nemoenvs_get_group_path(struct nemoenvs *envs, int group)
{
	return envs->groups[group]->path;
}

static inline const char *nemoenvs_get_group_icon(struct nemoenvs *envs, int group)
{
	return envs->groups[group]->icon;
}

static inline const char *nemoenvs_get_group_ring(struct nemoenvs *envs, int group)
{
	return envs->groups[group]->ring;
}

static inline int nemoenvs_get_group_type(struct nemoenvs *envs, int group)
{
	return envs->groups[group]->type;
}

static inline const char *nemoenvs_get_action_icon(struct nemoenvs *envs, int group, int action)
{
	return envs->groups[group]->actions[action]->icon;
}

static inline const char *nemoenvs_get_action_ring(struct nemoenvs *envs, int group, int action)
{
	return envs->groups[group]->actions[action]->ring;
}

static inline int nemoenvs_get_action_type(struct nemoenvs *envs, int group, int action)
{
	return envs->groups[group]->actions[action]->type;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
