#ifndef	__NEMO_PITCH_FILTER_H__
#define	__NEMO_PITCH_FILTER_H__

#include <stdint.h>

struct pitchsample {
	double x, y;
	double dx, dy;
	uint32_t dt;
};

struct pitchfilter {
	struct pitchsample *samples;

	double min_dist;
	uint32_t max_sample;

	uint32_t index;

	double dist;
	uint32_t dtime;
	double dx, dy;

	uint32_t time;
};

extern struct pitchfilter *pitchfilter_create(double min_dist, uint32_t max_sample);
extern void pitchfilter_destroy(struct pitchfilter *filter);

extern void pitchfilter_dispatch(struct pitchfilter *filter, double x, double y, double dx, double dy, uint32_t time);
extern int pitchfilter_flush(struct pitchfilter *filter);

#endif
