#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomosi.h>
#include <mosirandom.h>
#include <nemomisc.h>

void nemomosi_rain_dispatch(struct nemomosi *mosi, uint32_t msecs, uint32_t s0, uint32_t d0, uint32_t d1, double t0, double t1)
{
	struct mosione *one;
	uint32_t d9;
	double t9;
	int i, j;

	for (j = 0; j < mosi->width; j++) {
		d9 = random_get_int(d0, d1);
		t9 = random_get_double(t0, t1);

		for (i = 0; i < mosi->height; i++) {
			one = &mosi->ones[i * mosi->width + j];

			one->stime = msecs + s0 + d9 * ((double)i / (double)mosi->height);
			one->etime = one->stime + d9 * t9;

			one->has_transition = 1;
		}
	}
}
