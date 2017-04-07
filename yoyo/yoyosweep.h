#ifndef __NEMOYOYO_SWEEP_H__
#define __NEMOYOYO_SWEEP_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemotimer.h>
#include <nemoaction.h>

struct nemoyoyo;

struct yoyosweep {
	struct nemoyoyo *yoyo;

	struct actiontap *tap;

	struct nemotimer *timer;

	float minimum_range, maximum_range;
	uint32_t minimum_interval, maximum_interval;
	uint32_t minimum_duration, maximum_duration;

	float feedback_sx0, feedback_sx1;
	float feedback_sy0, feedback_sy1;
	float feedback_alpha0, feedback_alpha1;

	float actor_distance;
	uint32_t actor_duration;
};

extern struct yoyosweep *nemoyoyo_sweep_create(struct nemoyoyo *yoyo);
extern void nemoyoyo_sweep_destroy(struct yoyosweep *sweep);

extern int nemoyoyo_sweep_dispatch(struct yoyosweep *sweep, struct actiontap *tap);

static inline void nemoyoyo_sweep_set_minimum_range(struct yoyosweep *sweep, float min)
{
	sweep->minimum_range = min;
}

static inline void nemoyoyo_sweep_set_maximum_range(struct yoyosweep *sweep, float max)
{
	sweep->maximum_range = max;
}

static inline void nemoyoyo_sweep_set_minimum_interval(struct yoyosweep *sweep, uint32_t min)
{
	sweep->minimum_interval = min;
}

static inline void nemoyoyo_sweep_set_maximum_interval(struct yoyosweep *sweep, uint32_t max)
{
	sweep->maximum_interval = max;
}

static inline void nemoyoyo_sweep_set_minimum_duration(struct yoyosweep *sweep, uint32_t min)
{
	sweep->minimum_duration = min;
}

static inline void nemoyoyo_sweep_set_maximum_duration(struct yoyosweep *sweep, uint32_t max)
{
	sweep->maximum_duration = max;
}

static inline void nemoyoyo_sweep_set_feedback_sx(struct yoyosweep *sweep, float sx0, float sx1)
{
	sweep->feedback_sx0 = sx0;
	sweep->feedback_sx1 = sx1;
}

static inline void nemoyoyo_sweep_set_feedback_sy(struct yoyosweep *sweep, float sy0, float sy1)
{
	sweep->feedback_sy0 = sy0;
	sweep->feedback_sy1 = sy1;
}

static inline void nemoyoyo_sweep_set_feedback_alpha(struct yoyosweep *sweep, float alpha0, float alpha1)
{
	sweep->feedback_alpha0 = alpha0;
	sweep->feedback_alpha1 = alpha1;
}

static inline void nemoyoyo_sweep_set_actor_distance(struct yoyosweep *sweep, float distance)
{
	sweep->actor_distance = distance;
}

static inline void nemoyoyo_sweep_set_actor_duration(struct yoyosweep *sweep, uint32_t duration)
{
	sweep->actor_duration = duration;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
