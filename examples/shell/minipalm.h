#ifndef	__MINISHELL_PALM_H__
#define	__MINISHELL_PALM_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <minishell.h>
#include <minigrab.h>

#include <showhelper.h>

struct minipalm {
	struct minigrab *fingers[5];

	struct showone *group;
};

extern struct minipalm *minishell_palm_create(void);
extern void minishell_palm_destroy(struct minipalm *palm);

extern void minishell_palm_prepare(struct minishell *mini, struct minigrab *grab);
extern void minishell_palm_update(struct minishell *mini, struct minigrab *grab);
extern void minishell_palm_finish(struct minishell *mini, struct minigrab *grab);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
