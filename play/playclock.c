#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemoplay.h>
#include <playclock.h>
#include <nemomisc.h>

struct playclock *nemoplay_clock_create(void)
{
	struct playclock *clock;

	clock = (struct playclock *)malloc(sizeof(struct playclock));
	if (clock == NULL)
		return NULL;
	memset(clock, 0, sizeof(struct playclock));

	if (pthread_mutex_init(&clock->lock, NULL) != 0)
		goto err1;

	clock->speed = 1.0f;

	clock->ptime = 0.0f;
	clock->ltime = av_gettime_relative() / 1000000.0f;
	clock->dtime = -clock->ltime;

	return clock;

err1:
	free(clock);

	return NULL;
}

void nemoplay_clock_destroy(struct playclock *clock)
{
	pthread_mutex_destroy(&clock->lock);

	free(clock);
}

void nemoplay_clock_set_speed(struct playclock *clock, double speed)
{
	pthread_mutex_lock(&clock->lock);

	clock->speed = speed;

	pthread_mutex_unlock(&clock->lock);
}

void nemoplay_clock_set(struct playclock *clock, double ptime)
{
	double ctime = av_gettime_relative() / 1000000.0f;

	pthread_mutex_lock(&clock->lock);

	clock->ptime = ptime;
	clock->ltime = ctime;
	clock->dtime = ptime - ctime;

	pthread_mutex_unlock(&clock->lock);
}

double nemoplay_clock_get(struct playclock *clock)
{
	double ctime = av_gettime_relative() / 1000000.0f;
	double rtime;

	pthread_mutex_lock(&clock->lock);

	if (clock->state == NEMOPLAY_CLOCK_NORMAL_STATE)
		rtime = clock->dtime + ctime - (ctime - clock->ltime) * (1.0f - clock->speed);
	else
		rtime = clock->dtime + clock->ltime;

	pthread_mutex_unlock(&clock->lock);

	return rtime;
}

void nemoplay_clock_set_state(struct playclock *clock, int state)
{
	pthread_mutex_lock(&clock->lock);

	clock->state = state;

	if (state == NEMOPLAY_CLOCK_NORMAL_STATE) {
		double ctime = av_gettime_relative() / 1000000.0f;

		clock->dtime += (clock->ltime - ctime);
		clock->ltime = ctime;
	}

	pthread_mutex_unlock(&clock->lock);
}
