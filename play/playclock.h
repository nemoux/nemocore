#ifndef __NEMOPLAY_CLOCK_H__
#define __NEMOPLAY_CLOCK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <pthread.h>

struct playclock {
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

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
