#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomote.h>
#include <emitters/blast.h>

int nemomote_blast_emit(struct nemomote *mote, unsigned int count)
{
	nemomote_ready(mote, count);

	return 0;
}
