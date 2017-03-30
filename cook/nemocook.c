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

void nemocook_draw_texture(uint32_t tex, float x0, float y0, float x1, float y1)
{
	GLfloat vertices[16] = {
		x0, y0, 0.0f, 1.0f,
		x1, y0, 1.0f, 1.0f,
		x1, y1, 1.0f, 0.0f,
		x0, y1, 0.0f, 0.0f
	};

	glBindTexture(GL_TEXTURE_2D, tex);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[2]);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glBindTexture(GL_TEXTURE_2D, 0);
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
