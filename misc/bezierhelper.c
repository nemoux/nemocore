#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <math.h>

#include <bezierhelper.h>

double cubicbezier_point(double t, double p0, double p1, double p2, double p3)
{
	return p0 * (1.0f - t) * (1.0f - t) * (1.0f - t) +
		3.0f * p1 * (1.0f - t) * (1.0f - t) * t +
		3.0f * p2 * (1.0f - t) * t * t +
		p3 * t * t * t;
}

double cubicbezier_length(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3, int steps)
{
	double length = 0.0f;
	double t;
	double px, py, cx, cy, dx, dy;
	int i;

	px = cubicbezier_point(0.0f, x0, x1, x2, x3);
	py = cubicbezier_point(0.0f, y0, y1, y2, y3);

	for (i = 1; i <= steps; i++) {
		t = (double)i / (double)steps;

		cx = cubicbezier_point(t, x0, x1, x2, x3);
		cy = cubicbezier_point(t, y0, y1, y2, y3);

		dx = cx - px;
		dy = cy - py;

		length = length + sqrtf(dx * dx + dy * dy);

		px = cx;
		py = cy;
	}

	return length;
}
