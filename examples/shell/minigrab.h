#ifndef __MINISHELL_GRAB_H__
#define __MINISHELL_GRAB_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <minishell.h>
#include <talehelper.h>

struct minigrab {
	struct talegrab base;

	double x, y;
	double dx, dy;

	struct minishell *mini;
	void *userdata;
};

extern struct minigrab *minishell_grab_create(struct minishell *mini, struct nemotale *tale, struct taleevent *event, nemotale_dispatch_grab_t dispatch, void *userdata);
extern void minishell_grab_destroy(struct minigrab *grab);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
