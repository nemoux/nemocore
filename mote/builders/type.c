#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomote.h>
#include <builders/type.h>

int nemomote_type_update(struct nemomote *mote, uint32_t type)
{
	int i;

	for (i = mote->lcount; i < mote->lcount + mote->rcount; i++) {
		mote->types[i] = type;
	}

	return 0;
}

int nemomote_type_set(struct nemomote *mote, int base, int count, uint32_t type)
{
	int i;

	for (i = base; i < base + count; i++) {
		mote->types[i] = type;
	}

	return 0;
}
