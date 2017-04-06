#ifndef __NEMOYOYO_SWEEP_H__
#define __NEMOYOYO_SWEEP_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemotimer.h>

struct nemoyoyo;
struct actiontap;

struct yoyosweep {
	struct nemoyoyo *yoyo;

	struct actiontap *tap;

	struct nemotimer *timer;

	float minimum_range, maximum_range;
	uint32_t minimum_interval, maximum_interval;
	uint32_t minimum_duration, maximum_duration;

	float actor_distance;
	uint32_t actor_duration;
};

extern struct yoyosweep *nemoyoyo_sweep_create(struct nemoyoyo *yoyo, struct actiontap *tap);
extern void nemoyoyo_sweep_destroy(struct yoyosweep *sweep);

extern void nemoyoyo_sweep_dispatch(struct yoyosweep *sweep);

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
