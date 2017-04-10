#ifndef __NEMOYOYO_SWEEP_H__
#define __NEMOYOYO_SWEEP_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemotimer.h>
#include <nemoaction.h>

#include <nemoyoyo.h>

struct yoyosweep {
	struct nemoyoyo *yoyo;

	struct actiontap *tap;

	struct nemotimer *timer;

	float minimum_range, maximum_range;
	float minimum_angle, maximum_angle;
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

NEMOYOYO_DECLARE_SET_ATTRIBUTE(sweep, float, minimum_range, minimum_range);
NEMOYOYO_DECLARE_SET_ATTRIBUTE(sweep, float, maximum_range, maximum_range);
NEMOYOYO_DECLARE_SET_ATTRIBUTE(sweep, float, minimum_angle, minimum_angle);
NEMOYOYO_DECLARE_SET_ATTRIBUTE(sweep, float, maximum_angle, maximum_angle);
NEMOYOYO_DECLARE_SET_ATTRIBUTE(sweep, uint32_t, minimum_interval, minimum_interval);
NEMOYOYO_DECLARE_SET_ATTRIBUTE(sweep, uint32_t, maximum_interval, maximum_interval);
NEMOYOYO_DECLARE_SET_ATTRIBUTE(sweep, uint32_t, minimum_duration, minimum_duration);
NEMOYOYO_DECLARE_SET_ATTRIBUTE(sweep, uint32_t, maximum_duration, maximum_duration);
NEMOYOYO_DECLARE_SET_ATTRIBUTE(sweep, float, feedback_sx0, feedback_sx0);
NEMOYOYO_DECLARE_SET_ATTRIBUTE(sweep, float, feedback_sx1, feedback_sx1);
NEMOYOYO_DECLARE_SET_ATTRIBUTE(sweep, float, feedback_sy0, feedback_sy0);
NEMOYOYO_DECLARE_SET_ATTRIBUTE(sweep, float, feedback_sy1, feedback_sy1);
NEMOYOYO_DECLARE_SET_ATTRIBUTE(sweep, float, feedback_alpha0, feedback_alpha0);
NEMOYOYO_DECLARE_SET_ATTRIBUTE(sweep, float, feedback_alpha1, feedback_alpha1);
NEMOYOYO_DECLARE_SET_ATTRIBUTE(sweep, float, actor_distance, actor_distance);
NEMOYOYO_DECLARE_SET_ATTRIBUTE(sweep, uint32_t, actor_duration, actor_duration);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
