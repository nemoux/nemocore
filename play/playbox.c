#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemoplay.h>
#include <playbox.h>
#include <nemomisc.h>

struct playbox *nemoplay_box_create(int size)
{
	struct playbox *box;

	box = (struct playbox *)malloc(sizeof(struct playbox));
	if (box == NULL)
		return NULL;
	memset(box, 0, sizeof(struct playbox));

	box->ones = (struct playone **)malloc(sizeof(struct playone *) * size);
	if (box->ones == NULL)
		goto err1;
	memset(box->ones, 0, sizeof(struct playone *) * size);

	box->nones = 0;
	box->sones = size;

	return box;

err1:
	free(box);

	return NULL;
}

void nemoplay_box_destroy(struct playbox *box)
{
	int i;

	for (i = 0; i < box->nones; i++)
		nemoplay_one_destroy(box->ones[i]);

	free(box->ones);
	free(box);
}

int nemoplay_box_insert_one(struct playbox *box, struct playone *one)
{
	if (box->nones >= box->sones)
		return -1;

	box->ones[box->nones++] = one;

	return 0;
}
