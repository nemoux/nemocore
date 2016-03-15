#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showease.h>
#include <nemomisc.h>

struct showone *nemoeases[NEMOEASE_LAST_TYPE];

void __attribute__((constructor(101))) nemoshow_ease_initialize(void)
{
	int i;

	for (i = 0; i < NEMOEASE_LAST_TYPE; i++) {
		nemoeases[i] = nemoshow_ease_create();
		nemoshow_ease_set_type(nemoeases[i], i);
	}
}

void __attribute__((destructor(101))) nemoshow_ease_finalize(void)
{
	int i;

	for (i = 0; i < NEMOEASE_LAST_TYPE; i++) {
		nemoshow_ease_destroy(nemoeases[i]);
	}
}

struct showone *nemoshow_ease_create(void)
{
	struct showease *ease;
	struct showone *one;

	ease = (struct showease *)malloc(sizeof(struct showease));
	if (ease == NULL)
		return NULL;
	memset(ease, 0, sizeof(struct showease));

	one = &ease->base;
	one->type = NEMOSHOW_EASE_TYPE;
	one->update = nemoshow_ease_update;
	one->destroy = nemoshow_ease_destroy;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "type", ease->type, NEMOSHOW_EASE_TYPE_MAX);
	nemoobject_set_reserved(&one->object, "x0", &ease->x0, sizeof(double));
	nemoobject_set_reserved(&one->object, "y0", &ease->y0, sizeof(double));
	nemoobject_set_reserved(&one->object, "x1", &ease->x1, sizeof(double));
	nemoobject_set_reserved(&one->object, "y1", &ease->y1, sizeof(double));

	return one;
}

void nemoshow_ease_destroy(struct showone *one)
{
	struct showease *ease = NEMOSHOW_EASE(one);

	nemoshow_one_finish(one);

	free(ease);
}

static int nemoshow_ease_compare(const void *a, const void *b)
{
	return strcasecmp((const char *)a, (const char *)b);
}

int nemoshow_ease_update(struct showone *one)
{
	return 0;
}

void nemoshow_ease_set_type(struct showone *one, int type)
{
	struct showease *ease = NEMOSHOW_EASE(one);

	nemoease_set(&ease->ease, type);
}

void nemoshow_ease_set_bezier(struct showone *one, double x0, double y0, double x1, double y1)
{
	struct showease *ease = NEMOSHOW_EASE(one);

	nemoease_set_cubic(&ease->ease, x0, y0, x1, y1);
}
