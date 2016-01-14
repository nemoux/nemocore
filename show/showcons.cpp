#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showcons.h>
#include <nemoshow.h>
#include <nemoxml.h>
#include <nemomisc.h>

struct showone *nemoshow_cons_create(void)
{
	struct showcons *cons;
	struct showone *one;

	cons = (struct showcons *)malloc(sizeof(struct showcons));
	if (cons == NULL)
		return NULL;
	memset(cons, 0, sizeof(struct showcons));

	one = &cons->base;
	one->type = NEMOSHOW_CONS_TYPE;
	one->update = nemoshow_cons_update;
	one->destroy = nemoshow_cons_destroy;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "name", cons->name, NEMOSHOW_EXPR_SYMBOL_NAME_MAX);
	nemoobject_set_reserved(&one->object, "value", &cons->value, sizeof(double));

	return one;
}

void nemoshow_cons_destroy(struct showone *one)
{
	struct showcons *cons = NEMOSHOW_CONS(one);

	nemoshow_one_finish(one);

	free(cons);
}

int nemoshow_cons_arrange(struct showone *one)
{
	struct showcons *cons = NEMOSHOW_CONS(one);

	return 0;
}

int nemoshow_cons_update(struct showone *one)
{
	struct showcons *cons = NEMOSHOW_CONS(one);

	nemoshow_update_symbol(one->show, cons->name, cons->value);

	return 0;
}
