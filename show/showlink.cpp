#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showlink.h>
#include <nemoshow.h>
#include <nemomisc.h>

struct showone *nemoshow_link_create(void)
{
	struct showlink *link;
	struct showone *one;

	link = (struct showlink *)malloc(sizeof(struct showlink));
	if (link == NULL)
		return NULL;
	memset(link, 0, sizeof(struct showlink));

	one = &link->base;
	one->type = NEMOSHOW_LINK_TYPE;
	one->update = nemoshow_link_update;
	one->destroy = nemoshow_link_destroy;

	nemoshow_one_prepare(one);

	return one;
}

void nemoshow_link_destroy(struct showone *one)
{
	struct showlink *link = NEMOSHOW_LINK(one);

	nemoshow_one_finish(one);

	free(link);
}

int nemoshow_link_arrange(struct nemoshow *show, struct showone *one)
{
	struct showlink *link = NEMOSHOW_LINK(one);

	return 0;
}

int nemoshow_link_update(struct nemoshow *show, struct showone *one)
{
	struct showlink *link = NEMOSHOW_LINK(one);

	return 0;
}
