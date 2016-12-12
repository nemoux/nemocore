#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemoplay.h>
#include <playqueue.h>
#include <playbox.h>
#include <nemomisc.h>

struct playbox *nemoplay_box_create_by_queue(struct playqueue *queue)
{
	struct playbox *box;
	struct playone *one;
	int i;

	box = (struct playbox *)malloc(sizeof(struct playbox));
	if (box == NULL)
		return NULL;
	memset(box, 0, sizeof(struct playbox));

	box->count = nemoplay_queue_get_count(queue);

	box->ones = (struct playone **)malloc(sizeof(struct playone *) * box->count);
	if (box->ones == NULL)
		goto err1;

	for (i = 0; i < box->count; i++) {
		one = nemoplay_queue_dequeue(queue);
		if (one == NULL)
			break;

		box->ones[i] = one;
	}

	return box;

err1:
	free(box);

	return NULL;
}

void nemoplay_box_destroy(struct playbox *box)
{
	int i;

	for (i = 0; i < box->count; i++)
		nemoplay_one_destroy(box->ones[i]);

	free(box->ones);
	free(box);
}
