#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomote.h>
#include <nemomisc.h>

struct nemomote *nemomote_create(int max)
{
	struct nemomote *mote;

	mote = (struct nemomote *)malloc(sizeof(struct nemomote));
	if (mote == NULL)
		return NULL;
	memset(mote, 0, sizeof(struct nemomote));

	mote->buffers = (double *)malloc(sizeof(double[12]) * max);
	if (mote->buffers == NULL)
		goto err1;
	memset(mote->buffers, 0, sizeof(double[12]) * max);

	mote->types = (uint32_t *)malloc(sizeof(uint32_t) * max);
	if (mote->types == NULL)
		goto err2;
	memset(mote->types, 0, sizeof(uint32_t) * max);

	mote->attrs = (uint32_t *)malloc(sizeof(uint32_t) * max);
	if (mote->attrs == NULL)
		goto err3;
	memset(mote->attrs, 0, sizeof(uint32_t) * max);

	mote->tweens = (double *)malloc(sizeof(double[16]) * max);
	if (mote->tweens == NULL)
		goto err4;
	memset(mote->tweens, 0, sizeof(double[16]) * max);

	mote->mcount = max;
	mote->lcount = 0;
	mote->rcount = 0;

	return mote;

err4:
	free(mote->attrs);

err3:
	free(mote->types);

err2:
	free(mote->buffers);

err1:
	free(mote);

	return NULL;
}

void nemomote_destroy(struct nemomote *mote)
{
	if (mote->buffers != NULL)
		free(mote->buffers);
	if (mote->types != NULL)
		free(mote->types);
	if (mote->attrs != NULL)
		free(mote->attrs);
	if (mote->tweens != NULL)
		free(mote->tweens);

	free(mote);
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
				mote->types[to] = mote->types[from];
				mote->attrs[to] = mote->attrs[from];
				mote->tweens[to * 16 + 0] = mote->tweens[from * 16 + 0];
				mote->tweens[to * 16 + 1] = mote->tweens[from * 16 + 1];
				mote->tweens[to * 16 + 2] = mote->tweens[from * 16 + 2];
				mote->tweens[to * 16 + 3] = mote->tweens[from * 16 + 3];
				mote->tweens[to * 16 + 4] = mote->tweens[from * 16 + 4];
				mote->tweens[to * 16 + 5] = mote->tweens[from * 16 + 5];
				mote->tweens[to * 16 + 6] = mote->tweens[from * 16 + 6];
				mote->tweens[to * 16 + 7] = mote->tweens[from * 16 + 7];
				mote->tweens[to * 16 + 8] = mote->tweens[from * 16 + 8];
				mote->tweens[to * 16 + 9] = mote->tweens[from * 16 + 9];
				mote->tweens[to * 16 + 10] = mote->tweens[from * 16 + 10];
				mote->tweens[to * 16 + 11] = mote->tweens[from * 16 + 11];
				mote->tweens[to * 16 + 12] = mote->tweens[from * 16 + 12];
				mote->tweens[to * 16 + 13] = mote->tweens[from * 16 + 13];
				mote->tweens[to * 16 + 14] = mote->tweens[from * 16 + 14];
				mote->tweens[to * 16 + 15] = mote->tweens[from * 16 + 15];
			}

			mote->lcount--;
		}
	}

	return 0;
}

int nemomote_get_one_by_type(struct nemomote *mote, uint32_t type)
{
	int base;
	int i;

	base = random_get_integer(0, mote->lcount);

	for (i = 0; i < mote->lcount; i++) {
		if (mote->types[(base + i) % mote->lcount] == type)
			return (base + i) % mote->lcount;
	}

	return -1;
}
