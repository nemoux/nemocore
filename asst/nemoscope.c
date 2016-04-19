#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <math.h>

#include <nemoscope.h>
#include <nemolog.h>

struct nemoscope *nemoscope_create(void)
{
	struct nemoscope *scope;

	scope = (struct nemoscope *)malloc(sizeof(struct nemoscope));
	if (scope == NULL)
		return NULL;
	memset(scope, 0, sizeof(struct nemoscope));

	nemolist_init(&scope->list);

	return scope;
}

void nemoscope_destroy(struct nemoscope *scope)
{
	struct scopeone *one, *none;

	nemolist_for_each_safe(one, none, &scope->list, link) {
		nemolist_remove(&one->link);

		free(one);
	}

	nemolist_remove(&scope->list);

	free(scope);
}

void nemoscope_clear(struct nemoscope *scope)
{
	struct scopeone *one, *none;

	nemolist_for_each_safe(one, none, &scope->list, link) {
		nemolist_remove(&one->link);

		free(one);
	}

	nemolist_init(&scope->list);
}

int nemoscope_add(struct nemoscope *scope, uint32_t tag, uint32_t type, float x, float y, float w, float h)
{
	struct scopeone *one;

	if (w <= 0.0f || h <= 0.0f)
		return -1;

	one = (struct scopeone *)malloc(sizeof(struct scopeone));
	if (one == NULL)
		return -1;
	memset(one, 0, sizeof(struct scopeone));

	one->tag = tag;

	one->x = x;
	one->y = y;
	one->w = w;
	one->h = h;
	one->type = type;

	nemolist_insert(&scope->list, &one->link);

	return 0;
}

uint32_t nemoscope_pick(struct nemoscope *scope, float x, float y)
{
	struct scopeone *one;

	nemolist_for_each(one, &scope->list, link) {
		if (one->type == NEMOSCOPE_RECT_TYPE) {
			if (one->x <= x && one->x + one->w < x &&
					one->y <= y && one->y + one->h < y)
				return one->tag;
		} else if (one->type == NEMOSCOPE_CIRCLE_TYPE) {
			if (one->w == one->h) {
				float r = one->w / 2.0f;
				float cx = one->x + r;
				float cy = one->y + r;
				float dx = cx - x;
				float dy = cy - y;

				if (sqrtf(dx * dx + dy * dy) <= r)
					return one->tag;
			} else {
				float rx = one->w / 2.0f;
				float ry = one->h / 2.0f;
				float cx = one->x + rx;
				float cy = one->y + ry;
				float dx = x - cx;
				float dy = y - cy;

				if (((dx * dx) / (rx * rx) + (dy * dy) / (ry * ry)) <= 1.0f)
					return one->tag;
			}
		}
	}

	return 0;
}
