#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <math.h>

#include <nemomosi.h>
#include <mosirandom.h>
#include <nemomisc.h>

void nemomosi_cross_dispatch(struct nemomosi *mosi, uint32_t msecs, uint32_t x, uint32_t y, uint32_t s0, uint32_t d0, uint32_t d1, double t0, double t1)
{
	struct mosione *one;
	uint32_t stime, dtime;
	uint32_t d9;
	double t9;
	int i;

	d9 = random_get_int(d0, d1);
	t9 = random_get_double(t0, t1);

	for (i = x; i >= 0; i--) {
		one = &mosi->ones[y * mosi->width + i];

		stime = msecs + s0 + d9 * ((double)abs(x - i) / (double)mosi->height);
		dtime = d9 * t9;

		nemomosi_one_set_transition(one, stime, dtime);
	}

	for (i = x; i < mosi->width; i++) {
		one = &mosi->ones[y * mosi->width + i];

		stime = msecs + s0 + d9 * ((double)abs(x - i) / (double)mosi->height);
		dtime = d9 * t9;

		nemomosi_one_set_transition(one, stime, dtime);
	}

	for (i = y; i >= 0; i--) {
		one = &mosi->ones[i * mosi->width + x];

		stime = msecs + s0 + d9 * ((double)abs(y - i) / (double)mosi->height);
		dtime = d9 * t9;

		nemomosi_one_set_transition(one, stime, dtime);
	}

	for (i = y; i < mosi->height; i++) {
		one = &mosi->ones[i * mosi->width + x];

		stime = msecs + s0 + d9 * ((double)abs(y - i) / (double)mosi->height);
		dtime = d9 * t9;

		nemomosi_one_set_transition(one, stime, dtime);
	}
}
