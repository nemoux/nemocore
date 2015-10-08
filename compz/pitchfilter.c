#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <math.h>
#include <float.h>

#include <pitchfilter.h>
#include <nemomisc.h>

struct pitchfilter *pitchfilter_create(double min_dist, uint32_t max_sample)
{
	struct pitchfilter *filter;

	filter = (struct pitchfilter *)malloc(sizeof(struct pitchfilter));
	if (filter == NULL)
		return NULL;
	memset(filter, 0, sizeof(struct pitchfilter));

	filter->samples = (struct pitchsample *)malloc(sizeof(struct pitchsample) * max_sample);
	if (filter->samples == NULL)
		goto err1;
	memset(filter->samples, 0, sizeof(struct pitchsample) * max_sample);

	filter->min_dist = min_dist;
	filter->max_sample = max_sample;

	filter->index = 0;

	return filter;

err1:
	free(filter);

	return NULL;
}

void pitchfilter_destroy(struct pitchfilter *filter)
{
	free(filter->samples);
	free(filter);
}

void pitchfilter_dispatch(struct pitchfilter *filter, double x, double y, double dx, double dy, uint32_t time)
{
	double dist = sqrtf(dx * dx + dy * dy);

	if (filter->time == 0)
		filter->time = time;

	if (dist < filter->min_dist) {
		filter->index = 0;
	} else if (filter->index < filter->max_sample) {
		filter->samples[filter->index].x = x;
		filter->samples[filter->index].y = y;
		filter->samples[filter->index].dx = dx;
		filter->samples[filter->index].dy = dy;
		filter->samples[filter->index].dt = time - filter->time;

		filter->index++;
	}

	filter->time = time;
}

int pitchfilter_flush(struct pitchfilter *filter)
{
	int i;

	filter->dist = 0.0f;
	filter->dtime = 0;
	filter->dx = 0.0f;
	filter->dy = 0.0f;

	for (i = 0; i < filter->index; i++) {
		double dx = filter->samples[i].dx;
		double dy = filter->samples[i].dy;
		uint32_t dtime = filter->samples[i].dt;
		double dist = sqrtf(dx * dx + dy * dy);

		filter->dist = filter->dist + dist;
		filter->dtime = filter->dtime + dtime;
	}

	if (filter->index > 0) {
		double dx, dy, dist;
		double x0, y0, x1, y1;

		x0 = filter->samples[0].x;
		y0 = filter->samples[0].y;
		x1 = filter->samples[filter->index - 1].x;
		y1 = filter->samples[filter->index - 1].y;

		dx = x1 - x0;
		dy = y1 - y0;

		dist = sqrtf(dx * dx + dy * dy);

		dx = 0.0f;
		dy = 0.0f;

		for (i = 0; i < filter->index - 1; i++) {
			x0 = filter->samples[i + 0].x;
			y0 = filter->samples[i + 0].y;
			x1 = filter->samples[i + 1].x;
			y1 = filter->samples[i + 1].y;

			dx += (x1 - x0);
			dy += (y1 - y0);
		}

		filter->dx = dx / dist;
		filter->dy = dy / dist;
	}

	if (isnan(filter->dx) || isnan(filter->dy))
		return 0;

	return 1;
}
