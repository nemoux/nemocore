#ifndef __NEMOPLAY_CLOCK_H__
#define __NEMOPLAY_CLOCK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <pthread.h>

typedef enum {
	NEMOPLAY_CLOCK_NORMAL_STATE = 0,
	NEMOPLAY_CLOCK_STOP_STATE = 1,
	NEMOPLAY_CLOCK_LAST_STATE
} NemoPlayClockState;

struct playclock {
	int state;

	double ptime;
	double dtime;
	double ltime;

	double speed;

	pthread_mutex_t lock;
};

extern struct playclock *nemoplay_clock_create(void);
extern void nemoplay_clock_destroy(struct playclock *clock);

extern void nemoplay_clock_set_speed(struct playclock *clock, double speed);
extern void nemoplay_clock_set(struct playclock *clock, double ptime);
extern double nemoplay_clock_get(struct playclock *clock);

extern void nemoplay_clock_set_state(struct playclock *clock, int state);

static inline int nemoplay_clock_get_state(struct playclock *clock)
{
	return clock->state;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
