#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomosi.h>
#include <mosirandom.h>
#include <nemomisc.h>

void nemomosi_random_dispatch(struct nemomosi *mosi, uint32_t msecs, uint32_t s0, uint32_t s1, uint32_t d0, uint32_t d1)
{
	struct mosione *one;
	int i, j;

	for (i = 0; i < mosi->height; i++) {
		for (j = 0; j < mosi->width; j++) {
			one = &mosi->ones[i * mosi->width + j];

			nemomosi_one_set_transition(one,
					msecs + random_get_int(s0, s1),
					random_get_int(d0, d1));
		}
	}
}
