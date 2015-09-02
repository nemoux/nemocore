#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomote.h>

int nemomote_init(struct nemomote *mote)
{
	mote->buffers = NULL;
	mote->attrs = NULL;
	mote->mcount = 0;
	mote->lcount = 0;
	mote->rcount = 0;

	return 0;
}

int nemomote_exit(struct nemomote *mote)
{
	if (mote->buffers != NULL)
		free(mote->buffers);
	if (mote->attrs != NULL)
		free(mote->attrs);

	return 0;
}

int nemomote_set_max_particles(struct nemomote *mote, unsigned int max)
{
	int i;

	if (mote->mcount != max) {
		if (mote->buffers != NULL)
			free(mote->buffers);
		if (mote->attrs != NULL)
			free(mote->attrs);

		mote->buffers = (double *)malloc(sizeof(double[12]) * max);
		if (mote->buffers == NULL)
			return -1;
		mote->attrs = (unsigned int *)malloc(sizeof(unsigned int) * max);
		if (mote->attrs == NULL)
			return -1;
	}

	mote->mcount = max;
	mote->lcount = 0;
	mote->rcount = 0;

	return 0;
}

int nemomote_reset(struct nemomote *mote)
{
	mote->lcount = 0;
	mote->rcount = 0;

	return 0;
}

int nemomote_ready(struct nemomote *mote, unsigned int count)
{
	mote->rcount = count;

	if (mote->lcount + mote->rcount > mote->mcount)
		mote->rcount = mote->mcount - mote->lcount;

	return 0;
}

int nemomote_commit(struct nemomote *mote)
{
	mote->lcount += mote->rcount;
	mote->rcount = 0;

	return 0;
}

int nemomote_cleanup(struct nemomote *mote)
{
	int i;

	for (i = 0; i < mote->lcount; i++) {
		if (mote->attrs[i] & NEMOMOTE_DEAD_BIT) {
			if (i < mote->lcount) {
				int to = i;
				int from = mote->lcount - 1;

				mote->buffers[to * 12 + 0] = mote->buffers[from * 12 + 0];
				mote->buffers[to * 12 + 1] = mote->buffers[from * 12 + 1];
				mote->buffers[to * 12 + 2] = mote->buffers[from * 12 + 2];
				mote->buffers[to * 12 + 3] = mote->buffers[from * 12 + 3];
				mote->buffers[to * 12 + 4] = mote->buffers[from * 12 + 4];
				mote->buffers[to * 12 + 5] = mote->buffers[from * 12 + 5];
				mote->buffers[to * 12 + 6] = mote->buffers[from * 12 + 6];
				mote->buffers[to * 12 + 7] = mote->buffers[from * 12 + 7];
				mote->buffers[to * 12 + 8] = mote->buffers[from * 12 + 8];
				mote->buffers[to * 12 + 9] = mote->buffers[from * 12 + 9];
				mote->buffers[to * 12 + 10] = mote->buffers[from * 12 + 10];
				mote->buffers[to * 12 + 11] = mote->buffers[from * 12 + 11];
				mote->attrs[to] = mote->attrs[from];
			}

			mote->lcount--;
		}
	}

	return 0;
}
