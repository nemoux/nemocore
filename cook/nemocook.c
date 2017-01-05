#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemocook.h>
#include <nemomisc.h>

void nemocook_draw_arrays(int mode, int ncounts, uint32_t *counts)
{
	int i, s;

	for (i = 0, s = 0; i < ncounts; i++) {
		glDrawArrays(mode, s, counts[i]);

		s += counts[i];
	}
}
