#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <math.h>

#include <nemomote.h>
#include <emitters/sine.h>

int nemomote_sine_set_property(struct motesine *emitter, double maxrate, double minrate, double period)
{
	emitter->maxrate = maxrate;
	emitter->minrate = minrate;
	emitter->period = period;
	emitter->factor = 2 * M_PI / period;
	emitter->scale = 0.5f * (maxrate - minrate);

	return 0;
}

int nemomote_sine_ready(struct nemomote *mote, struct motesine *emitter)
{
	emitter->timepassed = 0.0f;

	return 0;
}

int nemomote_sine_update(struct nemomote *mote, struct motesine *emitter, double secs)
{
	unsigned int count = 0;

	emitter->timepassed += secs;

	count = floorf(emitter->maxrate * emitter->timepassed + emitter->scale * (1 - cosf(emitter->timepassed * emitter->factor)) / emitter->factor);

	nemomote_ready(mote, count);

	return 0;
}
