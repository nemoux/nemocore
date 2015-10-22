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

struct pitchfilter *pitchfilter_create(uint32_t max_samples, uint32_t dir_samples)
{
	struct pitchfilter *filter;

	filter = (struct pitchfilter *)malloc(sizeof(struct pitchfilter));
	if (filter == NULL)
		return NULL;
	memset(filter, 0, sizeof(struct pitchfilter));

	filter->samples = (struct pitchsample *)malloc(sizeof(struct pitchsample) * max_samples);
	if (filter->samples == NULL)
		goto err1;
	memset(filter->samples, 0, sizeof(struct pitchsample) * max_samples);

	filter->max_samples = max_samples;
	filter->dir_samples = dir_samples;

	filter->sindex = 0;
	filter->eindex = 0;

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

	filter->samples[filter->eindex].x = x;
	filter->samples[filter->eindex].y = y;
	filter->samples[filter->eindex].dx = dx;
	filter->samples[filter->eindex].dy = dy;
	filter->samples[filter->eindex].dt = time - filter->time;

	filter->eindex = (filter->eindex + 1) % filter->max_samples;

	if (filter->sindex == filter->eindex)
		filter->sindex = (filter->sindex + 1) % filter->max_samples;

	filter->time = time;
}

int pitchfilter_flush(struct pitchfilter *filter)
{
	double dx, dy, dist;
	uint32_t dtime;
	int index;
	int i;

	if (filter->sindex == filter->eindex)
		return 0;

	filter->dist = 0.0f;
	filter->dtime = 0;
	filter->dx = 0.0f;
	filter->dy = 0.0f;

	for (i = 0; i < filter->max_samples; i++) {
		index = (filter->sindex + i) % filter->max_samples;
		if (index == filter->eindex)
			break;

		dx = filter->samples[index].dx;
		dy = filter->samples[index].dy;
		dtime = filter->samples[index].dt;
		dist = sqrtf(dx * dx + dy * dy);

		filter->dist = filter->dist + dist;
		filter->dtime = filter->dtime + dtime;
	}

	for (i = 0; i < filter->dir_samples; i++) {
		index = (filter->eindex + filter->max_samples - i - 1) % filter->max_samples;

		filter->dx = filter->dx + filter->samples[index].dx;
		filter->dy = filter->dy + filter->samples[index].dy;

		if (index == filter->sindex)
			break;
	}

	dist = sqrtf(filter->dx * filter->dx + filter->dy * filter->dy);

	filter->dx = filter->dx / dist;
	filter->dy = filter->dy / dist;

	if (isnan(filter->dx) || isnan(filter->dy) || filter->dtime == 0)
		return 0;

	return 1;
}
