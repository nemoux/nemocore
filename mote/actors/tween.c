#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomote.h>
#include <actors/tween.h>
#include <nemoease.h>

int nemomote_tween_prepare(struct motetween *tween, int max)
{
	tween->dst = (double *)malloc(sizeof(double[2]) * max);
	if (tween->dst == NULL)
		return -1;

	tween->src = (double *)malloc(sizeof(double[2]) * max);
	if (tween->src == NULL)
		goto err1;

	tween->sparts = max;
	tween->nparts = 0;

	return 0;

err1:
	free(tween->dst);

	return -1;
}

void nemomote_tween_finish(struct motetween *tween)
{
	free(tween->dst);
	free(tween->src);
}

void nemomote_tween_clear(struct motetween *tween)
{
	tween->nparts = 0;
}

void nemomote_tween_add(struct motetween *tween, double x, double y)
{
	tween->dst[tween->nparts * 2 + 0] = x;
	tween->dst[tween->nparts * 2 + 1] = y;

	tween->nparts++;
}

void nemomote_tween_shuffle(struct motetween *tween)
{
	double v;
	int i, p;

	for (i = 0; i < tween->nparts; i++) {
		p = random_get_int(0, tween->nparts);

		if (i != p) {
			v = tween->dst[i * 2 + 0];
			tween->dst[i * 2 + 0] = tween->dst[p * 2 + 0];
			tween->dst[p * 2 + 0] = v;

			v = tween->dst[i * 2 + 1];
			tween->dst[i * 2 + 1] = tween->dst[p * 2 + 1];
			tween->dst[p * 2 + 1] = v;
		}
	}
}

void nemomote_tween_ready(struct nemomote *mote, uint32_t type, struct motetween *tween, double secs)
{
	int i, p;

	for (i = 0, p = 0; i < mote->lcount && p < tween->nparts; i++) {
		if (mote->types[i] != type)
			continue;

		tween->src[p * 2 + 0] = NEMOMOTE_POSITION_X(mote, i);
		tween->src[p * 2 + 1] = NEMOMOTE_POSITION_Y(mote, i);

		p++;
	}

	tween->dtime = secs;
	tween->rtime = secs;

	tween->done = 0;
}

int nemomote_tween_update(struct nemomote *mote, uint32_t type, struct motetween *tween, double secs)
{
	double t;
	int i, p;

	if (tween->done != 0)
		return 0;

	tween->rtime -= secs;

	if (tween->rtime > 0.0f) {
		t = nemoease_quadratic_inout_function(tween->dtime - tween->rtime, tween->dtime);
	} else {
		t = 1.0f;

		tween->done = 1;
	}

	for (i = 0, p = 0; i < mote->lcount && p < tween->nparts; i++) {
		if (mote->types[i] != type)
			continue;

		NEMOMOTE_POSITION_X(mote, i) = (tween->dst[p * 2 + 0] - tween->src[p * 2 + 0]) * t + tween->src[p * 2 + 0];
		NEMOMOTE_POSITION_Y(mote, i) = (tween->dst[p * 2 + 1] - tween->src[p * 2 + 1]) * t + tween->src[p * 2 + 1];

		p++;
	}

	return tween->done;
}
