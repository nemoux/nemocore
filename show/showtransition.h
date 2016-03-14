#ifndef	__NEMOSHOW_TRANSITION_H__
#define	__NEMOSHOW_TRANSITION_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <showsequence.h>
#include <showease.h>

#include <nemolist.h>
#include <nemolistener.h>

typedef void (*nemoshow_transition_dispatch_frame_t)(void *userdata, uint32_t time, double t);
typedef void (*nemoshow_transition_dispatch_done_t)(void *userdata);

struct transitionsensor {
	struct nemolist link;
	struct nemolistener listener;

	struct showtransition *transition;
	struct showone *one;
};

struct showtransition {
	struct showone **sequences;
	int nsequences, ssequences;

	struct showtransition **transitions;
	int ntransitions, stransitions;

	struct showone **ones;
	int nones, sones;
	uint32_t *dirties;
	int ndirties, sdirties;

	struct showone **dones;
	int ndones, sdones;

	struct showtransition *parent;

	struct showease *ease;

	uint32_t duration;
	uint32_t delay;
	uint32_t repeat;

	uint32_t stime;
	uint32_t etime;

	uint32_t serial;

	struct nemolist link;

	struct nemolist sensor_list;

	nemoshow_transition_dispatch_frame_t dispatch_frame;
	nemoshow_transition_dispatch_done_t dispatch_done;

	void *userdata;
};

extern struct showtransition *nemoshow_transition_create(struct showone *ease, uint32_t duration, uint32_t delay);
extern void nemoshow_transition_destroy(struct showtransition *trans, int done);

extern void nemoshow_transition_check_one(struct showtransition *trans, struct showone *one);
extern void nemoshow_transition_dirty_one(struct showtransition *trans, struct showone *one, uint32_t dirty);
extern void nemoshow_transition_destroy_one(struct showtransition *trans, struct showone *one);

extern void nemoshow_transition_attach_sequence(struct showtransition *trans, struct showone *sequence);

extern int nemoshow_transition_dispatch(struct showtransition *trans, uint32_t time);

static inline void nemoshow_transition_set_dispatch_frame(struct showtransition *trans, nemoshow_transition_dispatch_frame_t dispatch_frame)
{
	trans->dispatch_frame = dispatch_frame;
}

static inline void nemoshow_transition_set_dispatch_done(struct showtransition *trans, nemoshow_transition_dispatch_done_t dispatch_done)
{
	trans->dispatch_done = dispatch_done;
}

static inline void nemoshow_transition_set_repeat(struct showtransition *trans, uint32_t repeat)
{
	trans->repeat = repeat;
}

static inline void nemoshow_transition_set_userdata(struct showtransition *trans, void *data)
{
	trans->userdata = data;
}

static inline void *nemoshow_transition_get_userdata(struct showtransition *trans)
{
	return trans->userdata;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
