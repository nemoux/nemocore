#ifndef __NEMOYOYO_DOTOR_H__
#define __NEMOYOYO_DOTOR_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemotimer.h>

struct nemoyoyo;
struct actiontap;

struct yoyodotor {
	struct nemoyoyo *yoyo;

	struct actiontap *tap;

	struct nemotimer *timer;

	float minimum_range, maximum_range;
	uint32_t minimum_interval, maximum_interval;
	uint32_t minimum_duration, maximum_duration;
};

extern struct yoyodotor *nemoyoyo_dotor_create(struct nemoyoyo *yoyo, struct actiontap *tap);
extern void nemoyoyo_dotor_destroy(struct yoyodotor *dotor);

extern void nemoyoyo_dotor_dispatch(struct yoyodotor *dotor, struct nemotool *tool);

static inline void nemoyoyo_dotor_set_minimum_range(struct yoyodotor *dotor, float min)
{
	dotor->minimum_range = min;
}

static inline void nemoyoyo_dotor_set_maximum_range(struct yoyodotor *dotor, float max)
{
	dotor->maximum_range = max;
}

static inline void nemoyoyo_dotor_set_minimum_interval(struct yoyodotor *dotor, uint32_t min)
{
	dotor->minimum_interval = min;
}

static inline void nemoyoyo_dotor_set_maximum_interval(struct yoyodotor *dotor, uint32_t max)
{
	dotor->maximum_interval = max;
}

static inline void nemoyoyo_dotor_set_minimum_duration(struct yoyodotor *dotor, uint32_t min)
{
	dotor->minimum_duration = min;
}

static inline void nemoyoyo_dotor_set_maximum_duration(struct yoyodotor *dotor, uint32_t max)
{
	dotor->maximum_duration = max;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
