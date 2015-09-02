#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomote.h>
#include <emitters/random.h>

int nemomote_random_set_property(struct moterandom *emitter, double maxrate, double minrate)
{
	emitter->maxrate = maxrate;
	emitter->minrate = minrate;

	return 0;
}

static double inline nemomote_random_get_next_time(struct moterandom *emitter)
{
	double rate = ((double)rand() / RAND_MAX) * (emitter->maxrate - emitter->minrate) + emitter->maxrate;

	return 1 / rate;
}

int nemomote_random_ready(struct nemomote *mote, struct moterandom *emitter)
{
	unsigned int count = 0;

	emitter->timetonext = nemomote_random_get_next_time(emitter);

	return 0;
}

int nemomote_random_update(struct nemomote *mote, struct moterandom *emitter, double secs)
{
	unsigned int count = 0;

	emitter->timetonext -= secs;

	while (emitter->timetonext <= 0.0f) {
		count++;

		emitter->timetonext += nemomote_random_get_next_time(emitter);
	}

	nemomote_ready(mote, count);

	return 0;
}
