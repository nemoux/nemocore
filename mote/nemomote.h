#ifndef	__NEMOMOTE_H__
#define	__NEMOMOTE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <emitters/blast.h>
#include <emitters/random.h>
#include <emitters/sine.h>
#include <actors/move.h>
#include <actors/accelerate.h>
#include <actors/explosion.h>
#include <actors/gravitywell.h>
#include <actors/antigravity.h>
#include <actors/mutualgravity.h>
#include <actors/mousegravity.h>
#include <actors/deathzone.h>
#include <actors/livezone.h>
#include <actors/boundingbox.h>
#include <actors/speedscale.h>
#include <actors/speedlimit.h>
#include <actors/deadline.h>
#include <actors/collide.h>
#include <actors/tween.h>
#include <builders/position.h>
#include <builders/velocity.h>
#include <builders/mass.h>
#include <builders/lifetime.h>
#include <builders/sleeptime.h>
#include <builders/color.h>
#include <builders/type.h>
#include <builders/tweener.h>
#include <nemozone.h>

#define	NEMOMOTE_DEAD_BIT			(1 << 0)

struct nemomote {
	double *buffers;
	uint32_t *types;
	uint32_t *attrs;
	double *tweens;

	int mcount;
	int lcount;
	int rcount;
};

extern struct nemomote *nemomote_create(int max);
extern void nemomote_destroy(struct nemomote *mote);

extern int nemomote_reset(struct nemomote *mote);

extern int nemomote_ready(struct nemomote *mote, unsigned int count);
extern int nemomote_commit(struct nemomote *mote);
extern int nemomote_cleanup(struct nemomote *mote);

extern int nemomote_get_one_by_type(struct nemomote *mote, uint32_t type);

static inline int nemomote_get_count(struct nemomote *mote)
{
	return mote->lcount;
}

#define NEMOMOTE_POSITION_X(m, i)			((m)->buffers[i * 12 + 0])
#define NEMOMOTE_POSITION_Y(m, i)			((m)->buffers[i * 12 + 1])

#define NEMOMOTE_VELOCITY_X(m, i)			((m)->buffers[i * 12 + 2])
#define NEMOMOTE_VELOCITY_Y(m, i)			((m)->buffers[i * 12 + 3])

#define NEMOMOTE_COLOR_R(m, i)				((m)->buffers[i * 12 + 4])
#define NEMOMOTE_COLOR_G(m, i)				((m)->buffers[i * 12 + 5])
#define NEMOMOTE_COLOR_B(m, i)				((m)->buffers[i * 12 + 6])
#define NEMOMOTE_COLOR_A(m, i)				((m)->buffers[i * 12 + 7])

#define NEMOMOTE_MASS(m, i)						((m)->buffers[i * 12 + 8])
#define NEMOMOTE_LIFETIME(m, i)				((m)->buffers[i * 12 + 9])
#define NEMOMOTE_SLEEPTIME(m, i)			((m)->buffers[i * 12 + 10])

#define	NEMOMOTE_TYPE(m, i)						((m)->types[i])

#define	NEMOMOTE_TWEEN_SX(m, i)				((m)->tweens[i * 6 + 0])
#define	NEMOMOTE_TWEEN_SY(m, i)				((m)->tweens[i * 6 + 1])

#define	NEMOMOTE_TWEEN_DX(m, i)				((m)->tweens[i * 6 + 2])
#define	NEMOMOTE_TWEEN_DY(m, i)				((m)->tweens[i * 6 + 3])

#define NEMOMOTE_TWEEN_DT(m, i)				((m)->tweens[i * 6 + 4])
#define NEMOMOTE_TWEEN_RT(m, i)				((m)->tweens[i * 6 + 5])

#ifdef __cplusplus
}
#endif

#endif
