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
	mote->types = NULL;
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
	if (mote->types != NULL)
		free(mote->types);
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
		if (mote->types != NULL)
			free(mote->types);
		if (mote->attrs != NULL)
			free(mote->attrs);

		mote->buffers = (double *)malloc(sizeof(double[10]) * max);
		if (mote->buffers == NULL)
			return -1;
		memset(mote->buffers, 0, sizeof(double[10]) * max);

		mote->types = (uint32_t *)malloc(sizeof(uint32_t) * max);
		if (mote->types == NULL)
			return -1;
		memset(mote->types, 0, sizeof(uint32_t) * max);

		mote->attrs = (uint32_t *)malloc(sizeof(uint32_t) * max);
		if (mote->attrs == NULL)
			return -1;
		memset(mote->attrs, 0, sizeof(uint32_t) * max);
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
	mote->rcount += count;

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

				mote->buffers[to * 10 + 0] = mote->buffers[from * 10 + 0];
				mote->buffers[to * 10 + 1] = mote->buffers[from * 10 + 1];
				mote->buffers[to * 10 + 2] = mote->buffers[from * 10 + 2];
				mote->buffers[to * 10 + 3] = mote->buffers[from * 10 + 3];
				mote->buffers[to * 10 + 4] = mote->buffers[from * 10 + 4];
				mote->buffers[to * 10 + 5] = mote->buffers[from * 10 + 5];
				mote->buffers[to * 10 + 6] = mote->buffers[from * 10 + 6];
				mote->buffers[to * 10 + 7] = mote->buffers[from * 10 + 7];
				mote->buffers[to * 10 + 8] = mote->buffers[from * 10 + 8];
				mote->buffers[to * 10 + 9] = mote->buffers[from * 10 + 9];
				mote->types[to] = mote->types[from];
				mote->attrs[to] = mote->attrs[from];
			}

			mote->lcount--;
		}
	}

	return 0;
}
