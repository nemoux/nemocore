#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <math.h>

#include <nemomosi.h>
#include <mosiwave.h>
#include <nemomisc.h>

void nemomosi_wave_dispatch(struct nemomosi *mosi, uint32_t msecs, double x, double y, double r, uint32_t s, uint32_t d0, uint32_t d1)
{
	struct mosione *one;
	double d, dx, dy;
	uint32_t stime;
	uint32_t etime;
	int i, j;

	for (i = 0; i < mosi->height; i++) {
		for (j = 0; j < mosi->width; j++) {
			one = &mosi->ones[i * mosi->width + j];

			dx = j - x;
			dy = i - y;

			d = sqrtf(dx * dx + dy * dy);
			if (d <= r) {
				stime = msecs + s * (d / r);
				etime = stime + random_get_int(d0, d1);

				if (one->has_transition == 0 || one->stime > stime) {
					one->stime = stime;
					one->etime = etime;

					one->has_transition = 1;
				}
			}
		}
	}
}
