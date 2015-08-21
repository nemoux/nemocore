#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <geometryhelper.h>

double point_get_angle_on_line(double x1, double y1, double x2, double y2, double x3, double y3)
{
	double px = x2 - x1, py = y2 - y1, dab = px * px + py * py;
	double k = ((x3 - x1) * px + (y3 - y1) * py) / dab;
	double x4 = x1 + k * px;
	double y4 = y1 + k * py;

	return atan2(y3 - y4, x3 - x4);
}
