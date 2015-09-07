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

int nemomote_type_set(struct nemomote *mote, uint32_t type, uint32_t ntype)
{
	int i;

	for (i = 0; i < mote->lcount; i++) {
		if (mote->types[i] == type)
			mote->types[i] = ntype;
	}

	return 0;
}

int nemomote_type_set_one(struct nemomote *mote, int index, uint32_t type)
{
	mote->types[index] = type;

	return 0;
}
