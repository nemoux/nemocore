#ifndef __MINISHELL_GRAB_H__
#define __MINISHELL_GRAB_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <minishell.h>
#include <talehelper.h>
#include <showhelper.h>
#include <geometryhelper.h>

typedef enum {
	MINISHELL_NORMAL_GRAB = 0,
	MINISHELL_PALM_GRAB = 1,
	MINISHELL_ACTIVE_GRAB = 2,
	MINISHELL_YOYO_GRAB = 3,
	MINISHELL_LAST_GRAB
} MiniShellGrabType;

struct minigrab {
	struct talegrab base;

	struct nemolist link;

	int type;
	uint32_t serial;

	struct showone *group;
	struct showone *edge;
	struct showone *one;

	double x, y;
	double dx, dy;
	double ro;

	double lx, ly;
	uint32_t ltime;

	struct minishell *mini;
	void *userdata;
};

extern struct minigrab *minishell_grab_create(struct minishell *mini, struct nemotale *tale, struct taleevent *event, nemotale_dispatch_grab_t dispatch, void *userdata);
extern void minishell_grab_destroy(struct minigrab *grab);

static inline int minishell_grab_check_update(struct minigrab *grab, struct taleevent *event, uint32_t interval, double distance)
{
	return grab->ltime + interval < event->time ||
		point_get_distance(grab->lx, grab->ly, event->x, event->y) > distance;
}

static inline void minishell_grab_update(struct minigrab *grab, struct taleevent *event)
{
	grab->ltime = event->time;
	grab->lx = event->x;
	grab->ly = event->y;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
