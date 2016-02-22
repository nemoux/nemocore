#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showease.h>
#include <nemoxml.h>
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

int nemoshow_ease_arrange(struct showone *one)
{
	struct showease *ease = NEMOSHOW_EASE(one);

	if (strcmp(ease->type, "bezier") == 0) {
		nemoease_set_cubic(&ease->ease, ease->x0, ease->y0, ease->x1, ease->y1);
	} else {
		static struct easemap {
			char name[32];

			int type;
		} maps[] = {
			{ "circular_in",				NEMOEASE_CIRCULAR_IN_TYPE },
			{ "circular_out",				NEMOEASE_CIRCULAR_OUT_TYPE },
			{ "circular_inout",			NEMOEASE_CIRCULAR_INOUT_TYPE },
			{ "cubic_in",						NEMOEASE_CUBIC_IN_TYPE },
			{ "cubic_out",					NEMOEASE_CUBIC_OUT_TYPE },
			{ "cubic_inout",				NEMOEASE_CUBIC_INOUT_TYPE },
			{ "exponential_in",			NEMOEASE_EXPONENTIAL_IN_TYPE },
			{ "exponential_out",		NEMOEASE_EXPONENTIAL_OUT_TYPE },
			{ "exponential_inout",	NEMOEASE_EXPONENTIAL_INOUT_TYPE },
			{ "linear",							NEMOEASE_LINEAR_TYPE },
			{ "quadratic_in",				NEMOEASE_QUADRATIC_IN_TYPE },
			{ "quadratic_out",			NEMOEASE_QUADRATIC_OUT_TYPE },
			{ "quadratic_inout",		NEMOEASE_QUADRATIC_INOUT_TYPE },
			{ "quartic_in",					NEMOEASE_QUARTIC_IN_TYPE },
			{ "quartic_out",				NEMOEASE_QUARTIC_OUT_TYPE },
			{ "quartic_inout",			NEMOEASE_QUARTIC_INOUT_TYPE },
			{ "quintic_in",					NEMOEASE_QUINTIC_IN_TYPE },
			{ "quintic_out",				NEMOEASE_QUINTIC_OUT_TYPE },
			{ "quintic_inout",			NEMOEASE_QUINTIC_INOUT_TYPE },
			{ "sinusoidal_in",			NEMOEASE_SINUSOIDAL_IN_TYPE },
			{ "sinusoidal_out",			NEMOEASE_SINUSOIDAL_OUT_TYPE },
			{ "sinusoidal_inout",		NEMOEASE_SINUSOIDAL_INOUT_TYPE },
		}, *map;

		map = static_cast<struct easemap *>(bsearch(ease->type, static_cast<void *>(maps), sizeof(maps) / sizeof(maps[0]), sizeof(maps[0]), nemoshow_ease_compare));
		if (map == NULL)
			return -1;

		nemoease_set(&ease->ease, map->type);
	}

	return 0;
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
