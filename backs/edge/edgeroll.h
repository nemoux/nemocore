#ifndef __NEMOUX_EDGE_ROLL_H__
#define __NEMOUX_EDGE_ROLL_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemolist.h>
#include <nemolistener.h>

#include <edgeback.h>
#include <showhelper.h>

#define EDGEBACK_GROUP_MAX			(8)
#define EDGEBACK_ACTION_MAX			(16)

typedef enum {
	EDGEBACK_ROLL_READY_STATE = 0,
	EDGEBACK_ROLL_ACTIVE_STATE = 1,
	EDGEBACK_ROLL_DONE_STATE = 2,
	EDGEBACK_ROLL_LAST_STATE
} EdgeBackRollState;

struct nemotimer;

struct edgeroll {
	struct edgeback *edge;

	struct nemosignal destroy_signal;

	struct nemotimer *timer;

	struct showone *groups[EDGEBACK_GROUP_MAX];
	int ngroups;

	struct showone *actions[EDGEBACK_ACTION_MAX];
	int nactions;

	int site;

	uint32_t serial;
	uint32_t state;
	uint32_t refs;

	int groupidx;
	int actionidx;

	double r;

	double x0, y0;
	double x1, y1;

	int tapcount;
};

extern struct edgeroll *nemoback_edgeroll_create(struct edgeback *edge, int site);
extern void nemoback_edgeroll_destroy(struct edgeroll *roll);

extern int nemoback_edgeroll_shutdown(struct edgeback *edge, struct edgeroll *roll);

extern int nemoback_edgeroll_down(struct edgeback *edge, struct edgeroll *roll, double x, double y);
extern int nemoback_edgeroll_motion(struct edgeback *edge, struct edgeroll *roll, double x, double y);
extern int nemoback_edgeroll_up(struct edgeback *edge, struct edgeroll *roll, double x, double y);

extern int nemoback_edgeroll_activate_group(struct edgeback *edge, struct edgeroll *roll, uint32_t group);
extern int nemoback_edgeroll_deactivate_group(struct edgeback *edge, struct edgeroll *roll);

extern int nemoback_edgeroll_activate_action(struct edgeback *edge, struct edgeroll *roll, uint32_t action);
extern int nemoback_edgeroll_deactivate_action(struct edgeback *edge, struct edgeroll *roll);

static inline void nemoback_edgeroll_reference(struct edgeroll *roll)
{
	roll->refs++;
}

static inline uint32_t nemoback_edgeroll_unreference(struct edgeroll *roll)
{
	return --roll->refs;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
