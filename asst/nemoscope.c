#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <math.h>

#include <nemoscope.h>
#include <nemotoken.h>
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

int nemoscope_add_cmd(struct nemoscope *scope, uint32_t tag, const char *cmd)
{
	struct scopeone *one;
	struct nemotoken *token;
	const char *type;
	int i, count;

	token = nemotoken_create(cmd, strlen(cmd));
	if (token == NULL)
		return -1;
	nemotoken_divide(token, ' ');
	nemotoken_divide(token, ':');
	nemotoken_divide(token, ',');
	nemotoken_update(token);

	count = nemotoken_get_token_count(token);

	one = (struct scopeone *)malloc(sizeof(struct scopeone));
	if (one == NULL)
		goto err1;
	memset(one, 0, sizeof(struct scopeone));

	one->array = (float *)malloc(sizeof(float) * count);
	if (one->array == NULL)
		goto err2;

	one->tag = tag;

	type = nemotoken_get_token(token, 0);
	if (type[0] == 'r')
		one->type = NEMOSCOPE_RECT_TYPE;
	else if (type[0] == 'c')
		one->type = NEMOSCOPE_CIRCLE_TYPE;
	else if (type[0] == 'e')
		one->type = NEMOSCOPE_ELLIPSE_TYPE;
	else if (type[0] == 'p')
		one->type = NEMOSCOPE_POLYGON_TYPE;
	else
		goto err3;

	for (i = 0; i < count - 1; i++) {
		one->array[i] = nemotoken_get_double(token, i + 1, 0.0f);
	}

	nemolist_insert(&scope->list, &one->link);

	nemotoken_destroy(token);

	return 0;

err3:
	free(one->array);

err2:
	free(one);

err1:
	nemotoken_destroy(token);

	return -1;
}

int nemoscope_add_rect(struct nemoscope *scope, uint32_t tag, float x, float y, float w, float h)
{
	struct scopeone *one;

	one = (struct scopeone *)malloc(sizeof(struct scopeone));
	if (one == NULL)
		return -1;
	memset(one, 0, sizeof(struct scopeone));

	one->array = (float *)malloc(sizeof(float) * 5);
	if (one->array == NULL)
		goto err1;

	one->tag = tag;
	one->type = NEMOSCOPE_RECT_TYPE;

	one->array[0] = x;
	one->array[1] = y;
	one->array[2] = w;
	one->array[3] = h;

	nemolist_insert(&scope->list, &one->link);

	return 0;

err1:
	free(one);

	return -1;
}

int nemoscope_add_circle(struct nemoscope *scope, uint32_t tag, float x, float y, float r)
{
	struct scopeone *one;

	one = (struct scopeone *)malloc(sizeof(struct scopeone));
	if (one == NULL)
		return -1;
	memset(one, 0, sizeof(struct scopeone));

	one->array = (float *)malloc(sizeof(float) * 4);
	if (one->array == NULL)
		goto err1;

	one->tag = tag;
	one->type = NEMOSCOPE_CIRCLE_TYPE;

	one->array[0] = x;
	one->array[1] = y;
	one->array[2] = r;

	nemolist_insert(&scope->list, &one->link);

	return 0;

err1:
	free(one);

	return -1;
}

int nemoscope_add_ellipse(struct nemoscope *scope, uint32_t tag, float x, float y, float rx, float ry)
{
	struct scopeone *one;

	one = (struct scopeone *)malloc(sizeof(struct scopeone));
	if (one == NULL)
		return -1;
	memset(one, 0, sizeof(struct scopeone));

	one->array = (float *)malloc(sizeof(float) * 5);
	if (one->array == NULL)
		goto err1;

	one->tag = tag;
	one->type = NEMOSCOPE_ELLIPSE_TYPE;

	one->array[0] = x;
	one->array[1] = y;
	one->array[2] = rx;
	one->array[3] = ry;

	nemolist_insert(&scope->list, &one->link);

	return 0;

err1:
	free(one);

	return -1;
}

uint32_t nemoscope_pick(struct nemoscope *scope, float x, float y)
{
	struct scopeone *one;

	nemolist_for_each(one, &scope->list, link) {
		if (one->type == NEMOSCOPE_RECT_TYPE) {
			if (one->array[0] <= x && one->array[0] + one->array[2] < x &&
					one->array[1] <= y && one->array[1] + one->array[3] < y)
				return one->tag;
		} else if (one->type == NEMOSCOPE_CIRCLE_TYPE) {
			float r = one->array[2];
			float cx = one->array[0] + r;
			float cy = one->array[1] + r;
			float dx = cx - x;
			float dy = cy - y;

			if (sqrtf(dx * dx + dy * dy) <= r)
				return one->tag;
		} else if (one->type == NEMOSCOPE_ELLIPSE_TYPE) {
			float rx = one->array[2];
			float ry = one->array[3];
			float cx = one->array[0] + rx;
			float cy = one->array[1] + ry;
			float dx = x - cx;
			float dy = y - cy;

			if (((dx * dx) / (rx * rx) + (dy * dy) / (ry * ry)) <= 1.0f)
				return one->tag;
		} else if (one->type == NEMOSCOPE_POLYGON_TYPE) {
		}
	}

	return 0;
}
