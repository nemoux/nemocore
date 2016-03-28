#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemoplay.h>

struct nemoplay *nemoplay_create(void)
{
	struct nemoplay *play;

	play = (struct nemoplay *)malloc(sizeof(struct nemoplay));
	if (play == NULL)
		return NULL;
	memset(play, 0, sizeof(struct nemoplay));

	return play;
}

void nemoplay_destroy(struct nemoplay *play)
{
	free(play);
}
