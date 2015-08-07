#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showloop.h>
#include <showcanvas.h>
#include <nemoxml.h>
#include <nemomisc.h>

struct showone *nemoshow_loop_create(void)
{
	struct showloop *loop;
	struct showone *one;

	loop = (struct showloop *)malloc(sizeof(struct showloop));
	if (loop == NULL)
		return NULL;
	memset(loop, 0, sizeof(struct showloop));

	one = &loop->base;
	one->type = NEMOSHOW_LOOP_TYPE;
	one->update = nemoshow_loop_update;
	one->destroy = nemoshow_loop_destroy;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "begin", &loop->begin, sizeof(int32_t));
	nemoobject_set_reserved(&one->object, "end", &loop->end, sizeof(int32_t));

	return one;
}

void nemoshow_loop_destroy(struct showone *one)
{
	struct showloop *loop = NEMOSHOW_LOOP(one);

	nemoshow_one_finish(one);

	free(loop);
}

int nemoshow_loop_arrange(struct nemoshow *show, struct showone *one)
{
	struct showloop *loop = NEMOSHOW_LOOP(one);

	loop->canvas = nemoshow_one_get_canvas(one);
}

int nemoshow_loop_update(struct nemoshow *show, struct showone *one)
{
	struct showloop *loop = NEMOSHOW_LOOP(one);

	if (loop->canvas != NULL) {
		nemoshow_canvas_damage_all(loop->canvas);
	}

	return 0;
}
