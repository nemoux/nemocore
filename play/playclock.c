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

	clock->speed = 1.0f;

	return clock;
}

void nemoplay_clock_destroy(struct playclock *clock)
{
	free(clock);
}

void nemoplay_clock_set_speed(struct playclock *clock, double speed)
{
	clock->speed = speed;
}

void nemoplay_clock_set(struct playclock *clock, double ptime)
{
	double ctime = av_gettime_relative() / 1000000.0f;

	clock->ptime = ptime;
	clock->ltime = ctime;
	clock->dtime = ptime - ctime;
}

double nemoplay_clock_get(struct playclock *clock)
{
	double ctime = av_gettime_relative() / 1000000.0f;

	return clock->dtime + ctime - (ctime - clock->ltime) * (1.0f - clock->speed);
}
