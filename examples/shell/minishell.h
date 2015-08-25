#ifndef __MINISHELL_H__
#define __MINISHELL_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <minishell.h>
#include <minimote.h>
#include <showhelper.h>

#include <nemolist.h>
#include <nemolistener.h>

struct minishell {
	struct nemoshell *shell;

	struct nemoshow *show;

	struct nemolist grab_list;

	uint32_t serial;

	struct showone *canvas;
	struct showone *links;
	struct showone *blur5;
	struct showone *blur15;

	struct minimote *mote;

	void **slots;
	int nslots;
};

static inline int minishell_need_slot(struct minishell *mini)
{
	int i;

	for (i = 1; i < mini->nslots; i++) {
		if (mini->slots[i] == NULL)
			return i;
	}

	return 0;
}

static inline void *minishell_get_slot(struct minishell *mini, int index)
{
	return mini->slots[index];
}

static inline void minishell_set_slot(struct minishell *mini, int index, void *data)
{
	mini->slots[index] = data;
}

static inline void minishell_put_slot(struct minishell *mini, int index)
{
	mini->slots[index] = NULL;
}

static inline int minishell_has_slot(struct minishell *mini, int index)
{
	return mini->slots[index] != NULL;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
