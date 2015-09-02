#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomote.h>
#include <emitters/blast.h>

int nemomote_blast_set_property(struct moteblast *emitter, unsigned int startcount)
{
	emitter->startcount = startcount;

	return 0;
}

int nemomote_blast_ready(struct nemomote *mote, struct moteblast *emitter)
{
	nemomote_ready(mote, emitter->startcount);

	return 0;
}

int nemomote_blast_emit(struct nemomote *mote, struct moteblast *emitter, double secs)
{
	return 0;
}
