#ifndef __MINISHELL_GRAB_H__
#define __MINISHELL_GRAB_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <minishell.h>
#include <talehelper.h>
#include <showhelper.h>

typedef enum {
	MINISHELL_NORMAL_GRAB = 0,
	MINISHELL_PALM_GRAB = 1,
	MINISHELL_LAST_GRAB
} MiniShellGrabType;

struct minigrab {
	struct talegrab base;

	struct nemolist link;

	int type;
	uint32_t serial;

	struct showone *one;

	double x, y;
	double dx, dy;

	double lx, ly;
	uint32_t ltime;

	struct minishell *mini;
	void *userdata;
};

extern struct minigrab *minishell_grab_create(struct minishell *mini, struct nemotale *tale, struct taleevent *event, struct showone *one, nemotale_dispatch_grab_t dispatch, void *userdata);
extern void minishell_grab_destroy(struct minigrab *grab);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
