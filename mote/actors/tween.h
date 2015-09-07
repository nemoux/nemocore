#ifndef __NEMOMOTE_TWEEN_ACTOR_H__
#define __NEMOMOTE_TWEEN_ACTOR_H__

struct motetween {
	double *dst;
	double *src;

	int sparts, nparts;

	double dtime, rtime;
	int done;
};

extern int nemomote_tween_prepare(struct motetween *tween, int max);
extern void nemomote_tween_finish(struct motetween *tween);

extern void nemomote_tween_clear(struct motetween *tween);
extern void nemomote_tween_add(struct motetween *tween, double x, double y);
extern void nemomote_tween_shuffle(struct motetween *tween);

extern void nemomote_tween_ready(struct nemomote *mote, uint32_t type, struct motetween *tween, double secs);
extern int nemomote_tween_update(struct nemomote *mote, uint32_t type, struct motetween *tween, double secs);

static inline int nemomote_tween_get_count(struct motetween *tween)
{
	return tween->nparts;
}

#endif
