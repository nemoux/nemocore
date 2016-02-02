#ifndef	__NEMOSHELL_ENVS_H__
#define	__NEMOSHELL_ENVS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemobox.h>

typedef enum {
	NEMOENVS_ACTION_APP_TYPE = 0,
	NEMOENVS_ACTION_KEYPAD_TYPE = 1,
	NEMOENVS_ACTION_SPEAKER_TYPE = 2,
	NEMOENVS_ACTION_LAST_TYPE
} NemoEnvsActionType;

struct nemoshell;

struct nemoaction {
	char *path;
	char *icon;
	char *ring;
	char *args[16];

	uint32_t flags;

	int type;
	int input;

	uint32_t max_width, max_height;
	uint32_t min_width, min_height;
	int has_min_size, has_max_size;

	uint32_t fadein_type;
	uint32_t fadein_ease;
	uint32_t fadein_delay;
	uint32_t fadein_duration;

	uint32_t time;
};

struct nemogroup {
	struct nemoaction **actions;
	int sactions, nactions;

	char *icon;
	char *ring;
};

struct nemoenvs {
	struct nemogroup **groups;
	int sgroups, ngroups;
};

extern struct nemoenvs *nemoenvs_create(void);
extern void nemoenvs_destroy(struct nemoenvs *envs);

extern void nemoenvs_load_actions(struct nemoshell *shell, struct nemoenvs *envs);

extern void nemoenvs_load_background(struct nemoshell *shell);
extern void nemoenvs_load_soundmanager(struct nemoshell *shell);

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

static inline const char *nemoenvs_get_group_icon(struct nemoenvs *envs, int group)
{
	return envs->groups[group]->icon;
}

static inline const char *nemoenvs_get_group_ring(struct nemoenvs *envs, int group)
{
	return envs->groups[group]->ring;
}

static inline const char *nemoenvs_get_action_icon(struct nemoenvs *envs, int group, int action)
{
	return envs->groups[group]->actions[action]->icon;
}

static inline const char *nemoenvs_get_action_ring(struct nemoenvs *envs, int group, int action)
{
	return envs->groups[group]->actions[action]->ring;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
