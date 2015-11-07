#ifndef __NEMOMOTE_TWEEN_ACTOR_H__
#define __NEMOMOTE_TWEEN_ACTOR_H__

#include <nemoease.h>

typedef enum {
	NEMOMOTE_POSITION_TWEEN = (1 << 0),
	NEMOMOTE_COLOR_TWEEN = (1 << 1),
	NEMOMOTE_MASS_TWEEN = (1 << 2),
} NemoMoteTween;

extern int nemomote_tween_update(struct nemomote *mote, uint32_t type, double secs, struct nemoease *ease, uint32_t dtype, uint32_t tween);

#endif
