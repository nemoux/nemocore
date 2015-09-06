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
#include <builders/position.h>
#include <builders/velocity.h>
#include <builders/mass.h>
#include <builders/lifetime.h>
#include <builders/color.h>
#include <builders/type.h>
#include <nemozone.h>

#define	NEMOMOTE_DEAD_BIT			(1 << 0)

struct nemomote {
	double *buffers;
	uint32_t *types;
	uint32_t *attrs;

	int mcount;
	int lcount;
	int rcount;
};

extern int nemomote_init(struct nemomote *mote);
extern int nemomote_exit(struct nemomote *mote);

extern int nemomote_set_max_particles(struct nemomote *mote, unsigned int max);
extern int nemomote_reset(struct nemomote *mote);

extern int nemomote_ready(struct nemomote *mote, unsigned int count);
extern int nemomote_commit(struct nemomote *mote);
extern int nemomote_cleanup(struct nemomote *mote);

static inline int nemomote_get_count(struct nemomote *mote)
{
	return mote->lcount;
}

#define NEMOMOTE_POSITION_X(m, i)			((m)->buffers[i * 10 + 0])
#define NEMOMOTE_POSITION_Y(m, i)			((m)->buffers[i * 10 + 1])

#define NEMOMOTE_VELOCITY_X(m, i)			((m)->buffers[i * 10 + 2])
#define NEMOMOTE_VELOCITY_Y(m, i)			((m)->buffers[i * 10 + 3])

#define NEMOMOTE_COLOR_R(m, i)				((m)->buffers[i * 10 + 4])
#define NEMOMOTE_COLOR_G(m, i)				((m)->buffers[i * 10 + 5])
#define NEMOMOTE_COLOR_B(m, i)				((m)->buffers[i * 10 + 6])
#define NEMOMOTE_COLOR_A(m, i)				((m)->buffers[i * 10 + 7])

#define NEMOMOTE_MASS(m, i)						((m)->buffers[i * 10 + 8])
#define NEMOMOTE_LIFETIME(m, i)				((m)->buffers[i * 10 + 9])

#ifdef __cplusplus
}
#endif

#endif
