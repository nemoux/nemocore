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

void nemocook_clear_color_buffer(float r, float g, float b, float a)
{
	glClearColor(r, g, b, a);
	glClear(GL_COLOR_BUFFER_BIT);
}

void nemocook_clear_depth_buffer(float d)
{
	glClearDepth(d);
	glClear(GL_DEPTH_BUFFER_BIT);
}
